#!/bin/sh
#Package ver 1.0
nvram set wl0_net_mode=mixed
nvram set wl0_frameburst=off
nvram set wl0_infra=1
nvram set wl0_ifname=eth1
nvram set wl0_mode=ap
nvram set wl0_ap_isolate=1
nvram set wl0_gmode=1
nvram set wl0_dtim=1
nvram set wl0_ssid=PawqLinksysRouter
nvram set wl0_key1=7061777131
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
nvram set wl0_country_code=PL
nvram set wl_country_code=PL
nvram set wl_country=PL
nvram commit
wifi
#reboot
/etc/init.d/S44firewall
