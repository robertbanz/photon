CC = gcc -O
LIBS = -lpigpio
CFLAGS = -I. -I../raspi -I../common
SRCS_CPP = pod.cc
OBJS = $(SRCS_CPP:.c=.o)

pod: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
