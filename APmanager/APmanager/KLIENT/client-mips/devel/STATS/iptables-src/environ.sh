#!/bin/sh
/bin/sh mkrelease.sh  release.h
rm APclientd
export PATH=$PATH:/mnt/filmy1/buildroot/staging_dir_mipsel/bin/
mipsel-linux-uclibc-gcc -I../common/ -lz -lcrypto -lssl tevents.c client.c log.c main.c server.c cprotocol.c iptables.c ../common/common.c -o APclientd
#strip APclientd
ls -la APclientd
pub APclientd
mipsel-linux-uclibc-strip APclientd
