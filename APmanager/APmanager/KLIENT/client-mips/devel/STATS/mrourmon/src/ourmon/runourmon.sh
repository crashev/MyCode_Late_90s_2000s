#!/bin/sh


#!/bin/sh
# 32k 32768
# 10mbyte 10485760
# 64k 65536
# 128k 131072 
# 256k 262144
# 512k 524288
# 1024 1048576
# 2mbyte 2097152
# 3mbyte 3145728
# 4mbyte 4194304
  
#BSIZE=32768
#BSIZE=65536
#BSIZE=131072
#BSIZE=262144
#BSIZE=524288

#BSIZE=1048576
#BSIZE=2097152
#BSIZE=3145728
#BSIZE=4194304
#BSIZE=5242880
#BSIZE=6291456
#BSIZE=7340032
BSIZE=8388608
#BSIZE=10485760
#BSIZE=680000
sysctl -w debug.bpf_bufsize=$BSIZE
sysctl -w debug.bpf_maxbufsize=$BSIZE
#/usr/home/mrourmon/bin/ourmon -a 30 -i em0 -f /etc/ourmon.conf -m /home/mrourmon/web.pages/mon.lite &
/usr/home/mrourmon/bin/ourmon -a 30 -i em0 -f /etc/ourmon.conf -m /tmp/mon.lite &
