
/*
 * $Log: et.c,v $
 * Revision 1.1  1998/11/08 21:58:39  banz
 * Initial revision
 *
 */

static char rcsid[] = "$Id: et.c,v 1.1 1998/11/08 21:58:39 banz Exp $";

#include "et.h"

int et_gencode(int team, int slot, unsigned char *ir, unsigned char *slotno) {
  
  /* we take team & slot, and leave a valid IR code & slot # for that player */

  /* quick -- bounds check */
  if ((team < 0) || (team > 1))
    return -1;

  if ((slot < 0) || (slot > 19))
    return -1;

  if ( team == 0 ) /* red */ {
    *ir = RED_IR_START + slot;
    *slotno = RED_RF_START + slot;
  } else {
    *ir = GRN_IR_START + slot;
    *slotno = GRN_RF_START + slot;
    
    if (*ir == 0x80)
      *ir = 0x94;
  }
  return 0;
}
  
