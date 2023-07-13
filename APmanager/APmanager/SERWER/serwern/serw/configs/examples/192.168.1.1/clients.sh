#!/bin/sh
#Package ver 1.0
cat > /etc/ethers << EOF
00:D0:59:44:6B:56,id:*	192.168.1.101
00:0B:2B:09:5A:E0,id:*	192.168.1.197
00:C0:49:A7:C0:D8,id:*	192.168.1.102
EOF
wifi;
killall -9 dnsmasq;/etc/init.d/S50dnsmasq;
/etc/init.d/S44firewall;
tc qdisc del dev eth1 root
tc -s qdisc ls dev eth1

