#!/bin/sh
gcc -I../common -lssl -lcrypto -Wall ../common/common.c log.c client.c main.c -o APclientd