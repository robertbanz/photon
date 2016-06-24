#!/bin/sh
xterm &
x11vnc -display :0 -forever &
if [ -x /usr/local/bin/olad ]; then
  /usr/local/bin/olad --syslog --daemon
fi
/usr/local/bin/start-pulseaudio-x11
exec /usr/local/bin/fvwm2
