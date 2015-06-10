
/* 
 * $Log: tclmixer.c,v $
 * Revision 1.1  1999/01/31 17:16:45  banz
 * Initial revision
 *
*/

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <tcl8.6/tcl.h>

char *names[SOUND_MIXER_NRDEVICES] = SOUND_DEVICE_NAMES;

int res_name(const char *name, int mask);
void print_recsrc(int recsrc);

typedef struct mixer_t {
  
  int fd; /* file descriptor */
  char *name; /* command name */
  int devmask, recmask, recsrc, orecsrc;
} mixer;

int Tcl_Mixer(ClientData, Tcl_Interp *, int, const char **);
int Tcl_Mixer_Cmd(ClientData, Tcl_Interp *, int, const char **);

int Tclmixer_Init(Tcl_Interp *interp) {

  Tcl_CreateCommand(interp, "mixer", Tcl_Mixer, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

  return TCL_OK;
}

int Tcl_Mixer(ClientData clientData, Tcl_Interp *interp, int argc, 
	      const char **argv ) {
  
  /* arg1 = mixer name
     arg2 = mixer device
     returns mixer name
  */

  mixer *m = malloc(sizeof(mixer));

  if ( argc != 3 ) {
    Tcl_SetResult(interp, "must give a name & device.", TCL_STATIC);
    return TCL_ERROR;
  }

  m->fd = open( argv[2], O_RDWR );

  if (m->fd < 0) {
    Tcl_SetResult(interp, "can't open mixer device", TCL_STATIC);
    return TCL_ERROR;
  }

  ioctl(m->fd, SOUND_MIXER_READ_DEVMASK, &m->devmask);
  ioctl(m->fd, SOUND_MIXER_READ_RECMASK, &m->recmask);
  ioctl(m->fd, SOUND_MIXER_READ_RECSRC, &m->recsrc);
  fcntl(m->fd, F_SETFD, 1);
  m->name = strdup(argv[1]);

  Tcl_CreateCommand(interp, m->name, Tcl_Mixer_Cmd, (ClientData) m, NULL);

  char* c = Tcl_Alloc(strlen(argv[1] + 1));
  strcpy(c, argv[1]);
  Tcl_SetResult(interp, c, TCL_DYNAMIC);

  return TCL_OK;
}

int res_name(const char *name, int mask ) {
  int foo;
  for (foo = 0; foo < SOUND_MIXER_NRDEVICES; foo++ )
    if ((1<<foo) & mask && !strcmp(names[foo], name))
      break;
  
  return foo == SOUND_MIXER_NRDEVICES ? -1 : foo;
}

int Tcl_Mixer_Cmd(ClientData clientData, Tcl_Interp *interp, int argc,
		  const char **argv ) {
  
  mixer *m = (mixer *) clientData;
  char b[1024];
  int foo, bar, baz, dev, l, r;

  /* commands: get, set, list */

  if ( argc < 2 ) {
    Tcl_SetResult(interp,"get, set, or list.", TCL_STATIC);
    return TCL_ERROR;
  }

  if ( !strcmp(argv[1], "list") ) {   
    for ( foo = 0; foo < SOUND_MIXER_NRDEVICES; foo++ ) {
      if ((!(1 << foo) & m->devmask ))
	continue;
      if (ioctl(m->fd, MIXER_READ(foo), &bar) == -1)
	continue;
      Tcl_AppendElement(interp, names[foo]);
    }
    return TCL_OK;
  }

  if ( !strcmp(argv[1], "set") ) {
    if ( argc != 4 ) {
      Tcl_SetResult(interp,"must give device and value to set.", TCL_STATIC);
      return TCL_ERROR;
    }
    if ((dev = res_name(argv[2], m->devmask)) == -1 ) {
      Tcl_SetResult(interp,"not a valid device.", TCL_STATIC);
      return TCL_ERROR;
    }
    
    bar = sscanf(argv[3], "%3d:%3d", &l, &r);

    if ( ( bar != 1 ) && ( bar != 2 ) ) {
      Tcl_SetResult(interp,"invalid input for values.", TCL_STATIC);
      return TCL_ERROR;
    }

    switch (bar) {
    case 1:
      r = l;
    case 2:
      if (l < 0) l = 0;
      else if (l > 100) l = 100;
      if (r < 0) r = 0;
      else if (r > 100) r = 100;
      l |= r << 8;
      ioctl(m->fd, MIXER_WRITE(dev), &l);
      break;
    }
    return TCL_OK;
  }

  if ( !strcmp(argv[1], "get") ) {
    if ( argc < 3 ) {
      Tcl_SetResult(interp,"must give a name.", TCL_STATIC);
      return TCL_ERROR;
    }
    for ( baz = 2; baz < argc; ++baz ) {
      if ((dev = res_name(argv[baz], m->devmask)) == -1 ) {
        Tcl_SetResult(interp,"not a valid device name.", TCL_STATIC);
	return TCL_ERROR;
      }

      ioctl(m->fd, MIXER_READ(dev), &bar);
      sprintf(b, "%3d:%3d", bar & 0x7f, (bar >> 8) & 0x7f);
      Tcl_AppendElement(interp, b);
    }
    return TCL_OK;
  }

  Tcl_SetResult(interp,"invalid command.", TCL_STATIC);
  return TCL_ERROR;
}
    
							  
    


      
