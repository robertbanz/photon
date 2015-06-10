
/*
 * $Log: ph_cr.c,v $
 * Revision 1.3  1999/02/09 17:37:04  banz
 * updated read code to handle mistakes better!
 *
 * Revision 1.2  1999/01/31 17:02:26  banz
 * binary fconfigure
 *
 * Revision 1.1  1998/11/08 21:54:22  banz
 * Initial revision
 *
*/

static char rcsid[] = "$Id: ph_cr.c,v 1.3 1999/02/09 17:37:04 banz Exp $";



#include <tcl.h>

#include <cr/cr.h>

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int ph_crinit(ClientData, Tcl_Interp *, int, char **);
int ph_crsetsync(ClientData, Tcl_Interp *, int, char **);
int ph_crread(ClientData, Tcl_Interp *, int, char **);
int ph_crwrite(ClientData, Tcl_Interp *, int, char **);
int ph_crenable(ClientData, Tcl_Interp *, int, char **);

static Tcl_Channel cr_channel;
static Tcl_Interp *cr_channel_interp;

int Ph_cr_Init(Tcl_Interp *interp) {
  
  Tcl_CreateCommand(interp, "cr_init", ph_crinit, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "cr_setsync", ph_crsetsync, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "cr_read", ph_crread, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "cr_write", ph_crwrite, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "cr_enable", ph_crenable, (ClientData) NULL,
		    (Tcl_CmdDeleteProc *) NULL);
  
  return TCL_OK;
}

/* this invokes "cr_handler" every time data comes in on the radio */
/* it's cr_handler's job to actually read it -- we don't do that */
/* for you <sigh> */

void ph_radiodata(ClientData, int);

void ph_radiodata(ClientData clientData, int mask) {

  if ( mask != TCL_READABLE ) {
    exit(-1);
  }

  Tcl_CreateChannelHandler(cr_channel, 0, ph_radiodata, NULL);
  Tcl_GlobalEval(cr_channel_interp, "cr_handler\n");
  Tcl_CreateChannelHandler(cr_channel, TCL_READABLE, ph_radiodata, NULL);
}

int ph_crenable(ClientData clientData, Tcl_Interp *interp, int argc,
		char **argv) {

  int a;

  if (argc != 2) {
    interp->result = "should I do it or not?";
    return TCL_ERROR;
  }

  a = atoi(argv[1]);

  cr_enable(a);
  
  return TCL_OK;
}

int ph_crinit(ClientData clientData, Tcl_Interp *interp, int argc, 
	      char **argv) {

  int fd;

  if ((fd = cr_init()) == -1) {
    interp->result = "CR Init Failed.";
    return TCL_ERROR;
  }

  /* yippie skippie do, time to add a file descriptor to... */

  cr_channel = Tcl_MakeFileChannel((ClientData) fd, TCL_READABLE|TCL_WRITABLE);
  
  cr_channel_interp = interp;
  Tcl_RegisterChannel(interp, cr_channel);
  Tcl_CreateChannelHandler(cr_channel, TCL_READABLE, ph_radiodata, (ClientData) NULL);
  Tcl_SetChannelOption(interp, cr_channel, "-buffering", "none");
  Tcl_SetChannelOption(interp, cr_channel, "-translation", "binary");

  return TCL_OK;

}

int ph_crsetsync(ClientData clientData, Tcl_Interp *interp, int argc,
		 char **argv) {

  int i;

  if (argc != 2) {
    interp->result = "missing sync code";
    return TCL_ERROR;
  }

  switch (argv[1][0]) {
    
  case 'P':
  case 'p':
    i = cr_setsync(PSYNC);
    break;
  case 'G':
  case 'g':
    i = cr_setsync(GSYNC);
    break;
  case 'E':
  case 'e':
    i = cr_setsync(ESYNC);
    break;
  default:
    interp->result = "Invalid sync.";
    return TCL_ERROR;
  }

  return TCL_OK;
}

int ph_crdumpxmit(ClientData clientData, Tcl_Interp *interp, int argc,
		  char **argv) {

  cr_dumpxmit();

}


int ph_crread(ClientData clientData, Tcl_Interp *interp, int argc, 
	      char **argv) {

  int i;
  cycl_msg msg;
  static char buf[1024];

  bzero(&msg, sizeof(cycl_msg));

  i = Tcl_Read(cr_channel, (char *) &msg, sizeof(msg));

  if ( i == -1 ) {
    interp->result = "0 0 xx 0";
    return TCL_OK;
  }

  sprintf(buf, "%d %d %s %d",
	    (int) msg.serial,
	    (int) msg.slot,
	    (msg.rcvd == RX) ? "rx" : "tx",
	    (int) msg.data);
    
  interp->result = buf;
  
  return TCL_OK;
  
}

  
int ph_crwrite(ClientData clientData, Tcl_Interp *interp, int argc,
	       char **argv) {
  
  int i;

  if (argc != 2) {
    interp->result = "missing tx data";
    return TCL_ERROR;
  }

  sscanf(argv[1], "%d", &i);

  cr_write((unsigned char) i);

  return TCL_OK;

}

