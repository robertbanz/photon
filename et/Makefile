INCLUDES=-I/usr/photon/src/ir -I/usr/local/include
OBJS=et.o ph_et.o ../ir/ir.o 
CFLAGS=-fPIC -O2 $(INCLUDES)
CC=cc $(CFLAGS)
LDFLAGS=-shared -L/usr/photon/lib -L/usr/local/lib

default: ph_et.so

et.o:		et.c
	$(CC) -c et.c

ph_et.o:	ph_et.c
	$(CC) -c ph_et.c

ph_et.so: $(OBJS)
	ld $(LDFLAGS) -o $@ $(OBJS) -lc -ltcl86

