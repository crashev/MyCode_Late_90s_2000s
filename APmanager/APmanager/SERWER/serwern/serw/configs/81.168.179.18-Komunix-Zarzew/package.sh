#!/bin/sh
#Package ver 1.0
nvram set wl0_wds="00:0F:B0:43:18:92 00:0e:2e:6b:61:4d"
nvram commit
wifi && /etc/init.d/S44firewall

