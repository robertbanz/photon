#include "cr.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * $Log: cr.c,v $
 * Revision 1.2  1999/01/31 17:03:24  banz
 * /devices/cycl anyone?
 *
 * Revision 1.1  1998/11/08 21:53:40  banz
 * Initial revision
 *
 *
*/

static char rcsid[] = "$Id: cr.c,v 1.2 1999/01/31 17:03:24 banz Exp $";

/* functions to control the central radio (/dev/cycl0) */

static int fd;

int cr_init(void) {
  
  fd = open("/devices/cycl", O_RDWR|O_NONBLOCK);
 
  return fd;

}

int cr_setsync(unsigned char sync) {
  
  int j;

  j = sync;

  switch(sync) {
  case PSYNC:
  case GSYNC:
  case ESYNC:
    ioctl(fd, CYCL_SETSYNC, &j);
    return j;
  default:
    return -1;
  }
}

int cr_enable(int i) {

  ioctl(fd, CYCL_ENABLE, &i);
  return i;

}

int cr_dumpxmit(void) {

  ioctl(fd, CYCL_DUMPXMIT);
  return 0;

}

int cr_read(cycl_msg *msg) {
  
  int i;
  
  i = read(fd, msg, sizeof(cycl_msg));

  if (i == 0) {
    
    return 0;
  }

  if (i != sizeof(cycl_msg)) {

    return -1;
  }
  
  return sizeof(cycl_msg);

}

int cr_write(unsigned char byte) {
  
  int i;

  
  i = write(fd, &byte, 1);

  if (i != 1) return -1;
  
  return 1;

}



