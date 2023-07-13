#!/bin/sh
#Package ver 1.0
cat > /etc/ethers << EOF
00:0F:B0:46:99:62,id:*	192.168.100.100
EOF
wifi;
killall -9 dnsmasq;/etc/init.d/S50dnsmasq;
/etc/init.d/S44firewall;
