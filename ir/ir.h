#ifndef _IR_H

/* modulate a bit stream:
   takes 
   1 or 0 (what to modulate)
   offset (in bits) into unsigned char *
   unsigned char * (where to put it)

   returns new offset into unsinged char * (in bits, of course)
   */

static int modulate(int, int, unsigned char *);

/* modulate a byte in a stream:
   takes:
   a byte
   a buffer to put it in

   returns the length of the stream.
   */

int mod_byte(unsigned char, unsigned char *);

/* modulate a byte in a stream, and write it to a file
   descriptor (with appropriate timing, etc */

ssize_t outir(const unsigned char);

void ir_setup(const char *);

#endif
