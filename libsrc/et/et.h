
/*
 * $Log: et.h,v $
 * Revision 1.1  1998/11/08 21:59:01  banz
 * Initial revision
 *
*/


#define RED_IR_START 0x42
#define GRN_IR_START 0x80

#define RED_RF_START 4
#define GRN_RF_START 30

int et_gencode(int team, int slot, unsigned char *ir, unsigned char *slotno);
