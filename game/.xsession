#!/bin/sh
xterm &
x11vnc -display :0 -forever &
if [ -x /usr/local/bin/olad ]; then
  /usr/local/bin/olad --syslog --daemon
fi
# Make sure we set the hardware mixer
# values to a known state.
for device in vol rec pcm mic mix; do
  mixer $x 100:100 || true
done
pulseaudio -D
exec /usr/local/bin/fvwm2
