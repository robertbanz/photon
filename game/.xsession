#!/bin/sh
xterm &
x11vnc -display :0 -forever &
if [ -x /usr/local/bin/olad ]; then
  /usr/local/bin/olad --syslog --daemon
fi
exec /usr/local/bin/fvwm2
