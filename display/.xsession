#!/bin/sh
xrandr --output Virtual1 --mode 1280x768
xset s off
xset -dpms
hostname=`hostname`
if echo $hostname | grep 'pgdisplay'; then
  exec wish8.6 /usr/photon/bin/pgdisplay -- -h
else
  exec wish8.6 /usr/photon/bin/cgdisplay -- -h
fi
