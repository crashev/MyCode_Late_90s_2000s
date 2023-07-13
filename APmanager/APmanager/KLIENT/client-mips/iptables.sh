#!/bin/sh
#
# Skrypt ustawia znakowanie pakietow dla celow statystycznych
#
iptables -t mangle -F FORWARD

iptables -t mangle -A FORWARD -p tcp --sport 20 -j MARK --set-mark 0x60
iptables -t mangle -A FORWARD -p tcp --dport 20 -j MARK --set-mark 0x61
iptables -t mangle -A FORWARD -p tcp --sport 21 -j MARK --set-mark 0x62
iptables -t mangle -A FORWARD -p tcp --dport 21 -j MARK --set-mark 0x63
iptables -t mangle -A FORWARD -p tcp --sport 22 -j MARK --set-mark 0x64
iptables -t mangle -A FORWARD -p tcp --dport 22 -j MARK --set-mark 0x65
iptables -t mangle -A FORWARD -p tcp --sport 25 -j MARK --set-mark 0x66
iptables -t mangle -A FORWARD -p tcp --dport 25 -j MARK --set-mark 0x67
iptables -t mangle -A FORWARD -p tcp --sport 80 -j MARK --set-mark 0x68
iptables -t mangle -A FORWARD -p tcp --dport 80 -j MARK --set-mark 0x69
iptables -t mangle -A FORWARD -p tcp --sport 110 -j MARK --set-mark 0x70
iptables -t mangle -A FORWARD -p tcp --dport 110 -j MARK --set-mark 0x71
iptables -t mangle -A FORWARD -p tcp --sport 119 -j MARK --set-mark 0x72
iptables -t mangle -A FORWARD -p tcp --dport 119 -j MARK --set-mark 0x73
iptables -t mangle -A FORWARD -p tcp --sport 143 -j MARK --set-mark 0x74
iptables -t mangle -A FORWARD -p tcp --dport 143 -j MARK --set-mark 0x75
iptables -t mangle -A FORWARD -p tcp --sport 443 -j MARK --set-mark 0x76
iptables -t mangle -A FORWARD -p tcp --dport 443 -j MARK --set-mark 0x77

