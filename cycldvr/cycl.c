
/*
 * $Log: cycl.c,v $
 * Revision 1.2  1999/02/09 17:26:43  banz
 * ring buffer overflow
 * spltty() interrupt priorities and things
 *
 * Revision 1.1  1998/11/08 21:56:31  banz
 * Initial revision
 *
 */

static char rcsid[] = "$Id: cycl.c,v 1.2 1999/02/09 17:26:43 banz Exp banz $";

#include <cycl.h>

#if NCYCL > 0

#include <sys/errno.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/signalvar.h>

struct clockframe;

#include "/usr/photondev/src/cycldvr/cycl.h"
#ifdef DEVFS
#include <sys/devfsext.h>
#endif

#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/frame.h>

#include <i386/isa/isa.h>
#include <i386/isa/isa_device.h>
#include <i386/isa/timerreg.h>
#include <i386/isa/icu.h>

#define TXQ_DEPTH 16

#define RXRING_DEPTH 128
cycl_msg rxring[RXRING_DEPTH];
int rxring_head = 0; /* add at head */
int rxring_tail = 0; /* read from tail */

extern void clkintr(struct clockframe );

typedef struct {
  int count;
  int front;
  int rear;
  unsigned char entry[TXQ_DEPTH];
} cyclequeue;

static struct {

  int unit;
  
  unsigned char sync;

  int enable;

  int open;

  pid_t pid;

  int baseport;
  int tx_port;
  int rx_port;
  int divisor_lsb_port;
  int divisor_msb_port;
  int int_enable_port;
  int int_id_port;
  int line_ctr_port;
  int modem_ctr_port;
  int line_stat_port;
  int modem_stat_port;

  int slotno;

  struct selinfo sc_selp;

  unsigned int count;

  unsigned char rcvd[67];
  unsigned char data[67];
  int outptr;

  cyclequeue txqueue;

#ifdef DEVFS
  void *devfs_token;
#endif

} cycl[NCYCL];

static void txq_init(cyclequeue *);
static int txq_add(cyclequeue *, unsigned char);
static unsigned char txq_get(cyclequeue *);
static int txq_size(cyclequeue *);
static int txq_empty(cyclequeue *);
static int txq_full(cyclequeue *);

static void cyclintr(void);

static int cyclprobe(struct isa_device *);
static int cyclattach(struct isa_device *);

struct isa_driver cycldriver = { cyclprobe, cyclattach, "cycl"};

#define CDEV_MAJOR 20

static d_open_t cyclopen;
static d_close_t cyclclose;
static d_read_t cyclread;
static d_write_t cyclwrite;
static d_ioctl_t cyclioctl;
static d_select_t cyclselect;
void  cyclwakeup(void);

void (*cyclintrptr) __P((void)) = NULL;

static struct cdevsw cycl_cdevsw =
{ cyclopen, cyclclose, cyclread, cyclwrite, /*20*/
  cyclioctl, nostop, nullreset, nodevtotty, /* cycle */
  cyclselect, nommap, NULL, "cycl", NULL, -1 };


static int
cyclprobe (struct isa_device *dev) {
  /* we're assuming it's there. */
  return 1;
}

static u_int cycl_imask = HWI_MASK | SWI_MASK;

static int
cyclattach (struct isa_device *dev) {
  
  int u = dev->id_unit;
  int i;

  cycl[u].unit = u;
  
  cycl[u].enable = 0;

  cycl[u].sync = PSYNC;

  cycl[u].pid = 0;

  cycl[u].open = 0;
  cycl[u].count = 0;
  cycl[u].baseport = dev->id_iobase;

  cycl[u].tx_port = cycl[u].rx_port = cycl[u].divisor_lsb_port = cycl[u].baseport;
  cycl[u].divisor_msb_port = cycl[u].int_enable_port = cycl[u].baseport + 1;
  cycl[u].int_id_port = cycl[u].baseport + 2;
  cycl[u].line_ctr_port = cycl[u].baseport + 3;
  cycl[u].modem_ctr_port = cycl[u].baseport + 4;
  cycl[u].line_stat_port = cycl[u].baseport + 5;
  cycl[u].modem_stat_port = cycl[u].baseport + 6;

  cycl[u].slotno = 0;
  
  txq_init(&cycl[u].txqueue);

  for(i=0; i<67; ++i)
    cycl[u].rcvd[i] = 0xff;

  bzero(cycl[u].data, 67);
  
  cycl[u].outptr = 0;

  /*setup serial port.*/
  outb(cycl[u].line_ctr_port, 128);
  outb(cycl[u].divisor_lsb_port, 0x60);
  outb(cycl[u].divisor_msb_port, 0x00);
  outb(cycl[u].line_ctr_port, 3);
  outb(cycl[u].int_enable_port, 0);
  outb(cycl[u].modem_ctr_port, 2);
  outb(cycl[u].int_id_port, 0);
#ifdef DEVFS
  cycl[u].devfs_token = devfs_add_devswf(&cycl_cdevsw, 0, DV_CHR, 0, 0,
					 0600, "cycl");
#endif

  disable_intr();
  cyclintrptr = cyclintr;
  enable_intr();
  return 1;
}

