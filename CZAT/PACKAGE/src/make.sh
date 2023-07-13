#!/bin/sh

echo "Making czat - Compiling !!!"
javac Chat.java;
zip czat *.class
jar cvfm czat.jar *.class
rm *.class;
#cp czat.zip /home/httpd/html/t/

