#!/bin/sh
/bin/sh mkrelease.sh  release.h
rm APclientd
#export PATH=$PATH:/mnt/filmy1/WRT54/LATEST/openwrt/staging_dir_mipsel/bin/
#mipsel-linux-uclibc-gcc -I/mnt/filmy1/WRT54/LATEST/openwrt/staging_dir_mipsel/usr/include/ -I../common/ -Iinclude/ mywatch.c tevents.c client.c log.c main.c server.c cprotocol.c iptables.c ../common/common.c -lz -lcrypto -lpcap -lssl -shared -o APclientd
export PATH=$PATH:/stuff/CVS/openwrt/staging_dir_mipsel/bin/
mipsel-linux-uclibc-gcc -Wall -O2 -I../common/ -Iinclude/ -I/stuff/CVS/openwrt/staging_dir_mipsel/usr/include/ -o APclientd mywatch.c tevents.c client.c log.c main.c server.c cprotocol.c iptables.c ../common/common.c -L/stuff/CVS/openwrt/staging_dir_mipsel/usr/lib/ -lshared -lssl -lpcap -lz -lcrypto
mipsel-linux-uclibc-strip APclientd
ls -la APclientd
pub APclientd
