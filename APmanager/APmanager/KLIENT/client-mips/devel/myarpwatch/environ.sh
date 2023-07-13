#!/bin/sh
rm arpwatch
export PATH=$PATH:/mnt/filmy1/buildroot/staging_dir_mipsel/bin/
mipsel-linux-uclibc-gcc -static -Ilibnet/ -Wall arpfuncs.c arpwatch.c -lpcap -o arpwatch 
ls -la arpwatch
mipsel-linux-uclibc-strip arpwatch

