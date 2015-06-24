#!/bin/sh
xterm &
/usr/photon/bin/main &
x11vnc -display :0 -forever &
exec /usr/local/bin/fvwm2
