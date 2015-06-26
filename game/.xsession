#!/bin/sh
xterm &
x11vnc -display :0 -forever &
/usr/local/bin/start-pulseaudio-x11
exec /usr/local/bin/fvwm2
