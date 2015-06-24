#include <sys/types.h>
#include <sys/uio.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/fcntl.h>
#include "ir.h"

static int fd;

#ifndef TIOCGETA
#define TIOCGETA TCGETS
#define TIOCSETA TCSETS
#endif

ssize_t outir(const unsigned char outbyte) {

  static int i;
  static unsigned char buf[128];
  ssize_t j;

#ifdef TIOCDRAIN
  ioctl(fd, TIOCDRAIN);
#else
  ioctl(fd, TCFLSH, 1 );
#endif

  j = write(fd, &outbyte, 1);

  return j;

}

void ir_setup(const char *file) {

  struct termios t;
  unsigned char i;

  fd = open (file, O_WRONLY);

  ioctl(fd, TIOCGETA, &t);

  t.c_oflag = 0;

  /*cfsetspeed( &t, B1200);*/
  cfsetispeed(&t, B1200);
  cfsetospeed(&t, B1200);

  ioctl(fd, TIOCSETA, &t);

}

