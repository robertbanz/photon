INCLUDES=-I/usr/local/include
OBJS=tclmixer.o
CFLAGS=-fPIC $(INCLUDES)
CC=cc $(CFLAGS)
LDFLAGS=-Bshareable

default: tclmixer.so

tclmixer.o: tclmixer.c
	$(CC) $(CFLAGS) -c tclmixer.c


tclmixer.so: $(OBJS)
	ld $(LDFLAGS) -o $@ $(OBJS)
