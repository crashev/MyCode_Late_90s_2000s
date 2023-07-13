#!/usr/bin/perl
#
# use ps to see if ourmon is there and restart 
# if not.
#
# run from crontab.
#
require "ctime.pl";

$debug = 0;

$path = "/usr/home/mrourmon/bin/ourmon";
$script="/home/mrourmon/bin/runourmon.sh";

open(CHECK, "ps -ax | grep ourmon | ");
$found = 0;
while(<CHECK>) {
    chop;
    ($f1, $f2, $f3, $f4, $f5, $f6) = split;
    if ($debug) {
    	print "$f1 $f2 $f3 $f4 $f5 $f6\n";
    }
    if ($f5 eq "$path") {
# 	print "f5:$f5\n";
        $found = 1;
        if ($debug) {
		print "found ourmon\n";
	}
    }
    if ($f6 eq "$path") {
        $found = 1;
# 	print "f6:$f6\n";
        if ($debug) {
		print "found ourmon\n";
	}
    }
}
close(CHECK);

#   If ourmon not alive then start it

#print $script;
if ($found == 0) {
     if ($debug) {
		print "calling system with $script\n";
     }
     system(`$script`);
     print "had to reboot ourmon\n"; 
#    system("echo oops | mail -s ourmon_reboot somebody\@somewhere.com");
}
else {
     if ($debug) {
     	print "did not reboot ourmon\n"
     }
}
