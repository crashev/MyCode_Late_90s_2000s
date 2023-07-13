#!/bin/sh
#
# Quality Of Service - Pawel Furtak <pawel@furtak.com.pl>
#

INS="/sbin/insmod"

$INS ipt_CONNMARK>/dev/null 2>&1
$INS ipt_connbytes>/dev/null 2>&1
$INS cls_u32>/dev/null 2>&1

IPT="/usr/sbin/iptables";
TC="/usr/sbin/tc";
DEV="br0";

#
# interfejs sieciowy odpowiedzialny za upload
#
DEV_UP="vlan1"
#
# Zerujemy IPTABLES i kolejki
#
$IPT -t mangle -F PREROUTING
$IPT -t mangle -F POSTROUTING

$TC qdisc del dev $DEV root
$TC qdisc add dev $DEV root handle 1:0 htb r2q 1 default 1
$TC class add dev $DEV parent 1:0 classid 1:1 htb rate 100mbit burst 20k quantum 60000

$TC qdisc del dev $DEV_UP root
$TC qdisc add dev $DEV_UP root handle 1:0 htb r2q 2 default 0
$TC class add dev $DEV_UP parent 1:0 classid 1:1 htb rate 100mbit burst 20k quantum 1500

$IPT -t mangle -A PREROUTING -j CONNMARK --restore-mark
$IPT -t mangle -A PREROUTING -m mark ! --mark 0 -j ACCEPT
$IPT -t mangle -A PREROUTING -j CONNMARK --save-mark

# Nasz licznik
a=100;
dodaj() {

if [  "$2" != "0" ]; then
    $TC class add dev $DEV parent 1:1 classid 1:$a htb rate ${2}kbit prio 0
    $TC qdisc  add dev $DEV parent 1:$a handle $a:1 sfq perturb 10
    $TC filter add dev $DEV protocol ip parent 1:0 prio 0 u32 match ip dst $1 flowid 1:$a
fi
# upload
if [ "$3" != "0" ]; then
    $TC class add  dev $DEV_UP parent 1:1 classid 1:$a htb rate ${3}kbit burst 6k quantum 1500
    $TC filter add dev $DEV_UP protocol ip preference 1 parent 1:0 handle $a fw flowid 1:$a
    $IPT -t mangle -I POSTROUTING -s $1 -j MARK --set-mark $a
fi
    let a=$a+1
}



dodaj 192.168.1.100 512 56 # przytnij adres 192.168.0.100 - 512kbps download/128kbps upload

$TC filter add dev $DEV protocol ip parent 1:0 prio 0 u32 match ip src 192.168.0.1 flowid 1:0
