#!/bin/sh
#Package ver 1.0
cat > /etc/ethers << EOF
00:0E:35:CE:07:A9,id:*	192.168.100.100
00:0d:9d:5e:ef:49,id:*	192.168.100.101
EOF
wifi;
killall -9 dnsmasq;/etc/init.d/S50dnsmasq;
/etc/init.d/S44firewall;

