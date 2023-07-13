#!/usr/local/bin/bash
gawk '/RELEASE.[0-9]+/ {print $1 " " $2 " " ++$3;next} /RELEASE_DATE/{printf "%s %s ",$1,$2;"date +%Y-%m-%d" |getline $d;printf "\"%s\"\n",$d;next} {print}' release.h> tmp$$
mv tmp$$ release.h

