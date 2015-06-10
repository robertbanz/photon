/*
 * $Log: ph_et.c,v $
 * Revision 1.1  1998/11/08 21:59:31  banz
 * Initial revision
 *
 */

static char rcsid[] = "$Id: ph_et.c,v 1.1 1998/11/08 21:59:31 banz Exp $";




#include <tcl8.6/tcl.h>

#include "et.h"
#include "ir.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

static pid_t irpid = 0;

int ph_IrSetup(ClientData, Tcl_Interp *, int, const char **);
int ph_IrSendByte(ClientData, Tcl_Interp *, int, const char **);
int ph_IrEnterPod(ClientData, Tcl_Interp *, int, const char **);
int ph_irstop(ClientData, Tcl_Interp *, int, const char **);

int Ph_et_Init(Tcl_Interp *interp) {
  
  Tcl_CreateCommand(interp, "ir_setup", &ph_IrSetup, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "ir_sendbyte", ph_IrSendByte, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "ir_enterpod", ph_IrEnterPod, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "ir_stop", ph_irstop, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  
  return TCL_OK;
}

void ph_irkillpid(void) {
  if (irpid > 0) kill(irpid, SIGTERM);
}

int ph_IrSetup(ClientData clientData, Tcl_Interp *interp, int argc,
	       const char **argv) {

  if (argc != 2) {
    Tcl_SetResult(interp, "missing a tty file", TCL_STATIC);
    return TCL_ERROR;
  }

  ir_setup(argv[1]);

  atexit(ph_irkillpid);
  
  return TCL_OK;

}

int ph_IrSendByte(ClientData clientData, Tcl_Interp *interp, int argc,
		  const char **argv) {

  static unsigned int b;

  if (argc != 2) {
    Tcl_SetResult(interp, "missing a byte", TCL_STATIC);
    return TCL_ERROR;
  }
  
  if (irpid != 0) {
    ph_irstop(clientData, interp, argc, argv);
  }

  b = atoi(argv[1]);

  if ((irpid = fork()) == 0) {
    while(1) {
      outir((unsigned char) b);
      usleep(833);
    }
  }
    
  return TCL_OK;

}

#define BARKER 0x9d
#define NOTBARKER 0x62

int ph_IrEnterPod(ClientData clientData, Tcl_Interp *interp, int argc,
		  const char **argv) {

  /* need a slot #, team # and game mode */

  int slot, team, mode;
  unsigned char slotno, ircode, modecode;
  unsigned char csum;

  if (argc != 4) {
    Tcl_SetResult(interp, "Wrong # of args", TCL_STATIC);
    return TCL_ERROR;
  }

  if (irpid != 0) {
    ph_irstop(clientData, interp, argc, argv);
  }

  team = atoi(argv[1]);
  slot = atoi(argv[2]);
  mode = atoi(argv[3]);

  et_gencode( team, slot, &ircode, &slotno );

  modecode = mode + 0xc0;

  csum = BARKER ^ NOTBARKER ^ ircode ^ slotno ^ modecode;

  if ((irpid = fork()) == 0) {
    while(1) {

      outir(BARKER);
      outir(NOTBARKER);
      outir(ircode);
      outir(slotno);
      outir(modecode);
      outir(csum);
    }
  }

  return TCL_OK;

}

int ph_irstop(ClientData clientData, Tcl_Interp *interp, int argc,
	      const char **argv) {

  /* narf "irpid" (if it's not 0 */ 
  int s;

  if (irpid == 0) {
    Tcl_SetResult(interp, "no IR process running", TCL_STATIC);
    return TCL_ERROR;
  }

  kill (irpid, SIGTERM);
  
  waitpid(irpid, &s, WNOHANG);
  
  irpid = 0;
  
  return TCL_OK;
}