static void cycl_keyup(void) {
  outb(cycl[0].modem_ctr_port, 3);
}

static void cycl_keydown(void) {
  outb(cycl[0].modem_ctr_port, 0);
}

static void cyclintr(void) {

  unsigned char x;

  int wake = 0;
  int s;

  disable_intr();

  s = splclock();

  if (cycl[0].enable == 0) {

    if (cycl[0].slotno == 67)
      cycl[0].slotno = 0;

    ++cycl[0].slotno;

    enable_intr();

    splx(s);

    return;
  }

  /* quick, read in a byte (if it's there) and shove it in our output
     buffer */
  x = inb(cycl[0].modem_stat_port); /* read for yucks */

  x = inb(cycl[0].line_stat_port);

#ifdef 0
  if (x & 0x02) {
    printf("cycl0: overrun error\n");
  }
  if (x & 0x04) {
    printf("cycl0: parity error\n");
  }
  if (x & 0x08) {
    printf("cycl0: framing error\n");
  }
  if (x & 0x10) {
    printf("cycl0: break condition\n");
  }
#endif
  
  if (x & 0x01) {

    cycl[0].rcvd[cycl[0].slotno] = RX;
    cycl[0].data[cycl[0].slotno] = inb(cycl[0].rx_port);
    
    rxring[rxring_head].serial = cycl[0].count;
    rxring[rxring_head].slot = cycl[0].slotno;
    rxring[rxring_head].data = cycl[0].data[cycl[0].slotno];
    rxring[rxring_head].rcvd = RX;    
    rxring_head = ( rxring_head + 1 ) % RXRING_DEPTH;
    
    if ( rxring_head == rxring_tail ) {
      printf("cycl0: rx buffer overflow\n");
      rxring_tail = ( rxring_tail + 1 ) % RXRING_DEPTH;
    }

    wake = 1;

  } else {
    cycl[0].rcvd[cycl[0].slotno] = 0x00;
    cycl[0].data[cycl[0].slotno] = 0x00;
  }

  switch (cycl[0].slotno) {

  case 67:
    cycl[0].slotno = 0;
    outb(cycl[0].tx_port, cycl[0].sync);
    cycl[0].rcvd[cycl[0].slotno] = TX;
    cycl[0].data[cycl[0].slotno] = cycl[0].sync;
    ++cycl[0].count;
    cycl[0].count %= 255;

    rxring[rxring_head].serial = cycl[0].count;
    rxring[rxring_head].slot = cycl[0].slotno;
    rxring[rxring_head].data = cycl[0].sync;
    rxring[rxring_head].rcvd = TX;    
    rxring_head = ( rxring_head + 1 ) % RXRING_DEPTH;

    if ( rxring_head == rxring_tail ) {
      printf("cycl0: rx buffer overflow\n");
      rxring_tail = ( rxring_tail + 1 ) % RXRING_DEPTH;
    }

    wake = 1;
    break;
    
  case 1:
  case 30:
    cycl_keydown();
    break;


  case 24:
  case 53:
    cycl_keyup();
    break;
  }

  /* transmits... */
  if (((cycl[0].slotno > 24) && (cycl[0].slotno < 30)) ||
      ((cycl[0].slotno > 54) && (cycl[0].slotno < 60))) {

    if (txq_size(&cycl[0].txqueue) != 0) {
      x = txq_get(&cycl[0].txqueue);
      
      outb(cycl[0].tx_port, x);
      cycl[0].rcvd[cycl[0].slotno] = TX;
      cycl[0].data[cycl[0].slotno] = x;

      rxring[rxring_head].serial = cycl[0].count;
      rxring[rxring_head].slot = cycl[0].slotno;
      rxring[rxring_head].data = x;
      rxring[rxring_head].rcvd = TX;    
      rxring_head = ( rxring_head + 1 ) % RXRING_DEPTH;           

      if ( rxring_head == rxring_tail ) {
	printf("cycl0: rx buffer overflow\n");
	rxring_tail = ( rxring_tail + 1 ) % RXRING_DEPTH;
      }

      wake = 1;
    }
  }

  
  ++cycl[0].slotno;

  enable_intr();

  splx(s);

  if (wake == 1)
    cyclwakeup();

  return;

}

static void txq_init(cyclequeue *q) {
  q->count = 0;
  q->front = 0;
  q->rear = 0;
}

