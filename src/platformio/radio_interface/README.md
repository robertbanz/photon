# Radio Interface Firmware

This is meant to be built via 'platformio' and uploaded to an Arduino due with the "PhoBoard" interface attached.

## Switch Functionality

### (switch 1) Remote (off) / Local (on)

Remote (off) is the default and should be set when connected to the Photon game software.

If "Local" mode is set, the radio interface will either send Psync or Gsync based upon the Sync switch.

### (switch 2) Gsync (on) / Psync (off)

If the unit is set to "Local" mode, this controls the sync output. Nonfunctional in Remote mode.

### (switch 3) Game (not currently used)

### (switch 4) Slave (on)  / Master (off)

Master (off) is the default setting and is what should normally be used.

Slave (on) causes the unit to behave somewhat like a "pod", and listen on it's radio Rx channel for an
external sync (psync / gsync) and tries to sync with it. Useful for validate the timing against a known-good source.

### (switch 5) Fake (on) / Normal (off)

Fake-mode enables the generation of "random" input data for some player slots. To be used to test the game software.

## Commands

PhoBoard sports a text-based interface. The game software surfaces some of the output (mode changes, warnings, etc)
from the board in the log window. The interface is available on the Due's "programming port" @ 115kbps.

### SETSYNC

Used to set the current sync type to either PSYNC, GSYNC or ESYNC, e.g.

`SETSYNC PSYNC`

### WRITE

Used to add a byte (in decimal) to be transmitted in one of the central radio's transmit slots, e.g.

`WRITE 34`

Note, if the transmit queue is looking full, the firmware will start pruning duplicate entries.

### SENDIR

Used to send a series of repeated bytes via the IR emitter on the board.

`SENDIR 53` will cause the emitter to send the red base code (hex 0x35).

`SENDIR 100 101 102 103 104` will send those bytes in repeating cycle. Can be used to string together, say, an ET
enter sequence.

### STOPIR

Diables the IR output previous triggered with SENDIR

## Data

When data is received or transmitted, you'll receive a string, always prefixed with 905, that looks like this:

905 32 32 rx 53

The first number is a rotating sequence number that is used by the game software to detect a new poll cycle.
The second number is the "slot number"
The third is either "tx" or "rx" -- clearly indicating if this was a byte that was transmitted or received.
The fourth is the actual byte.





