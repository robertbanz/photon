#!/bin/sh
xset s off
xset -dpms
hostname=`hostname`
if echo $hostname | grep 'pgdisplay'; then
  exec wish8.6 /usr/photon/bin/pgdisplay -- -h
else
  exec wish8.6 /usr/photon/bin/cgdisplay -- -h
fi
