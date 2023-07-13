#!/bin/sh
# example for copying mon.lite from front-end to back-end
# using scp
/usr/bin/scp -B /mnt/mon.lite yourwebmachine.org:
sleep 30
/usr/bin/scp -B /mnt/mon.lite yourwebmachine.org:
