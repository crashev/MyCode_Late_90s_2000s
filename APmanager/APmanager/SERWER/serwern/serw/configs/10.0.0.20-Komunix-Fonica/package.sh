#!/bin/sh
#Package ver 1.0
nvram set wl0_wds="00:0e:2e:6e:8b:5d 00:0E:2E:6E:8B:4C 00:0e:2e:6b:96:83 00:0e:2e:6e:8b:63 00:13:10:2f:4d:9f"
nvram commit
wifi && /etc/init.d/S44firewall && /etc/init.d/S50upload