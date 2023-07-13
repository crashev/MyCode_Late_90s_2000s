#!/bin/sh
lynx -source http://czateria.interia.pl/applet3/applet.html?rid=10 > out
cat out | grep public_chanels_enc | awk -F'"' '{print $4}' > out2
rm out
./base64-decoder out2 out
./make out kanaly.h
rm out
rm out2


