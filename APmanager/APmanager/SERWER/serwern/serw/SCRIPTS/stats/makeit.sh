#!/bin/sh
# Interfejsy
for i in `(ls *-traffic.rrd)`; do
    rrdtool graph `echo $i|awk -F'-' '{print $1}'`-1.png --start -86400 DEF:inoctets=$i:input:AVERAGE DEF:outoctets=$i:output:AVERAGE AREA:inoctets#00FF00:"In traffic" LINE1:outoctets#0000FF:"Out traffic"    
    rrdtool graph `echo $i|awk -F'-' '{print $1}'`-2.png -h 80 -w 600 DEF:in=$i:input:AVERAGE DEF:out=$i:output:AVERAGE "CDEF:out_neg=out,-1,*" "AREA:in#32CD32:Incoming" "LINE1:in#336600" "GPRINT:in:MAX:  Max\\: %5.1lf %s" "GPRINT:in:AVERAGE: Avg\\: %5.1lf %S" "GPRINT:in:LAST: Current\\: %5.1lf %Sbytes/sec\\n" "AREA:out_neg#4169E1:Outgoing" "LINE1:out_neg#0033CC" "GPRINT:out:MAX:  Max\\: %5.1lf %S" "GPRINT:out:AVERAGE: Avg\\: %5.1lf %S" "GPRINT:out:LAST: Current\\: %5.1lf %Sbytes/sec" "HRULE:0#000000"
    rrdtool graph `echo $i|awk -F'-' '{print $1}'`-3.png -b 1024 --start -7200 DEF:inoctets=$i:input:AVERAGE CDEF:realspeed=inoctets DEF:outoctets=$i:output:AVERAGE AREA:inoctets#00FF00:"In traffic" LINE1:outoctets#0000FF:"Out traffic" LINE2:realspeed#FF0000
done


