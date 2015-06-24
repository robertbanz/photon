
/*
 * $Log: cr.h,v $
 * Revision 1.1  1998/11/08 21:55:05  banz
 * Initial revision
 *
*/


#include <cycldvr/cycl.h>

#define PSYNC 0xC5
#define ESYNC 0xDD
#define GSYNC 0xCA

int cr_init(void);
int cr_setsync(unsigned char);
int cr_enable(int);
int cr_read(cycl_msg *);
int cr_write(unsigned char);

