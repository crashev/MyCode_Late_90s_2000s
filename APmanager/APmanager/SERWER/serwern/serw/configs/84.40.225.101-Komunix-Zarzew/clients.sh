#!/bin/sh
#Package ver 1.0
cat > /etc/ethers << EOF
EOF
wifi;
killall -9 dnsmasq;/etc/init.d/S50dnsmasq;
/etc/init.d/S44firewall;
cat > /etc/init.d/S49QoS <<EOF
#!/bin/sh
 INS="/sbin/insmod" 
\$INS ipt_CONNMARK
\$INS ipt_connbytes
\$INS cls_u32
BW=\`nvram get pawq_lacze_down\`
if [ ! \$BW ]; then
         BW=1024;
fi
DEV=\`nvram get pawq_lacze_dev\`
if [ ! \$DEV ]; then
         DEV="br0";
fi

IPT="/usr/sbin/iptables";
TC="/usr/sbin/tc";
DEV="br0";

\$IPT -t mangle -F PREROUTING
\$IPT -t mangle -F POSTROUTING

\$TC qdisc del dev \$DEV root
\$TC qdisc add dev \$DEV root handle 1:0 htb r2q 1 default 1
\$TC class add dev \$DEV parent 1:0 classid 1:1 htb rate \${BW}kbit burst 20k quantum 60000

\$IPT -t mangle -A PREROUTING -j CONNMARK --restore-mark
\$IPT -t mangle -A PREROUTING -m mark ! --mark 0 -j ACCEPT
\$IPT -t mangle -A PREROUTING -j CONNMARK --save-mark

# Nasz licznik
 a=100;

dodaj() {

if [  "\$3" != "0" ]; then
    \$TC class add dev \$DEV parent 1:1 classid 1:\$a htb rate \${3}kbit prio 0
    \$TC qdisc  add dev \$DEV parent 1:\$a handle \$a:1 sfq perturb 10
    \$TC filter add dev \$DEV protocol ip parent 1:0 prio 0 u32 match ip dst \$1 flowid 1:\$a
    let a=\$a+1
fi
    \$IPT -t mangle -A POSTROUTING -o \$DEV -p tcp -d \$1 -m connbytes --connbytes \$4:999999999 --connbytes-dir both --connbytes-mode bytes -j CONNMARK --set-mark \$a

    \$TC class add dev \$DEV parent 1:1 classid 1:\$a htb rate \${2}kbit prio 0
    \$TC qdisc  add dev \$DEV parent 1:\$a handle \$a:1 sfq perturb 10
    \$TC filter add dev \$DEV protocol ip parent 1:0 prio 0 handle \$a fw flowid 1:\$a

    let a=\$a+1

}

EOF
chmod a+x /etc/init.d/S49QoS
