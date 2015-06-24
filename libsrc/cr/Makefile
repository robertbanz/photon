INCLUDES=-I/usr/photondev/src -I/usr/photon/`cat /var/photon/.arch`/include
OBJS=cr.o ph_cr.o
CFLAGS=-O2 $(INCLUDES)
CC=cc $(CFLAGS)
LDFLAGS=-Bshareable

default: ph_cr.so

cr.o: cr.c
	$(CC) $(CFLAGS) -c cr.c

ph_cr.o: ph_cr.c
	$(CC) $(CFLAGS) -c ph_cr.c

ph_cr.so: $(OBJS)
	ld $(LDFLAGS) -o $@ $(OBJS)
