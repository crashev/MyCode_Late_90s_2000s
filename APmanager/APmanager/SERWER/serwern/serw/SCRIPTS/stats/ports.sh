#!/bin/sh

# Porty - 80 dla testu
    rrdtool graph ports/port80-1.png --start -86400 DEF:inoctets=ports20-995-iptraffic.rrd:80_in:AVERAGE DEF:outoctets=ports20-995-iptraffic.rrd:80_out:AVERAGE AREA:inoctets#00FF00:"In traffic" LINE1:outoctets#0000FF:"Out traffic"    
    rrdtool graph ports/port80-2.png -h 80 -w 600 DEF:in=ports20-995-iptraffic.rrd:80_in:AVERAGE DEF:out=ports20-995-iptraffic.rrd:80_out:AVERAGE "CDEF:out_neg=out,-1,*" "AREA:in#32CD32:Incoming" "LINE1:in#336600" "GPRINT:in:MAX:  Max\\: %5.1lf %s" "GPRINT:in:AVERAGE: Avg\\: %5.1lf %S" "GPRINT:in:LAST: Current\\: %5.1lf %Sbytes/sec\\n" "AREA:out_neg#4169E1:Outgoing" "LINE1:out_neg#0033CC" "GPRINT:out:MAX:  Max\\: %5.1lf %S" "GPRINT:out:AVERAGE: Avg\\: %5.1lf %S" "GPRINT:out:LAST: Current\\: %5.1lf %Sbytes/sec" "HRULE:0#000000"
    rrdtool graph ports/port80-3.png -b 1024 --start -7200 DEF:inoctets=ports20-995-iptraffic.rrd:80_in:AVERAGE CDEF:realspeed=inoctets DEF:outoctets=ports20-995-iptraffic.rrd:80_out:AVERAGE AREA:inoctets#00FF00:"In traffic" LINE1:outoctets#0000FF:"Out traffic" LINE2:realspeed#FF0000