int txq_add(cyclequeue *q, unsigned char a) {

  if (q->count == TXQ_DEPTH)
    return 0;

  q->count++;
  q->entry[q->rear] = a;
  q->rear = (q->rear+1) % TXQ_DEPTH;

  return 1;
}

unsigned char txq_get(cyclequeue *q) {

  unsigned char a;

  if (q->count == 0)
    return 0;

  q->count--;
  a = q->entry[q->front];
  q->front = (q->front + 1) % TXQ_DEPTH;
  
  return a;
}

int txq_size(cyclequeue *q) {
  return q->count;
}

int txq_empty(cyclequeue *q) {
  if (q->count == 0)
    return 1;
  else
    return 0;
}

int txq_full(cyclequeue *q) {
  if (q->count == TXQ_DEPTH)
    return 1;
  else
    return 0;
}

static int
cyclioctl(dev_t dev, int cmd, caddr_t data, int flag, struct proc *p)
{

  unsigned int x;
  unsigned char c;
  int s;

  s = splclock();

  switch (cmd) {

  case CYCL_ENABLE:
    x = *(unsigned int *) data;
    if (x == 1) {
      rxring_head = rxring_tail = 0;
      /* reinit serial port */
      disable_intr();
      outb(cycl[0].line_ctr_port, 128);
      outb(cycl[0].divisor_lsb_port, 0x60);
      outb(cycl[0].divisor_msb_port, 0x00);
      outb(cycl[0].line_ctr_port, 3);
      outb(cycl[0].int_enable_port, 0);
      outb(cycl[0].modem_ctr_port, 2);
      outb(cycl[0].int_id_port, 0);
      cycl[0].enable = 1;
      enable_intr();
    } else {
      rxring_head = rxring_tail = 0;
      cycl[0].enable = 0;
    }
    break;

  case CYCL_SETSYNC:
    x = *(unsigned int *) data;
    cycl[0].sync = (unsigned char) x;
    break;

  case CYCL_DUMPXMIT:
    txq_init(&cycl[0].txqueue);
    break;

  default:
    splx(s);
    return ENXIO;
  }
    
  splx(s);

  return 0;

}

static int
cyclopen (dev_t dev, int flags, int fmt, struct proc *p) {

  if (cycl[0].open)
    return EBUSY;

  cycl[0].open = 1;

  cycl[0].pid = p->p_pid;

  return 0;
}

static int
cyclclose (dev_t dev, int flags, int fmt, struct proc *p) {

  cycl[0].open = 0;
  cycl[0].enable = 0;
  return 0;

}

static int
cyclread (dev_t dev, struct uio *uio, int flag) {
  
  /* first, see if we have anything _to_ read from... */

  static cycl_msg msg;
  int s;
  /* big change  -- if we've got something in the
     output queue, we move it out... */

  s = splclock();

  disable_intr();

  if ( rxring_head != rxring_tail ) {
    
    /* output what is at rxring_tail */

    memcpy(&msg, &rxring[rxring_tail], sizeof(cycl_msg));

    rxring_tail = (rxring_tail + 1 ) % RXRING_DEPTH;

    splx(s);
    
    enable_intr();

    return uiomove((caddr_t) &msg, sizeof(cycl_msg), uio);

  }

  splx(s);

  enable_intr();

  return uiomove((caddr_t) &msg, 0, uio);

}

static int
cyclselect (dev_t dev, int rw, struct proc *p) {
  
  /* this be wacky -- we basically do a read -- stop where we find something
     that we would have sent out, don't increment the counter, then
     return */

  int t = cycl[0].outptr;

  if ( rw == FWRITE ){
    return 1; /* we can always write.*/
  }

  if ( rw == FREAD ) {
    if ( rxring_head != rxring_tail )
      return 1;

    selrecord(p, &cycl[0].sc_selp);

    return 0;
  }

  return 0;
}


void cyclwakeup(void) {
  struct proc *p;
  
  if (!cycl[0].open)
    return;

  selwakeup(&cycl[0].sc_selp);
  
  if (p = pfind(cycl[0].pid))
    psignal(p, SIGIO);


}

static int
cyclwrite (dev_t dev, struct uio *uio, int flag) {

  unsigned char b;
  int s;

  s = spltty();
  
  uiomove(&b, 1, uio);

  txq_add(&cycl[0].txqueue, b);

  splx(s);

  return 1;
}

static int cycl_cdevsw_installed = 0;

static void cycl_drvinit(void *unused) {
  
  dev_t dev;
  if (!cycl_cdevsw_installed ) {
    dev = makedev(CDEV_MAJOR, 0);
    cdevsw_add(&dev, &cycl_cdevsw, NULL);
    cycl_cdevsw_installed = 1;
  }

}

SYSINIT(cycldev, SI_SUB_DRIVERS, SI_ORDER_MIDDLE+CDEV_MAJOR, cycl_drvinit, NULL)

#endif 
  

  









