
/*
 * $Log: cycl.h,v $
 * Revision 1.1  1998/11/08 21:56:50  banz
 * Initial revision
 *
 */


#include <sys/types.h>
#include <sys/ioccom.h>

typedef  struct {
  unsigned char serial;    /* serial #, for error checking */
  unsigned char slot;      /* slot # */
  unsigned char rcvd;      /* did we receive anything? */
  unsigned char data;      /* character received */
} cycl_msg;

#define NONE 0
#define RX 1
#define TX 2

#define NSYNC 0x00
#define PSYNC 0xC5
#define GSYNC 0xCA
#define ESYNC 0xDD

/* cycl IOCTLs... */

#define CYCL_ENABLE _IOW('J', 1, int)
#define CYCL_SETSYNC _IOW('J', 2, unsigned char)
#define CYCL_DUMPXMIT _IO('P', 3)


  
