#!/bin/sh
#Package ver 1.0
rm /etc/dnsmasq.conf
cat >/etc/dnsmasq.conf << EOF
# filter what we send upstream
domain-needed
bogus-priv
filterwin2k

# allow /etc/hosts and dhcp lookups via *.lan
local=/lan/
domain=lan
user=root
# no dhcp / dns queries from the wan
except-interface=vlan1

# enable dhcp (start,end,netmask,leasetime)
dhcp-authoritative
dhcp-range=192.168.1.100,192.168.1.250,255.255.255.0,12h
dhcp-leasefile=/tmp/dhcp.leases
dhcp-option=3,192.168.1.1
dhcp-option=6,192.168.1.1,83.145.128.1

read-ethers
EOF
nvram set wl0_net_mode=mixed
nvram set wl0_frameburst=off
nvram set wl0_infra=1
nvram set wl0_ifname=eth1
nvram set wl0_mode=ap
nvram set wl0_ap_isolate=1
nvram set wl0_gmode=1
nvram set wl0_dtim=1
nvram set wl0_ssid=PawqLinksysRouterv2
# przykladowy wep klucz - tu pawq1 w hexach
nvram set wl0_key1=70617771317061777131313131
nvram set wl0id=0x4320
nvram set wl0_key2=
nvram set wl0_key3=
nvram set wl0_key4=
nvram set wl0_plcphdr=long
nvram set wl0_rate=0
nvram set wl0_closed=0
nvram set wl0_macmode=allow
nvram set wl0_radioids=BCM2050
nvram set wl0_phytype=g
nvram set wl0gpio2=0
nvram set wl0_lazywds=0
nvram set wl0gpio3=0
nvram set wl0_afterburner=off
nvram set wl0_antdiv=-1
nvram set wl0_wpa_psk=
nvram set wl0_unit=0
nvram set wl0_wds=
nvram set wl0_radius_port=1812
nvram set wl0_auth=0
nvram set wl0_radius_ipaddr=
nvram set wl0_phytypes=g
nvram set wl0_frag=2346
nvram set wl0_wep=enabled
nvram set wl0_country=
nvram set wl0_rateset=default
nvram set wl0_rts=2347
nvram set wl0_wpa_gtk_rekey=3600
nvram set wl0_key=1
nvram set wl0_radio=1
nvram set wl0_bcn=100
nvram set wl0_gmode_protection=auto
nvram set wl0_maclist=
nvram set wl0_radius_key=
nvram set wl0_corerev=7
nvram set wl0_channel=11
nvram set wl0_auth_mode=open
nvram set wl0_proto=static
nvram set wl0_crypto=tkip
#ustawienia regionu - wazne bo Europa uzywa kanalow 1-13/14 a usa tylko 11
nvram set wl0_country_code=PL
nvram set wl_country_code=PL
nvram set wl_country=PL
#zeby mi vlan1 nie dodawal przypadkiem do bridga
nvram set lan_ifnames="vlan0 eth1 eth2 eth3"
# ustawienia ograniczenia i priorytetow UPLOADu
nvram set pawq_qos_up=120
nvram set pawq_qos_if=vlan1
#lan ipaddr
nvram set lan_ipaddr=192.168.1.1
nvram set lan_proto=static
nvram set lan_netmask=255.255.255.0
nvram unset wl0_proto
nvram unset wl0_ifname
nvram unset wifi_ifname
nvram unset wifi_proto
nvram unset lan_ifname
nvram commit
wifi
reboot
# zawsze po wifi restartuj firewalla bo to on dodaje MACi do Access w WIFI
/etc/init.d/S44firewall
nvram set pawq_lacze_down=20480

