#!/bin/sh
gcc -static -lpcap arpfuncs.c arpwatch.c linux_arp.c /usr/lib/libpcap.a -o arpwatch
