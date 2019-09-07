# Photon Game Platform

This is a software (and hardware) suite to run the Photon game system on reasonably modern hardware.

The software is known to run on FreeBSD, though there is nothing that would stop it from running cleanly on any modern Unix with the appropriate dependencies installed. While previous revisions involved some C modules, everything now is just raw Tcl/Tk.

TL;DR Check this out in /usr/photon, and you're good to go. (will get to the specifics later)

system_build/game contains some instructions that were used to build the "game computer" we have used at PhoCon. This consists of a Mac-based host (that runs the lighting control software natively), and two virtual machines. One that runs pfsense and acts as a router to the private network that runs the Photon bits, and a second VM that runs the game software.

## Hardware

Previous revisions of this software -- you can check back in the commit history -- were expected to run on PC hardware (a Pentium at the time) on FreeBSD 2.x. It used a kernel module that modified the low-level timer handling to match what the Photon player units (PODs) expect. (see the README in the photon_gamma repository for more of that story)

As PC environment has evolved, the ability to match the Photon timing requirements became difficult. As part of making sure the software could move on and work on platforms beyond that could be picked up at computer swap-meets, some hardware assistance was required. The first attempt was to use some Arduino hardware, but a bare Arduino was not capable of reliably matching the POD's timing. I developed what I called the "PhoBoard" (schematics and layout in Eagle format are under hardware/) that provides an accurate timing interrupt in addition to an RS232-level serial port to connect to the central radio, and a modulated IR output when using the game computer itself as the entry terminal for the PODs.

The firmware for the PhoBoard(tm) is located in src/platformio/radio_interface. It is expected to run on an Arduino DUE, and be connected to the game computer (or game computer VM) via its programming port.

## Software

There are four primary components to the software. You'll find a lot of support scripts sitting around in bin/ to manage them, but this is what's important.

*  *bin/main* - It's called main for a reason. Listens on some ports for the ET & Displays to talk to, talks to the central radio via the Arduino, plays the game soundtrack, and has some fun menus to play with.
*  *bin/et* - The entry terminal. Talks to "main" to control game state. Can either use the IR emitter that *main* controls via the Arduino, or, to something connected to a local serial port that can provide a modulated IR output.
*  *bin/{cg,pg}display* - These drive the "Current Game" display, or "Post Game" display. At PhoCon we've ran these on Intel ComputeSticks running Ubuntu. They just need to be able to connect to the game computer and database to do their thing. They get constant updates about the state of the game, and in the case of the current game display, information about player state changes from *main* as they happen.  These have an "-hd" argument that selects for the 1080p layout, vs the old NTSC resolution screen layouts. (at PhoCon, all the displays are "hd")
*  *database* - The PostgreSQL database handles all of the state. For example, when the ET stages a game, it writes the data there, and just tells *main* to "start game number x". Similarly, when the displays need information about the game (or previous game), they query the DB.

### Running It In Production

*  The scripts that drive the ET & displays run them in a loop that upon startup will try to 'rsync' the entire /usr/photon tree from the game computer so they're running the latest revision of code.

*  As we're typically running this in a VM, if you've set up the FreeBSD image as I have, it launches the X windows server in a virtual frame buffer that's available via a VNC connection.

*  It also sets up PulseAudio (dot dot dot Total Junk dot dot dot SO much latency. So bad) to serve up the default audio input as the "background music" for the game, which will be faded out when the game starts.

## Configuration

etc/game.conf and etc/game.conf.local specify site specific and machine specific (respectively) parameters for system functionality. For example, the game.conf.local will specify a serial tty for the ET's IR output, while game.conf.local on the game computer points to the IR on the PhoBoard. game.conf is synced during the 'rsync' mentioed above, but .local is excluded.

## Having Fun

### Game Modes

The game modes available are defined in the database table g_modes, and make reference to actual code in lib/modes which define how various game events are handled. All current game modes use 'standard' but twiddle various parameters, but it's possible to change all the behaviors you want.

### Game Soundtracks

Similar to the game modes, the soundtracks and their lengths, etc, are defined in the g_tracks database table.

### Display Defaults

In lib/, the \*.defaults files -- in X Resources format -- allow tweaking of various fonts and layout parameters. The cg_idle sub directories contain custom tasks to display in-beween games. Perfect for advertising.

