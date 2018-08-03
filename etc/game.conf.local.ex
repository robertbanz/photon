# local machine game configuration file
# vim: ft=tcl:

# The ET should set these to the hostname of the game computer.
set server_name localhost
set dbhost localhost

# Have the ET use the IR emitter on the central
# radio interface
set ettty GAME
# Have the ET use a serial device
# set ettty /dev/cuaU0

# Audio mixer hardware interface
set mixer_device /dev/mixer0

set cr_tty /dev/photon_radio_interface

# set phasertty /dev/photon_phaser_interface
