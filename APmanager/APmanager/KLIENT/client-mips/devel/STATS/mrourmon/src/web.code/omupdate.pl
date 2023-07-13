#! /usr/bin/perl

#
# omupdate.pl - update rrds and create graphs
#
# For syntax:
#	omupdate.pl -h
#	Or see printUsageAndExit below
#
# For defaults (variable values):
#	omupdate.pl -z
#
# input file:
#       mon.lite  - file which is output of ourmon front-end binary
#       	note: mon.lite specifies individual filters by name
#           
# outputs:       
#       will create rrd file for a given filter if it does not
#               exist
#       will update rrd file and create graphics as appropriate
#	will update ourmon "syslog" like log files if logging is on.
#
# dependencies:
#	rrdtool must be installed.
#	drawtopn program must be installed.
#                
# note:     
#       30 second time delta is assumed throughout
#           
# IMPORTANT: you must update certain default config variables
#	to be correct before running this script.
# OR
#	make sure you have a correct script file with
#	runtime params set correctly. -m, etc.  See the
#	usage message.

# set rrdtool perl lib path if not globally available
# comment this out if globally available
use lib qw( /usr/local/rrdtool-1.0.13/lib/perl ../lib/perl );

# use rrdtool perl lib
use RRDs;

##########################################################
# default configuration variables and values

# base directory where rrds live 
my $rrdbase = "/home/mrourmon/rrddata";

# log directory.  we make the assumption here
# that this directory EXISTS and that we have
# one week worth of log backups in it with each
# day a named directory in UNIX date style,
# Sun, Mon, Tue, etc.
# the above is made by the configure script 
my $logbase = "/usr/home/mrourmon/logs";

# set mon.lite input file - this is where it is found
my $datafile = "/usr/home/mrourmon/tmp/mon.lite";

# set base directory for web page output, web files made here
#my $webpath = "/usr/home/mrourmon/web.pages";
my $webpath = "/usr/local/apache/htdocs/ourmon";

# path of C topn program named drawtopn
my $topnprog = "/usr/home/mrourmon/bin/drawtopn";

# leave this alone
# on Linux, drawtopn is not well-behaved for doing dns in legends.
# best if we move that behavior to omupdate.pl in the next release
# anyway
#
my $dns_lookup = 0;    # 2 is dns on in legends, 0 is off, 1 is not a good idea.

# ourmon syslog style logging defaults to on.
#	
my $sysname;
my $logging="on";		# logging to ourmon-style syslog is on

# There is a very primitive capability for one kind of 
# alert (IP flow too big).
#
# if logging is enabled, we "send" an alert to the alert log file
# for any flows greater than the following:
# sending an alert at the moment simply means logging an entry.
# This is only used with the topn_ip_update function (IP, not TCP
# and not UDP).  It is a hack.
#
# maxalertsize set to 10mbits
my $maxalertsize = 10000000;

my $topndebug = 0;

# now process command line arguments
# which may change the above configuration variables
processCmdArgs();

if ($logging eq "on") {
	$logging = 1;
}
else {
	$logging = 0;
}

#dumpArgs();
#exit 0;

###################################################
# everything below is NOT configurable

# put time log in same directory as above
my $timelog = "$webpath/time.log";

# store current time time.log file
open(tfd, ">$timelog") || die "cannot open $timelog: $!\n";
$start_time=`date`;
$start_time=~ s/\n//g;
print tfd "$start_time\n";
close(tfd);

# cp datafile to webpath so we can find it
system("cp $datafile $webpath");

# cd to webpath where we create files

chdir($webpath);

#  set rrd file names, error log file name, top N png file
#  name.  rrds and error log must have rrdbase as path
######################################################

my $pkts=$rrdbase . "\/" . "pkts.rrd";
my $ipproto=$rrdbase . "\/" . "fixed_ipproto.rrd";
my $tcp3=$rrdbase . "\/" . "fixed_tcp3.rrd";
my $iprange1=$rrdbase . "\/" . "fixed_iprange1.rrd";
my $cast=$rrdbase . "\/" . "fixed_cast.rrd";
my $size=$rrdbase . "\/" . "fixed_size.rrd";
my $errlog=$rrdbase . "\/" . "ourmon.log";
my $flowcount=$rrdbase . "\/" . "flowcount.rrd";
my $twormcount=$rrdbase . "\/" . "twormcount.rrd";
my $irccount=$rrdbase . "\/" . "irc.rrd";
my $topnstat=$rrdbase . "\/" . "topnstat.rrd";

$ip_flowcount = $udp_flowcount = $tcp_flowcount = $icmp_flowcount = 0;
$topn_exists = 0; 

# get current time 
my $t=time;

# open log file for errors
open(ERR, ">>$errlog") || die "can't open $errlog: $!\n";

# read data from the $datafile, put into @phrases

$delimiter = "\n";
open(FILE, $datafile) || die "can't open mon.lite file: $!\n";
@FILE = <FILE>;
close(FILE);

$start=time;

$phrases = join("<br>",@FILE);

##  fetch each line from datafile, line is separated by "\n"

@phrases = split(/$delimiter/,$phrases);

# walk phrases and process on line by line basis
# one subroutine per filter type


foreach my $elem (@phrases) {

     # get rid off blanks, return, line breaks

     $elem =~ s/\n//g;
     $elem =~ s/<br>//g;
     $saved_line = $elem;
     $elem =~ s/ //g;    
     # convert greater than to zilch, until we figure out
     # how to escape it
     $global_line = $elem;
     $global_line=~ s/\>//g;

     # save each item to array @items_in_eachline, from each line,

     @items_in_eachline = split(/:/,$elem);
#     print $items_in_eachline[0],"\n";    

     # check the first item in each line, against the key words, to see 
     #     if filter is enabled.

     # sysname makes pkts unique per system with per-system label
     # front-end arranges to make sure this label comes first in
     # mon-lite file.
     if ($items_in_eachline[0] eq "sysname") {
	$sysname = $items_in_eachline[1];
	# override rrd name with new name
	$pkts=$rrdbase . "\/" . $sysname . "\." .  "pkts.rrd";
     }
     elsif ($items_in_eachline[0] eq "pkts") {
	unless (open(HANDLE, $pkts)) {
		pkts_create();
	};	
	close(HANDLE);
	pkts_update(@items_in_eachline);
     }
     elsif  ($items_in_eachline[0] eq "fixed_ipproto") {
	unless (open(HANDLE, $ipproto)) {
		ipproto_create();
	};	
	close(HANDLE);
        ipproto_update(@items_in_eachline);
     }
     elsif ($items_in_eachline[0] eq "fixed_tcp3") {
	unless (open(HANDLE, $tcp3)) {
		tcp3_create();
	};	
	close(HANDLE);
        tcp3_update(@items_in_eachline);
     }
     elsif ($items_in_eachline[0] eq "fixed_iprange1") {
	unless (open(HANDLE, $iprange1)) {
		iprange1_create();
	};	
	close(HANDLE);
        iprange1_update(@items_in_eachline);
     }
     elsif ($items_in_eachline[0] eq "fixed_cast") {
	unless (open(HANDLE, $cast)) {
		cast_create();
	};	
	close(HANDLE);
        cast_update(@items_in_eachline);
     }
     elsif ($items_in_eachline[0] eq "fixed_size") {
	unless (open(HANDLE, $size)) {
		size_create();
	};	
	close(HANDLE);
        size_update(@items_in_eachline);
     }
     elsif  ($items_in_eachline[0] eq "bpf") {
        bpf_update($saved_line);
     }
     # topn ip (all flows)	
     elsif ($items_in_eachline[0] eq "topn_ip") {
        $ip_flowcount = $items_in_eachline[1] ;
	$topn_exists = 1;
        topn_update("IP", "topn_ip", @items_in_eachline);
        $global_line=~ s/\(/[/g;
        $global_line=~ s/\)/]/g;
	if ($logging) {
		omsyslog($logbase, "topn_ip.log", $global_line);
	}
     }
     # topn tcp flows
     elsif ($items_in_eachline[0] eq "topn_tcp") {
        $tcp_flowcount = $items_in_eachline[1] ;
        topn_update("TCP", "topn_tcp", @items_in_eachline);
	if ($logging) {
		omsyslog($logbase, "topn_tcp.log", $global_line);
	}
     }
     # topn udp flows
     elsif ($items_in_eachline[0] eq "topn_udp") {
        $udp_flowcount = $items_in_eachline[1] ;
        topn_update("UDP", "topn_udp", @items_in_eachline);
	if ($logging) {
		omsyslog($logbase, "topn_udp.log", $global_line);
	}
     }
    elsif ($items_in_eachline[0] eq "topn_icmp") {            
        $icmp_flowcount = $items_in_eachline[1] ;
        topn_update("ICMP", "topn_icmp", @items_in_eachline);
        if ($logging) {                                    
		omsyslog($logbase, "topn_icmp.log", $global_line);
        }                                                
     }                                                  
     elsif ($items_in_eachline[0] eq "tcp_ports") {
        topn_tcp_port_update(@items_in_eachline);
        if ($logging) {                                    
		omsyslog($logbase, "tcp_ports.log", $global_line);
        }                                                
     }
     elsif ($items_in_eachline[0] eq "udp_ports") {
        topn_udp_port_update(@items_in_eachline);
        if ($logging) {                                    
		omsyslog($logbase, "udp_ports.log", $global_line);
        }                                                
     } 
     elsif ($items_in_eachline[0] eq "syn_list") {
        if ($logging) {                                    
		omsyslog($logbase, "syn_list.log", $global_line);
        }                                                
        topn_synlist_update(@items_in_eachline);
     } 
     # ircstats
     elsif ($items_in_eachline[0] eq "ircstats") {
        irc_update(@items_in_eachline);
     } 
     # tworm is RRDTOOL-based and is also a side-effect of the top syn list
     #
     elsif ($items_in_eachline[0] eq "tworm") {
        tworm_update(@items_in_eachline);
     } 
     elsif ($items_in_eachline[0] eq "icmperror_list") {
        if ($logging) {                                    
		omsyslog($logbase, "icmperror_list.log", $global_line);
        }                                                
        topn_icmperror_update(@items_in_eachline);
     } 
     elsif ($items_in_eachline[0] eq "udperror_list") {
        if ($logging) {                                    
		omsyslog($logbase, "udperror_list.log", $global_line);
        }                                                
        topn_udperror_update(@items_in_eachline);
     } 
     # RRDTOOL-based
     elsif ($items_in_eachline[0] eq "hs_stat") {
        topn_stat(@items_in_eachline);
     }
     elsif ($items_in_eachline[0] eq "ip_scan") {
        ip_scan_generic("IP Scans (IP src by unique IP dsts)", "ip_scan", @items_in_eachline);
        if ($logging) {                                    
		omsyslog($logbase, "ip_scan.log", $global_line);
        }                                                
     }
     elsif ($items_in_eachline[0] eq "ip_portscan") {
        ip_scan_generic("IP Port Scans (IP src by unique L4 ports)", "ip_port_scan", @items_in_eachline);
        if ($logging) {                                    
		omsyslog($logbase, "ip_portscan.log", $global_line);
        }                                                
     }
     elsif ($items_in_eachline[0] eq "tcp_portscan") {
        ip_scan_generic("TCP Port Scans (IP src by unique TCP ports)", "tcpport_port_scan", @items_in_eachline);
        if ($logging) {                                    
		omsyslog($logbase, "tcp_portscan.log", $global_line);
        }                                                
     }
     elsif ($items_in_eachline[0] eq "udp_portscan") {
        ip_scan_generic("UDP Port Scans (IP src by unique UDP ports)", "udpport_port_scan", @items_in_eachline);
        if ($logging) {                                    
		omsyslog($logbase, "udp_portscan.log", $global_line);
        }                                                
     }
     elsif ($items_in_eachline[0] eq "elog") {
        @elog_items = split(/:/,$saved_line);
	storeEvent($eventfile, "ourmon front-end event:", $elog_items[1]);				
     }
	
     # else just ignore it so that it can be tested
	
}  # end loop

if ($topn_exists) {
        $flow_data = "$t:" . $ip_flowcount . ":" . $tcp_flowcount . ":" . $udp_flowcount . ":" .  $icmp_flowcount;
        #print $flow_data;
        flowcount_update($flow_data);
}


close ERR;

#    update and make png file for pkt capture
#    note: we support two possible interfaces at the same time
#################################################################
sub pkts_update 
{ 
my $graph = "pkts";
if ($sysname) {
	$graph = $sysname . "\." . $graph;
}

# TBD. update event log if pkts are lost
# do update
RRDs::update $pkts, "$t:"."$_[2]".":"."$_[4]".":"."$_[6]".":"."$_[8]";
if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$pkts': $ERROR\n";
      $time=`date`;
      $time=~ s/\n//g;
      print ERR "$time, error with rrd/update on $pkts: $ERROR\n";
};

# args are 1. filename,  2. title,  3. -s time back from now
#
graph_pkts("$graph.daily-png", "daily: BPF: pkts captured/dropped"." : ".$start_time, "-2000m");
graph_pkts("$graph.weekly-png", "weekly: BPF: pkts capt/dropped"." : ".$start_time, "-12000m");
graph_pkts("$graph.monthly-png", "monthly: BPF: pkts capt/dropped"." : ".$start_time, "-800h");
graph_pkts("$graph.yearly-png", "yearly: BPF: pkts capt/dropped"." : ".$start_time, "-400d");

}

# spit out rrdtool-speak to graph the PKTS 4 graphs

sub graph_pkts
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "pkts/sec",
#"-w", "400",
#"-h",  "100", 
"-b", "1024", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 
"DEF:cap1=$pkts:cap1:AVERAGE",
"CDEF:pcap1=cap1,30,/",
"DEF:drop1=$pkts:drop1:AVERAGE",
"CDEF:pdrop1=drop1,30,/",
"DEF:cap2=$pkts:cap2:AVERAGE",
"CDEF:pcap2=cap2,30,/",
"DEF:drop2=$pkts:drop2:AVERAGE",
"CDEF:pdrop2=drop2,30,/",
"LINE1:pcap1#0000ff:Capt(1)",
"LINE1:pdrop1#00ff00:Drop(2)",
"LINE1:pcap2#ff0000:Capt(2)",
"LINE1:pdrop2#ff00ff:Drop(2)",
"COMMENT:               \r",
"GPRINT:pcap1:MAX:Max capt1 %9.0lf %s",
"GPRINT:pcap1:AVERAGE:Average capt1 %9.0lf %s", 
"GPRINT:pcap1:LAST:Current capt1 %9.0lf %s", 
"GPRINT:pdrop1:MAX:Max drop1 %9.0lf %s",
"GPRINT:pdrop1:AVERAGE:Average drop1 %9.0lf %s", 
"GPRINT:pdrop1:LAST:Current drop1 %9.0lf %s", 
"GPRINT:pcap2:MAX:Max capt2 %9.0lf %s",
"GPRINT:pcap2:AVERAGE:Average capt2 %9.0lf %s", 
"GPRINT:pcap2:LAST:Current capt2 %9.0lf %s", 
"GPRINT:pdrop2:MAX:Max drop2 %9.0lf %s",
"GPRINT:pdrop2:AVERAGE:Average drop2 %9.0lf %s", 
"GPRINT:pdrop2:LAST:Current drop2 %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
#    die "$0: unable to graph `$pkts': $ERROR\n";
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $pkts: $ERROR\n";
#	print "$time, error with rrd/update on $pkts: $ERROR\n";
};

}

# update and make png file for fixed ipproto
#####################################################

sub ipproto_update
{

RRDs::update $ipproto, "$t:"."$_[2]".":"."$_[4]".":"."$_[6]".":"."$_[8]";
 if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$ipproto': $ERROR\n";
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    error with rrd/update on $ipproto: $ERROR\n";
  };

graph_ipproto("ipproto.daily-png", "daily: Fixed IP Protocols"." : ".$start_time, "-2000m");
graph_ipproto("ipproto.weekly-png", "weekly: Fixed IP Protocols"." : ".$start_time, "-12000m");
graph_ipproto("ipproto.monthly-png", "monthly: Fixed IP Protocols"." : ".$start_time, "-800h");
graph_ipproto("ipproto.yearly-png", "yearly: Fixed IP Protocols"." : ".$start_time, "-400d");

#RRDs::graph "ipproto.png",
#  "--title", "Fixed IP Protocols",
#  "--vertical-label", "bytes",
#  "--imgformat","PNG",
#  "DEF:a=$ipproto:tcp:AVERAGE",
#  "DEF:b=$ipproto:udp:AVERAGE",
#  "DEF:c=$ipproto:icmp:AVERAGE",
#  "DEF:d=$ipproto:xtra:AVERAGE",
#  "LINE1:a#0000ff:$_[1]",
#  "LINE1:b#00ff00:$_[3]",
#  "LINE1:c#ff0000:$_[5]",
#  "LINE1:d#0ffff0:$_[7]",
#;

}

# stored as bytes, multiple by 8 here and divide by 30 (secs)
# to get bits per second
#
sub graph_ipproto
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "bits/sec",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 
"DEF:tcp=$ipproto:tcp:AVERAGE",
"CDEF:ptcp=tcp,8,*,30,/",

"DEF:udp=$ipproto:udp:AVERAGE",
"CDEF:pudp=udp,8,*,30,/",

"DEF:icmp=$ipproto:icmp:AVERAGE",
"CDEF:picmp=icmp,8,*,30,/",

"DEF:xtra=$ipproto:xtra:AVERAGE",
"CDEF:pxtra=xtra,8,*,30,/",

"LINE1:ptcp#0000ff:tcp",
"LINE1:pudp#00ff00:udp",
"LINE1:picmp#ff0000:icmp",
"LINE1:pxtra#ff00ff:xtra",

"COMMENT:               \r",
"GPRINT:ptcp:MAX:Max tcp %9.0lf %s",
"GPRINT:ptcp:AVERAGE:Average tcp %9.0lf %s", 
"GPRINT:ptcp:LAST:Current tcp %9.0lf %s", 
"GPRINT:pudp:MAX:Max udp %9.0lf %s",
"GPRINT:pudp:AVERAGE:Average udp %9.0lf %s", 
"GPRINT:pudp:LAST:Current udp %9.0lf %s", 
"GPRINT:picmp:MAX:Max icmp %9.0lf %s",
"GPRINT:picmp:AVERAGE:Average icmp %9.0lf %s", 
"GPRINT:picmp:LAST:Current icmp %9.0lf %s", 
"GPRINT:pxtra:MAX:Max xtra %9.0lf %s",
"GPRINT:pxtra:AVERAGE:Average xtra %9.0lf %s", 
"GPRINT:pxtra:LAST:Current xtra %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$ipproto': $ERROR\n";
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $ipproto: $ERROR\n";
#	print "$time, error with rrd/update on $ipproto: $ERROR\n";
};

}

# update and make png file for fixed tcp3
#####################################################

sub tcp3_update
{

RRDs::update $tcp3, "$t:"."$_[2]".":"."$_[4]".":"."$_[6]";

if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$tcp3': $ERROR\n";
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    Update ERROR : $ERROR\n" if $ERROR;
};

graph_tcp3("tcp3.daily-png", "daily: 2 Fixed TCP dst ports"." : ".$start_time, "-2000m",
	$_[1], $_[3], $_[5]);
graph_tcp3("tcp3.weekly-png", "weekly: 2 Fixed TCP dst ports"." : ".$start_time, "-12000m",
	$_[1], $_[3], $_[5]);
graph_tcp3("tcp3.monthly-png", "monthly: 2 Fixed TCP dst ports"." : ".$start_time, "-800h",
	$_[1], $_[3], $_[5]);
graph_tcp3("tcp3.yearly-png", "yearly: 2 Fixed TCP dst ports"." : ".$start_time, "-400d",
	$_[1], $_[3], $_[5]);

#RRDs::graph "tcp3.png",
#  "--title", "Fixed TCP Ports",
#  "--vertical-label", "bytes",
#  "--imgformat","PNG",
#  "DEF:port1=$tcp3:port1:AVERAGE",
#  "DEF:port2=$tcp3:port2:AVERAGE",
#  "DEF:xtra=$tcp3:xtra:AVERAGE",
#  "LINE1:port1#0000ff:$_[1]",
#  "LINE1:port2#00ff00:$_[3]",
#  "LINE1:xtra#ff0000:$_[5]",
#;

}

sub graph_tcp3
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
my $label1 = $_[3];
my $label2 = $_[4];
my $label3 = $_[5];
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "bits/sec",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 
"DEF:port1=$tcp3:port1:AVERAGE",
"CDEF:pport1=port1,8,*,30,/",

"DEF:port2=$tcp3:port2:AVERAGE",
"CDEF:pport2=port2,8,*,30,/",

"DEF:xtra=$tcp3:xtra:AVERAGE",
"CDEF:pxtra=xtra,8,*,30,/",


#  "LINE1:port1#0000ff:$_[1]",
#  "LINE1:port2#00ff00:$_[3]",
#  "LINE1:xtra#ff0000:$_[5]",

"LINE1:pport1#0000ff:$label1",
"LINE1:pport2#00ff00:$label2",
"LINE1:pxtra#ff0000:$label3",

"GPRINT:pport1:MAX:Max port1 %9.0lf %s",
"GPRINT:pport1:AVERAGE:Average port1 %9.0lf %s", 
"GPRINT:pport1:LAST:Current port1 %9.0lf %s", 
"GPRINT:pport2:MAX:Max port2 %9.0lf %s",
"GPRINT:pport2:AVERAGE:Average port2 %9.0lf %s", 
"GPRINT:pport2:LAST:Current port2 %9.0lf %s", 
"GPRINT:pxtra:MAX:Max xtra %9.0lf %s",
"GPRINT:pxtra:AVERAGE:Average xtra %9.0lf %s", 
"GPRINT:pxtra:LAST:Current xtra %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$tcp3': $ERROR\n";
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $tcp3: $ERROR\n";
#	print "$time, error with rrd/update on $tcp3: $ERROR\n";
};

}

# update and make png file for iprange
##############################################################

sub iprange1_update
{
RRDs::update $iprange1, "$t:"."$_[2]".":"."$_[4]";
if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$iprange1': $ERROR\n";
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    unable to rrd/update $iprange1: $ERROR\n";
};

graph_iprange1("iprange1.daily-png", "daily: Fixed IP range"." : ".$start_time, "-2000m",
	$_[1]);
graph_iprange1("iprange1.weekly-png", "weekly: 2 Fixed IP range"." : ".$start_time, "-12000m",
	$_[1]);
graph_iprange1("iprange1.monthly-png", "monthly: 2 Fixed IP range"." : ".$start_time, "-800h",
	$_[1]);
graph_iprange1("iprange1.yearly-png", "yearly: 2 Fixed IP range"." : ".$start_time, "-400d",
	$_[1]);

# note:
# string labels are supplied from mon.lite file 
#RRDs::graph "iprange1.png",
#  "--title", "Fixed IP Range",
#  "--vertical-label", "bytes",
#  "--imgformat","PNG",
#  "DEF:range=$iprange1:range:AVERAGE",
#  "DEF:xtra=$iprange1:xtra:AVERAGE",
#  "LINE1:range#0000ff:$_[1]",
#  "LINE1:xtra#00ff00:$_[3]",
#;
}

sub graph_iprange1
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
my $label1 = $_[3];
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "bits/sec",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 
"DEF:extra=$iprange1:xtra:AVERAGE",
"CDEF:pxtra=extra,8,*,30,/",
"DEF:range=$iprange1:range:AVERAGE",
"CDEF:prange=range,8,*,30,/",
"LINE1:prange#0000ff:$label1",
"LINE1:pxtra#ff0000:xtra",
"GPRINT:prange:MAX:Max range %9.0lf %s",
"GPRINT:prange:AVERAGE:Average range %9.0lf %s", 
"GPRINT:prange:LAST:Current range %9.0lf %s", 
"GPRINT:pxtra:MAX:Max xtra %9.0lf %s",
"GPRINT:pxtra:AVERAGE:Average xtra %9.0lf %s", 
"GPRINT:pxtra:LAST:Current xtra %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $iprange1: $ERROR\n";
};

}


##############################################################
#
#
#
# update and make png file for ipcastiing
#
#
#
##############################################################

sub cast_update
{
RRDs::update $cast, "$t:"."$_[2]".":"."$_[4]".":"."$_[6]".":"."$_[8]";
if ($ERROR = RRDs::error) {
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    error on rrd/update for $cast: $ERROR\n";
};

graph_cast("cast.daily-png", "daily: Fixed M/U/B cast pkts"." : ".$start_time, "-2000m",
	$_[1], $_[3], $_[5], $_[7]);
graph_cast("cast.weekly-png", "weekly: Fixed M/U/B cast pkts"." : ".$start_time, "-12000m",
	$_[1], $_[3], $_[5], $_[7]);
graph_cast("cast.monthly-png", "monthly: Fixed M/U/B cast pkts"." : ".$start_time, "-800h",
	$_[1], $_[3], $_[5], $_[7]);
graph_cast("cast.yearly-png", "yearly: Fixed M/U/B cast pkts"." : ".$start_time, "-400d",
	$_[1], $_[3], $_[5], $_[7]);
 
# note
# string labels are supplied from mon.lite file 
#RRDs::graph "cast.png",
#  "--title", "mcast vs. ucast vs. bcast vs. xtra",
#  "--vertical-label", "bytes",
#  "--imgformat","PNG",
#  "DEF:mcast=$cast:mcast:AVERAGE",
#  "DEF:ucast=$cast:ucast:AVERAGE",
#  "DEF:bcast=$cast:bcast:AVERAGE",
#  "DEF:xtra=$cast:xtra:AVERAGE",
#  "LINE1:mcast#0000ff:$_[1]",
#  "LINE1:ucast#00ff00:$_[3]",
#  "LINE1:bcast#ff0000:$_[5]",
#  "LINE1:xtra#0ffff0:$_[7]",
#;
}

sub graph_cast
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
my $label1 = $_[3];
my $label2 = $_[4];
my $label3 = $_[5];
my $label4 = $_[6];

#print $label1, $label2, $label3, $label4, "\n";
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "bits/sec",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 
#  "DEF:ucast=$cast:ucast:AVERAGE",
#  "DEF:bcast=$cast:bcast:AVERAGE",
#  "DEF:xtra=$cast:xtra:AVERAGE",

"DEF:mcast=$cast:mcast:AVERAGE",
"CDEF:pmcast=mcast,8,*,30,/",

"DEF:ucast=$cast:ucast:AVERAGE",
"CDEF:pucast=ucast,8,*,30,/",

"DEF:bcast=$cast:bcast:AVERAGE",
"CDEF:pbcast=bcast,8,*,30,/",

"DEF:xtra=$cast:xtra:AVERAGE",
"CDEF:pxtra=xtra,8,*,30,/",

"LINE1:pmcast#0000ff:$label1",
"LINE1:pucast#00ff00:$label2",
"LINE1:pbcast#ff0000:$label3",
"LINE1:pxtra#ff00ff:$label4",

"GPRINT:pmcast:MAX:Max mcast %9.0lf %s",
"GPRINT:pmcast:AVERAGE:Average mcast %9.0lf %s", 
"GPRINT:pmcast:LAST:Current mcast %9.0lf %s", 

"GPRINT:pucast:MAX:Max ucast %9.0lf %s",
"GPRINT:pucast:AVERAGE:Average ucast %9.0lf %s", 
"GPRINT:pucast:LAST:Current ucast %9.0lf %s", 

"GPRINT:pbcast:MAX:Max bcast %9.0lf %s",
"GPRINT:pbcast:AVERAGE:Average bcast %9.0lf %s", 
"GPRINT:pbcast:LAST:Current bcast %9.0lf %s", 

"GPRINT:pxtra:MAX:Max xtra %9.0lf %s",
"GPRINT:pxtra:AVERAGE:Average xtra %9.0lf %s", 
"GPRINT:pxtra:LAST:Current xtra %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $cast: $ERROR\n";
} 

}

# update and make png file for topn_ip, topn_tcp, topn_udp, topn_icmp
#
# mon.lite form: topn_udp : 994 : 207.98.64.97.10->131.252.242.78.30: 4476 :  next-flow-id: byte-count: etc ...
# 
# calling args:  topn_update("UDP", "topn_udp", tuple args ...)
sub topn_update
{
	my $label = $_[0];
	my $picfile = $_[1];
	my $i=3;
	#my $data_src="";
	my $count;
	my @data=(); #initialize data values to 0
	my $graph=1;
	my $j;
	my $bar_count = 0;  #tracking number of histogram bars to see how many pics needed
	my $ncount = 0;
	my $range;
	my $tl;
 
	# if no data
	if ( $_[$i]==0 ) {
		my $zpicfile = $picfile . "1.png";
    		`$topnprog -t "Top $label (no flows)" -v "bits/sec" -s "$start_time" -f $zpicfile -n 0`;
		if ($topndebug) {
			print "$topnprog -t \"Top $label (no flows)\" -v \"bits/sec\" -s \"$start_time\" -f $zpicfile -n 0\n";
		}
    		return;
	} 

	$i = 4;
	# walk the flow tuples
	while ( ! ($_[$i] eq "") ) {
		# scale the count from bytes per 30 sec to bits/sec
		$count = ($_[$i+1] * 8) / 30;
		$count = int($count);

		$data[$graph] = $data[$graph] . '"' . $_[$i] . '"' . " " . $count . " ";

#print "arg=$_[$i], bar_count = $bar_count, graph=$graph, $data[$graph]\n";

		$i = $i + 2;
		$bar_count++;
		if( (($bar_count) % 10) == 0) {
			$graph++ ;
			$lastone = 1;
		} 
		else {
			$lastone = 0;
		} 
#print "XXX: arg=$_[$i], bar_count = $bar_count, graph=$graph, $data[$graph]\n\n";
	} 

	if ($lastone) {
	   	--$graph;
	}
#print "graph=$graph, bar_count=$bar_count\n";
	for ($j=1; $j <= $graph; $j++) { 
		my $top;
		my $rs;
		$png_file = $picfile . "$j" . ".png";
		$range = ($j-1) * 10;
  		if ($bar_count > 10) {
			$bar_count = $bar_count - 10; 
			$ncount = 10;
  		}
  		else {
			$ncount = $bar_count;
  		}
		$top = $range+$ncount; 
		$rs = $range + 1;

#print "bar_count=$bar_count j=$j, png_file=$png_file, top=$top, rs=$rs, range=$range, ncount=$ncount\n";
  		`$topnprog -t "Top [$rs..$top] $label flows" -v "bits/sec" -s "$start_time"  -l $dns_lookup -f $png_file -n $ncount -d $data[$j]`;
   		if ($topndebug) {
			print "$label: range=$range, top=$top, graph=$graph j=$j pc=$bar_count, ncount=$ncount\n";
   			$print_line = "$topnprog -t \"Top [$rs..$top] $label flows\" -v \"bits/sec\" -s \"$start_time\" -l $dns_lookup -f $png_file -n $ncount -d $data[$j]";
   			print "$label flow drawtopn: $print_line \n\n";
   		}
	}
}

# update and make png file for topn_tcp_ports
##############################################################
  
sub topn_tcp_port_update
{
	my $i=1;
	my $count;
	#my $data_src="";
	my @data=(); #initialize data values to 0
	my $graph=1;
	my $j;
	my $port_count = 0;  #tracking number of ports to see if additional graphs are needed
	my $ncount = 0;
 
	if ( $_[$i]==0 ) {
		`$topnprog -t "Top TCP Ports (no flows)" -v "bits/sec" -s "$start_time" -l 0 -f "topn_tcp_port1.png" -n 0`;
		if ($topndebug) {
			print "$topnprog -t \"Top TCP Ports (no flows)\" -v \"bits/sec\" -s \"$start_time\" -l 0 -f \"topn_tcp_port1.png\" -n 0\n";
		}
		return;
	}

	while ( ! ($_[$i*2] eq "") ) {
		# scale the count from bytes per 30 sec to bits/sec
		$count = ($_[$i*2+1] * 8) / 30;
		$count = int($count);

		$data[$graph]=$data[$graph] . '"'.$_[$i*2]."(port)/$_[$i*2+2](src)/$_[$i*2+3](dst)". '"' . " " . $count . " ";

		$i = $i + 2;
		$port_count++;
		if( ($port_count % 10) == 0) {
			$graph++ ;
			$lastone = 1;
		} 
		else {
			$lastone = 0;
		} 
		#print "PORT: $port_count \n";
	} 

	$i = $i - 2;   
	if ($lastone) {
	   	--$graph;
	}
	my $topn_tcp_port = "topn_tcp_port"; 
	for ($j=1; $j <= $graph; $j++) { 
		my $top;
		my $rs;
		$png_file = $topn_tcp_port . "$j" . ".png";
		$port_range = ($j-1) * 10;
  		if ($port_count > 10) {
			$port_count = $port_count - 10; 
			$ncount = 10;
  		}
  		else {
			$ncount = $port_count;
  		}
		$top = $port_range+$ncount; 
		$rs = $port_range + 1;
  		`$topnprog -t "Top [$rs..$top] TCP Ports" -v "bits/sec"  -l 0 -f $png_file -n $ncount -d $data[$j]`;
   		if ($topndebug) {
			print "TCP: port_range=$port_range, top=$top, graph=$graph j=$j pc=$port_count, ncount=$ncount\n";
   			$print_line = "$topnprog -t \"Top [$rs..$top] TCP\" -v \"bits/sec\" -s \"$start_time\" -l 0 -f $png_file -n $ncount -d $data[$j]";
   			print "TCP port drawtopn: $print_line \n\n";
   		}
	}
}


# update and make png file for topn_udp_ports
##############################################################
  
sub topn_udp_port_update
{

	my $i=1;
	my $count;
	#my $data_src="";
	my @data=(); #initialize data values to 0
	my $graph=1;
	my $j;
	my $port_count = 0;  #tracking number of ports to see if additional graphs are needed
	my $ncount;
 
	if ( $_[$i]==0 ) {
		`$topnprog -t "Top UDP Ports (no flows)" -v "bits/sec" -s "$start_time" -f "topn_udp_port1.png" -n 0`;
		if ($topndebug) {
			print "$topnprog -t \"Top UDP Ports (no flows)\" -v \"bits/sec\" -s \"$start_time\" -f \"topn_udp_port1.png\" -n 0\n";
		}
		return;
	} 

	while ( ! ($_[$i*2] eq "") ) {
		# scale the count from bytes per 30 sec to bits/sec
		$count = ($_[$i*2+1] * 8) / 30;
		$count = int($count);

      		$data[$graph]=$data[$graph] . '"'.$_[$i*2]."(port)/$_[$i*2+2](src)/$_[$i*2+3](dst)". '"' . " " . $count . " ";

      		$i = $i + 2;

		$port_count ++;
		if( ($port_count % 10) == 0) {
			$graph++ ;
			$lastone = 1;
		} 
		else {
	  		$lastone = 0;
		} 
    	}

 	$i = $i - 2;   
 	if ($lastone) {
   		--$graph;
 	}

	my $topn_udp_port = "topn_udp_port"; 
	
	for ($j=1; $j <= $graph; $j++) { 
		my $top;
  		$png_file = $topn_udp_port . "$j" . ".png";
  		$port_range = ($j-1) * 10;
  		if ($port_count > 10) {
			$port_count = $port_count - 10; 
			$ncount = 10;
  		}
  		else {
			$ncount = $port_count;
  		}
	
		$top = $port_range+$ncount;
  		`$topnprog -t "Top [$port_range..$top] UDP Ports" -v "bits/sec"  -l 0 -f $png_file -n $ncount -d $data[$j]`;
   		if ($topndebug) {
			print "UDP: port_range $port_range, top=$top, graph=$graph j=$j pc=$port_count, ncount=$ncount\n";
   			$print_line = "$topnprog -t \"Top [$port_range..$top] UDP\" -v \"bits/sec\" -s \"$start_time\" -l 0 -f $png_file -n $ncount -d $data[$j]";
   			print "UDP port drawtopn: $print_line \n\n";
   		}
	}
}

# update and make png file for topn_synlist
# 131.252.207.11:4416:254:198:13829 
# ip             syn  fin rst total
#
#
# new format:
#  ip: syn: fin: rst: tcpfrom: tcpto: icmpcount: Nports: port,count,port,count,port,count
#
# weights/flags:
#
# 1. syn count, which we sort by
#
# 2. control/work metric:
# control packets/total work (total work is sum of tcpto/tcpfrom)
#
# 3. quadratic metric: (syns - fins) * (rsts + icmperrors)
#
# 4.
# if total of syn/fin/rst > 90% of total work:  W for wormy!
# if total of syn/fin/rst > 50% of total work:  w for wormy!
#
# 5. syns high (> 20), fins low < 5).  O is for Ouch.
# 6. no FINS, some resets flag: R is for Resets.
# 7. no packets returned flag: M is for Mum.
# 

##############################################################
  
sub topn_synlist_update
{
	my $i=2;	# first data tuple (ip addr)
	my $notuples = $_[1];	# total tuple count: 2nd item after label
	my $count;
	#my $data_src="";
	my @data=(); #initialize data values to 0
	my $graph=1;
	my $j;
	my $port_count = 0;  #tracking number of ports to see if additional graphs are needed
	my $ncount = 0;
	my $wopinion;
	my $percent;
	my $tcpc;
	my $rs;
 
	if ( !$notuples ) {
		`$topnprog -t "Top TCP syns  (no flows)" -v "syns/period" -s "$start_time" -l 0 -f "topn_tcp_syn1.png" -n 0`;
		if ($topndebug) {
			print "$topnprog -t \"Top TCP syns (no flows)\" -v \"syns/period\" -s \"$start_time\" -l 0 -f \"topn_tcp_syn1.png\" -n 0\n";
		}
		return;
	 };

	# loop through tuples
	# OLD
	#  ip: syn: fin: rst: tcpfrom: tcpto: icmpcount: Nports: port,count,port,count,port,count
	#   0    1    2    3        4      5          6  7       8
	# NEW
	#  ip: syn: synack: fin: rst: tcpfrom: tcpto: icmpcount: Nports: port,count,port,count,port,count
	#   0    1    2     3    4     5        6      7         8       9
	while ( ! ($_[$i] eq "") ) {
		my $syns = $_[$i+1];		# syns sent by ip
		my $synacks = $_[$i+2];		# syn/acks sent by ip
		my $fins = $_[$i+3];		# fins returned to ip
		my $rsts = $_[$i+4];		# resets returned to ip
		my $tcpfrom = $_[$i+5];		# tcp pkts sent by ip
		my $tcpto = $_[$i+6];		# tcp pkts returned to ip
		my $icmpcount = $_[$i+7];	# icmp errors returned to ip
		my $ports = $_[$i+8];		# number of port 2-tuples
		my $portstring = $_[$i+9];	# port 2-tuples, comma separated
		my $total = $tcpto + $tcpfrom;  # total tcp pkts exchanged

		$count = $syns;
		$count = int($count);	# probably unnecessary

		# $data[$graph]=$data[$graph] . '"' . $_[$i*2] . "(ip)/$_[$i*2+2](fin)/$_[$i*2+3](rst)/$_[$i*2+4](t)" . '"' . " " . $count . " ";

		# has to be in "legend" count format for drawtopn
		#
		$data[$graph]=$data[$graph] . '"' . $_[$i] ;		# ip src
		$data[$graph] .= " f:$fins";				# fins
		$data[$graph] .= " r:$rsts";				# rsts
		$data[$graph] .= " t:$total";				# total pkts
		$tcpc = $syns + $fins + $rsts;				# total control packets
		$wopinion = "";
		# worm opinion metric
		if (($tcpc) > (0.90 * $total)) {
			$wopinion = "W";
		}
		elsif ($tcpc > (0.50 * $total)) {
			$wopinion = "w";
		}
		$percent = 0;
		if ($total) {
			$percent = int(($tcpc * 100.0)/$total);
		}
		# tack on percent/worm opinion and concat count to get label/count for 
		# drawtopn
		$data[$graph] .=  " $percent%:$wopinion" . '"'  . " " . $count . " ";

		$port_count++;
		if( ($port_count % 10) == 0) {
			$graph++ ;
			$lastone = 1;
		} 
		else {
			$lastone = 0;
		} 
		# advance to next tuple
		$i = $i + 10;
	} 
	close(SYNFD);

	if ($lastone) {
	   	--$graph;
	}
	my $topn_tcp_port = "topn_tcp_syn"; 
	for ($j=1; $j <= $graph; $j++) { 
		my $top;
		$png_file = $topn_tcp_port . "$j" . ".png";
		$port_range = ($j-1) * 10;
  		if ($port_count > 10) {
			$port_count = $port_count - 10; 
			$ncount = 10;
  		}
  		else {
			$ncount = $port_count;
  		}
		$top = $port_range+$ncount; 
  		`$topnprog -t "Top [$port_range..$top] TCP Syns" -v "syns/period"  -l 0 -f $png_file -n $ncount -d $data[$j]`;
   		if ($topndebug) {
			print "TCP: syn_range=$port_range, top=$top, graph=$graph j=$j pc=$port_count, ncount=$ncount\n";
   			$print_line = "$topnprog -t \"Top [$port_range..$top] TCP Syns\" -v \"syns/period\" -s \"$start_time\" -l 0 -f $png_file -n $ncount -d $data[$j]";
   			print "TCP syn drawtopn: $print_line \n\n";
   		}
	}
}

# update and make png file for topn_icmperror
# format: ip address: icmp error count: tcp pkt count: udp count: icmp count: unreach: redirect: ttl
# note: icmp count here are ping (echo request) only
# e.g., 131.252.243.64:238:3258:4:238:1:0:237:
#       0              1   2    3 4   5 6 7
##############################################################
  
sub topn_icmperror_update
{
	my $i=2;	# first data tuple (ip addr)
	my $notuples = $_[1];	# total tuple count: 2nd item after label
	my $count;
	#my $data_src="";
	my @data=(); #initialize data values to 0
	my $graph=1;
	my $j;
	my $port_count = 0;  #tracking number of ports to see if additional graphs are needed
	my $ncount = 0;
 
	# if no tuples, draw null graph
	if ( !$notuples ) {
		`$topnprog -t "Top ICMP errors (no tuples)" -v "errors/period" -s "$start_time" -l 0 -f "topn_icmperror1.png" -n 0`;
		if ($topndebug) {
			print "$topnprog -t \"Top ICMP errors (no tuples)\" -v \"errors/period\" -s \"$start_time\" -l 0 -f \"topn_icmperror1.png\" -n 0\n";
		}
		return;
	}

	# while there is an ip address at the beginning of the tuple
	while ( ! ($_[$i] eq "") ) {
		my $tcpcount = $_[$i+2];
		my $udpcount = $_[$i+3];
		my $icmpcount = $_[$i+4];
		my $unreach = $_[$i+5];
		my $redirect = $_[$i+6];
		my $ttl = $_[$i+7];
		# get the count which is in the next entry up from the ip address
		# divide by 30 would be per sec, but this is per sample period so no divide by 30!
		$count = $_[$i+1];	# sort by number is second
		$count = int($count);	# probably unnecessary

		# build an associative array of graphs; i.e., strings to pass to drawtopn per graph
		# this is the -f option part
		# concat this string with the previous
		# note: we have to reconstruct the string in this format:
		#	("ip address, and info"  count)
		# to make drawtopn happy as it expects (legend, count), and makes a histogram based
		# on the count.
		# format: ip address: 
		$data[$graph]=$data[$graph] . '"' . $_[$i] ;
		if ($tcpcount) {
			$data[$graph] .= " T:$tcpcount";
		}
		if ($udpcount) {
			$data[$graph] .= " U:$udpcount";
		}
		if ($icmpcount) {
			$data[$graph] .= " P:$icmpcount";
		}
		if ($unreach) {
			$data[$graph] .= " Un:$unreach";
		}
		if ($redirect) {
			$data[$graph] .= " R:$redirect";
		}
		if ($ttl) {
			$data[$graph] .= " X:$ttl";
		}

		$data[$graph] .=  '"'  . " " . $count . " ";

		$i = $i + 8;
		$port_count++;
		if( ($port_count % 10) == 0) {
			$graph++ ;
			$lastone = 1;
		} 
		else {
			$lastone = 0;
		} 
		#print "PORT: $port_count \n";
	} 

	if ($lastone) {
	   	--$graph;
	}
	my $topn_tcp_port = "topn_icmperror"; 
	for ($j=1; $j <= $graph; $j++) { 
		my $top;
		$png_file = $topn_tcp_port . "$j" . ".png";
		$port_range = ($j-1) * 10;
  		if ($port_count > 10) {
			$port_count = $port_count - 10; 
			$ncount = 10;
  		}
  		else {
			$ncount = $port_count;
  		}
		$top = $port_range+$ncount; 

  		`$topnprog -t "Top [$port_range..$top] ICMP errors " -v "errors/period"  -l 0 -f $png_file -n $ncount -d $data[$j]`;
	
    		if ($topndebug) {
			#print "TCP: error_range=$port_range, top=$top, graph=$graph j=$j pc=$port_count, ncount=$ncount\n";
   			$print_line = "$topnprog -t \"Top [$port_range..$top] ICMP errors\" -v \"errors/period\" -s \"$start_time\" -l 0 -f $png_file -n $ncount -d $data[$j]";
   			print "ICMP error drawtopn: $print_line \n\n";
   		}
	}
}

# update and make png file for topn_udperror
# format: ip address: weight: udp sends: udp recvs: unreach: pings: noports: port tuple (dst_port, count)
#	  0	      1       2          3          4		5     6        7
##############################################################
  
sub topn_udperror_update
{
	my $i=2;	# first data tuple (ip addr)
	my $notuples = $_[1];	# total tuple count: 2nd item after label
	my $count;
	#my $data_src="";
	my @data=(); #initialize data values to 0
	my $graph=1;
	my $j;
	# note port is irrelevant here.  we are counting tuples.
	my $port_count = 0;  #tracking number of ports to see if additional graphs are needed
	my $ncount = 0;
 
	# if no tuples, draw null graph
	if ( !$notuples ) {
		`$topnprog -t "Top UDP errors (no tuples)" -v "errors/period" -s "$start_time" -l 0 -f "topn_udperror1.png" -n 0`;
		if ($topndebug) {
			print "$topnprog -t \"Top UDP errors (no tuples)\" -v \"errors/period\" -s \"$start_time\" -l 0 -f \"topn_udperror1.png\" -n 0\n";
		}
		return;
	}

	# print out port signature stuff in more detail than
	# can be done on the graphics
	udpReportOpen("udpreport.txt");

	# while there is an ip address at the beginning of the tuple
	while ( ! ($_[$i] eq "") ) {
		my $udpsent = $_[$i+2];
		my $udprecv = $_[$i+3];
		my $unreach = $_[$i+4];	# icmp unreach sent back
		my $icmpcount = $_[$i+5];	# pings sent by ip
		my $portcount = $_[$i+6];	# number of port tuples
		my $portstring = $_[$i+7];	# all port tuples in one string
		# get the count which is in the next entry up from the ip address
		$count = $_[$i+1];	# sort by number is second, here it is a weight.
		$count = int($count);	# probably unnecessary

		# print out a port report for each ip source
		# but only if the weight is non-zero
		if ($count > 0) {
			udpReport($_[$i], $count, $udpsent, $udprecv, $unreach, $icmpcount, $portcount, $portstring);
		}

		# build an associative array of graphs; i.e., strings to pass to drawtopn per graph
		# this is the -f option part
		# concat this string with the previous
		# note: we have to reconstruct the string in this format:
		#	("ip address, and info"  count)
		# to make drawtopn happy as it expects (legend, count), and makes a histogram based
		# on the count.
		# format: ip address: 
		$data[$graph]=$data[$graph] . '"' . $_[$i] ;
		if ($udpsent) {
			$data[$graph] .= " U:$udpsent";
		}
		if ($udprecv) {
			$data[$graph] .= " R:$udprecv";
		}
		if ($unreach) {
			$data[$graph] .= " Un:$unreach";
		}
		if ($icmpcount) {
			$data[$graph] .= " P:$icmpcount";
		}
		$data[$graph] .=  '"'  . " " . $count . " ";

		$i = $i + 8;
		$port_count++;
		if( ($port_count % 10) == 0) {
			$graph++ ;
			$lastone = 1;
		} 
		else {
			$lastone = 0;
		} 
		#print "PORT: $port_count \n";
	} 
	udpReportClose();

	if ($lastone) {
	   	--$graph;
	}
	my $topn_tcp_port = "topn_udperror"; 
	for ($j=1; $j <= $graph; $j++) { 
		my $top;
		$png_file = $topn_tcp_port . "$j" . ".png";
		$port_range = ($j-1) * 10;
  		if ($port_count > 10) {
			$port_count = $port_count - 10; 
			$ncount = 10;
  		}
  		else {
			$ncount = $port_count;
  		}
		$top = $port_range+$ncount; 

  		`$topnprog -t "Top [$port_range..$top] UDP errors " -v "errors/period"  -l 0 -f $png_file -n $ncount -d $data[$j]`;
	
    		if ($topndebug) {
			#print "TCP: error_range=$port_range, top=$top, graph=$graph j=$j pc=$port_count, ncount=$ncount\n";
   			$print_line = "$topnprog -t \"Top [$port_range..$top] UDP errors\" -v \"errors/period\" -s \"$start_time\" -l 0 -f $png_file -n $ncount -d $data[$j]";
   			print "UDP error drawtopn: $print_line \n\n";
   		}
	}
}

# update and make png file for topn_udp
##############################################################

#
# create the RRD file for the pkts filter
#
sub pkts_create
{

########################################################################
#
#  $start-1  
#                the time slot for starting 
#  "--step", 30
#                data updated for every 30 seconds 
#  DS:a, DS:b    (count/dropped for if 1, and same for i/f 2)
#                2 data sources, all is GAUGE type
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                 unknown. U:U means no max:min limit
#
# mrtg samples
#        600 5-minute samples:    2   days and 2 hours
#        600 30-minute samples:  12.5 days
#        600 2-hour samples:     50   days
#        600 1-day samples:     732   days
# therefore we take the mrtg scheme which is:
#                "RRA:AVERAGE:0.5:1:600",   
#                "RRA:AVERAGE:0.5:6:700",    
#                "RRA:AVERAGE:0.5:24:775", 
#                "RRA:AVERAGE:0.5:288:797",
#                "RRA:MAX:0.5:1:600",   
#                "RRA:MAX:0.5:6:700",     
#                "RRA:MAX:0.5:24:775",  
#                "RRA:MAX:0.5:288:797");
# and simply update it for 30 second sample periods,
# as opposed to 300 second sample periods (* 10)
#
# 1 sample per 30 seconds for 2 days plus,
# everything else * 10 for number of samples, 30 min. 2 hrs, 1 day

RRDs::create ($pkts, "--start",$start-1, "--step",30,
	"DS:cap1:GAUGE:600:U:U",
        "DS:drop1:GAUGE:600:U:U",
        "DS:cap2:GAUGE:600:U:U",
        "DS:drop2:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:2880:797",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:2880:797"
);

my $epkts = RRDs::error;
print ERR "rrd creation ERROR : $epkts\n" if $epkts;

}

sub ipproto_create
{
#######################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c, DS:d
#                4 data sources, all is GAUGE type, since they are already
#                 zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                 unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
#
########################################################################

RRDs::create ($ipproto, "--start",$start-1, "--step", 30,
	"DS:tcp:GAUGE:600:U:U",
        "DS:udp:GAUGE:600:U:U",
        "DS:icmp:GAUGE:600:U:U",
        "DS:xtra:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:2880:797",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:2880:797"
);

my $eipproto = RRDs::error;
print ERR "rrd creation ERROR : $eipproto\n" if $eipproto;
}

sub tcp3_create
{
######################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c
#                3 data sources, all is GAUGE type, since theay are already
#                zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                 unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
#
########################################################################

RRDs::create ($tcp3, "--start",$start-1, "--step", 30,
	"DS:port1:GAUGE:600:U:U",
        "DS:port2:GAUGE:600:U:U",
        "DS:xtra:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:2880:797",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:2880:797"
);

my $etcp3 = RRDs::error;
print ERR "rrd creation ERROR : $etcp3\n" if $etcp3;
}

sub iprange1_create
{
#######################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b 
#                2 data sources, all is GAUGE type, since theay are already
#                zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
#
########################################################################

RRDs::create ($iprange1, "--start",$start-1, "--step",30,
	"DS:range:GAUGE:600:U:U",
        "DS:xtra:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:2880:797",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:2880:797"
);
my $eiprange1 = RRDs::error;
print ERR "rrd creation ERROR : $eiprange1\n" if $eiprange1;
}

sub cast_create
{
########################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c, DS:d
#                4 data sources, all is GAUGE type, since they are already
#                zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
# 
########################################################################

RRDs::create ($cast, "--start",$start-1, "--step",30,
        "DS:mcast:GAUGE:600:U:U",
        "DS:ucast:GAUGE:600:U:U",
        "DS:bcast:GAUGE:600:U:U",
        "DS:xtra:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:2880:797",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:2880:797"
);
my $ecast = RRDs::error;
print ERR "rrd creation ERROR : $ecast\n" if $ecast;
}
 
sub size_create
{
########################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c, DS:d
#                4 data sources, all is GAUGE type, since they are already
#                zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
# 
########################################################################

RRDs::create ($size, "--start",$start-1, "--step",30,
        "DS:tiny:GAUGE:600:U:U",
        "DS:small:GAUGE:600:U:U",
        "DS:med:GAUGE:600:U:U",
        "DS:large:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:2880:797",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:2880:797"
);
my $esize = RRDs::error;
print ERR "rrd creation ERROR : $esize\n" if $esize;
}


sub size_update
{
RRDs::update $size, "$t:"."$_[2]".":"."$_[4]".":"."$_[6]".":"."$_[8]";
if ($ERROR = RRDs::error) {
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    error on rrd/update for $size: $ERROR\n";
};

graph_size("size.daily-png", "daily: packet sizes (100/500/1000/1500)", "-2000m",
	$_[1], $_[3], $_[5], $_[7]);
graph_size("size.weekly-png", "weekly: packet sizes (100/500/1000/1500)", "-12000m",
	$_[1], $_[3], $_[5], $_[7]);
graph_size("size.monthly-png", "monthly: packet sizes (100/500/1000/1500)", "-800h",
	$_[1], $_[3], $_[5], $_[7]);
graph_size("size.yearly-png", "yearly: packet sizes (100/500/1000/1500)", "-400d",
	$_[1], $_[3], $_[5], $_[7]);
}

sub graph_size
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
my $label1 = $_[3];
my $label2 = $_[4];
my $label3 = $_[5];
my $label4 = $_[6];

#print $label1, $label2, $label3, $label4, "\n";
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "pkts/sec",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 

"DEF:tiny=$size:tiny:AVERAGE",
"CDEF:ptiny=tiny,8,*,30,/",

"DEF:small=$size:small:AVERAGE",
"CDEF:psmall=small,8,*,30,/",

"DEF:med=$size:med:AVERAGE",
"CDEF:pmed=med,8,*,30,/",

"DEF:large=$size:large:AVERAGE",
"CDEF:plarge=large,8,*,30,/",

"LINE1:ptiny#0000ff:$label1",
"LINE1:psmall#00ff00:$label2",
"LINE1:pmed#ff0000:$label3",
"LINE1:plarge#ff00ff:$label4",

"GPRINT:ptiny:MAX:Max tiny %9.0lf %s",
"GPRINT:ptiny:AVERAGE:Average tiny %9.0lf %s", 
"GPRINT:ptiny:LAST:Current tiny %9.0lf %s", 

"GPRINT:psmall:MAX:Max small %9.0lf %s",
"GPRINT:psmall:AVERAGE:Average small %9.0lf %s", 
"GPRINT:psmall:LAST:Current small %9.0lf %s", 

"GPRINT:pmed:MAX:Max med %9.0lf %s",
"GPRINT:pmed:AVERAGE:Average med %9.0lf %s", 
"GPRINT:pmed:LAST:Current med %9.0lf %s", 

"GPRINT:plarge:MAX:Max large %9.0lf %s",
"GPRINT:plarge:AVERAGE:Average large %9.0lf %s", 
"GPRINT:plarge:LAST:Current large %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $size: $ERROR\n";
} 

}


@glabels;	# global list of bpf labels (individual graph labels)
@gcounts;	# global list of per label counts 
@glen;		# count of arguments

# boop
# syntax: bpf tag #pairs label count [label count]* ...  xtra count
sub bpf_update
{
	$line = $_[0];
	my $tag;
	my $uberlabel;
	my @words;
	my $index;
	my $labelpairs;
	my $rrdname;
	my $xtracount;
	my @cargs;
	my $countpkts;		# bytes or packets for label 

     	@words = split(/:/,$line);
	$tag = $uberlabel = $words[1];	# this is the graph label with blanks 
     	$tag =~ s/ //g;    # without blanks
	$countpkts = $words[2];	# flag: packets or bytes for count
	$labelpairs = $words[3];	# count of bpfs

	# put per graph labels in an array
	$index = 4;
	for ( $i = 0; $i < $labelpairs; $i++) {
		$glabels[$i] = $words[$index];
		$gcounts[$i] = $words[$index+1];
		$index = $index + 2;
	}
	$index++;
	$xtracount = $words[$index]; 
	$glen = $labelpairs;

	# take label and eliminate blanks.
	# then create rrd file if necessary
	#
	$rrdname = $rrdbase . "\/" . "$tag.rrd";
	unless (open(HANDLE, $rrdname)) {
		bpf_create($rrdname, @glabels);
	};	
	close(HANDLE);

	# update rrd
	$cargs[0] = $rrdname;
	$cargs[1] = "$t:";
	for ( $i = 0; $i < $labelpairs; $i++) {
		$cargs[1] = $cargs[1] . "$gcounts[$i]:"; 
	}
	$cargs[1] = $cargs[1] . "$xtracount"; 

	RRDs::update(@cargs);
 	if ($ERROR = RRDs::error) {
		#    die "$0: unable to update `$rrdname': $ERROR\n";
     		$time=`date`;
     		$time=~ s/\n//g;
     		print ERR "$time    error with rrd/update on $rrdname: $ERROR\n";
  	};

	# graph pictures
	graph_bpf($rrdname, "bpf.$tag.daily-png", 
		"daily: $uberlabel"." : ".$start_time, "-2000m", 
		$labelpairs, $xtracount, $countpkts);
	graph_bpf($rrdname, "bpf.$tag.weekly-png", 
		"weekly: $uberlabel"." : ".$start_time, "-12000m", 
		$labelpairs, $xtracount, $countpkts);
	graph_bpf($rrdname, "bpf.$tag.monthly-png", 
		"monthly: $uberlabel "." : ".$start_time, "-800h", 
		$labelpairs, $xtracount, $countpkts);
	graph_bpf($rrdname, "bpf.$tag.yearly-png", 
		"yearly: $uberlabel "." : ".$start_time, "-400d", 
		$labelpairs, $xtracount, $countpkts);

}

sub graph_bpf
{
	my $rrdname = $_[0];
	my $fname = $_[1];
	my $title = $_[2];
	my $stime = $_[3];
	my $pairs = $_[4];
	my $xtracount = $_[5];
	my $countpkts = $_[6];
	my @cargs;
	my $cargsindex;
	my $i;
	my $rgb;

	$cargs[0] = $fname;
	$cargs[1] = "--title"; $cargs[2] = $title;
	$cargs[3] = "-s"; $cargs[4] = $stime;
	$cargs[5] = "-e"; $cargs[6] = "now";
	$cargs[7] = "-a"; $cargs[8] = "PNG";
	$cargs[9] = "-v"; 
	$cargs[10] = "bits/sec";
	if ($countpkts) {
		$cargs[10] = "pkts/sec";
	}
	$cargs[11] = "-b"; $cargs[12] = "1000";
	$cargs[13] = "--alt-y-grid"; $cargs[14] = "--lazy";
	$cargs[15] = "-c";  $cargs[16] = "MGRID#ee0000";
	$cargs[17] = "-c";  $cargs[18] = "GRID#000000";

	$cargsindex = 19;
	for ($i = 0; $i < $pairs; $i++) {
		$cargs[$cargsindex++] = "DEF:$glabels[$i]=$rrdname:$glabels[$i]:AVERAGE";
		# for packets divide by 30 (pkts/sec) 
		# for bytes multiply by 8 and divide by 30 (bits/sec)
		if ($countpkts) {
		 	$cargs[$cargsindex++] = "CDEF:p$glabels[$i]=$glabels[$i],30,/";
		}
		else {
			$cargs[$cargsindex++] = "CDEF:p$glabels[$i]=$glabels[$i],8,*,30,/";
		}
	}
	$cargs[$cargsindex++] = "DEF:xtra=$rrdname:xtra:AVERAGE";
	if ($countpkts) {
		$cargs[$cargsindex++] = "CDEF:pxtra=xtra,30,/";
	}
	else {
		$cargs[$cargsindex++] = "CDEF:pxtra=xtra,8,*,30,/";
	}

	for ($i = 0; $i < $pairs; $i++) {
		$rgb = getrgb($i);
		$cargs[$cargsindex++] = "LINE1:p$glabels[$i]#$rgb:$glabels[$i]";
	}
	$rgb = getrgb($i);
	$cargs[$cargsindex++] = "LINE2:pxtra#$rgb:xtra";

	$cargs[$cargsindex++] = "COMMENT:               \r";

	for ($i = 0; $i < $pairs; $i++) {
		$cargs[$cargsindex++] = "GPRINT:p$glabels[$i]:MAX:Max $glabels[$i] %9.0lf %s";
		$cargs[$cargsindex++] = "GPRINT:p$glabels[$i]:AVERAGE:Average $glabels[$i] %9.0lf %s";
		$cargs[$cargsindex++] = "GPRINT:p$glabels[$i]:LAST:Current $glabels[$i] %9.0lf %s";
	}
	$cargs[$cargsindex++] = "GPRINT:pxtra:MAX:Max xtra %9.0lf %s";
	$cargs[$cargsindex++] = "GPRINT:pxtra:AVERAGE:Average xtra %9.0lf %s";
	$cargs[$cargsindex++] = "GPRINT:pxtra:LAST:Current xtra %9.0lf %s";
 
	RRDs::graph( @cargs );

	if ($ERROR = RRDs::error) {
	#    die "$0: unable to update `$ipproto': $ERROR\n";
      		$time=`date`;
      		$time=~ s/\n//g;
 		print ERR "$time, error with rrd/graph on $rrdname: $ERROR\n";
	#	print "$time, error with rrd/update on $rrdname: $ERROR\n";
	};
}

# bpf_create($filename, @labels)
#
# create bpf rrd in input filename, with variable number of DS entries
sub bpf_create
{
	my @labels = @_;
	my $name = $labels[0];	# name of rrd file
#	my $labelcount = $#labels;
 	my $labelcount = $glen;
	my $i;
	my @cargs;
	my $cargsindex = 5;

	$cargs[0] = $name; 
	$cargs[1] = "--start";
	$cargs[2] = $start-1; 
	$cargs[3] = "--step"; 
	$cargs[4] = 30;
	for ($i = 1; $i < ($labelcount + 1); $i++) {
		$cargs[$cargsindex++] = "DS:$labels[$i]:GAUGE:600:U:U",
	}
	$cargs[$cargsindex++] = "DS:xtra:GAUGE:600:U:U",
	$cargs[$cargsindex++] = "RRA:AVERAGE:0.5:1:6000";
	$cargs[$cargsindex++] = "RRA:AVERAGE:0.5:60:700";
	$cargs[$cargsindex++] = "RRA:AVERAGE:0.5:240:775";
	$cargs[$cargsindex++] = "RRA:AVERAGE:0.5:2880:797";
	$cargs[$cargsindex++] = "RRA:MAX:0.5:1:6000";
	$cargs[$cargsindex++] = "RRA:MAX:0.5:60:700";
	$cargs[$cargsindex++] = "RRA:MAX:0.5:240:775";
	$cargs[$cargsindex++] = "RRA:MAX:0.5:2880:797";

	RRDs::create (@cargs);

	my $eipproto = RRDs::error;
	print ERR "rrd creation ERROR : $eipproto\n" if $eipproto;
}

# make colors for labels ... this routine needs work
#
sub getrgb
{
	my $index = $_[0];
	my $n;

	#print $_0, " ", "$index\n";
	$n = $index % 6;
	if ($n eq 0) {
		return("0000ff");
	}
	elsif ($n eq 1) {
		return("00ff00");
	}
	elsif ($n eq 2) {
		return("ff0000");
	}
	elsif ($n eq 3) {
		#return("00fff0");
		# purple  red/blue
		return("ff00ff");
	}
	elsif ($n eq 4) {
		# yellow
		return("ffff00");
	}
	elsif ($n eq 5) {
		# cyan	
		return("00ffff");
		
	}
	return("f0f0f0");
}

#
# put the alert in the alert log file
# input params: flow plus bit count
# make a line, and quote for parens because they make
#       the shell unhappy.   
sub alert_log
{
	my $flow = $_[0];
	my $count = $_[1];
         
  	# ignore xtra as it is the remainder for top_ip
  	if ($flow eq "xtra") {
        	return;
  	}
  	$logline = $flow . " " . $count;
  	if ($logging) {
		omsyslog($logbase, "alert.log", $logline);
  	}
}

sub printUsageAndExit()
{
	print "-m mon.lite_file (default: /usr/home/mrourmon/monfiles/mon.lite)\n";
	print "-l logging_dir (default: /usr/home/mrourmon/logs)\n";
	print "-r rrdtool_database_dir (default: /usr/home/mrourmon/rrdbase)\n";
	print "-t drawtopn_path (default: /usr/home/mrourmon/bin/drawtopn)\n";
	print "-w web_dir_path  (default: /usr/local/apache/htdocs/ourmon)\n";
	print "-L [on | off]    (default: on)\n";
	print "-a megabits      (default: 10000000)\n"; 
	print "-e eventfile\n";
	print "-h :help message\n";
	exit 1;
}

# debug function
sub dumpArgs()
{
	print "(FILE)topnprog - $topnprog\n";
	print "(FILE)datafile - $datafile\n";
	print "(DIR)rrdbase - $rrdbase\n";
	print "(VAR)logbase - $logbase\n";
	print "(DIR)webpath - $webpath\n";
	print "(DIR)eventfile - $eventfile\n";
	print "(VAR)logging - $logging\n";
	print "(VAR)maxalertsize - $maxalertsize\n";
}

# -m mon.lite file (absolute dir/filename)
# -w web_dir path (absolute dir)
# -r rrdtool database path (absolute)
# -t absolute path to drawtopn program
# -l logging base directory path (absolute)
# -L logging on, else off.  args are {on, off}
# -a N  alert size in bits
# -z print defaults and exit
sub processCmdArgs
{
  	my $ret = 0;
  	my $arg;
  	while($arg = shift(@ARGV)){
    		if($arg eq '-z'){
			dumpArgs();
			exit(1);
    		}
		# mon.lite file
    		if($arg eq '-m'){
      			$datafile = '';   # clear the default config file
      			$datafile =  shift(@ARGV);
      			if($datafile eq '' || !defined($datafile)){
         			printUsageAndExit();
      			}
    		}
		# web dir path
		elsif($arg eq '-w'){
			$webpath = '';
			$webpath = shift(@ARGV);
			if($webpath eq '' || !defined($webpath)){
				printUsageAndExit();
		      	}
		}
		# rrdbase path
		elsif($arg eq '-r'){
			$rrdbase = '';
			$rrdbase = shift(@ARGV);
			if($rrdbase eq '' || !defined($rrdbase)){
				printUsageAndExit();
		      	}
		}
		# path for drawtopn
		elsif($arg eq '-t'){
		      $topnprog = '';
		      $topnprog = shift(@ARGV);
		      if($topnprog eq '' || !defined($topnprog)){
				printUsageAndExit();
		      }
		}
		# logbase
		elsif($arg eq '-l'){
		      $logbase = '';
		      $logbase = shift(@ARGV);
		      if($logbase eq '' || !defined($logbase)){
				printUsageAndExit();
		      }
		}
		# logging on/off
		elsif($arg eq '-L'){
		      $logging = '';
		      $logging = shift(@ARGV);
		      if($logging eq '' || !defined($logging)){
				printUsageAndExit();
		      }
		}
		# alertsize
		elsif($arg eq '-a'){
		      $maxalertsize = '';
		      $maxalertsize = shift(@ARGV);
		      if($maxalertsize eq '' || !defined($maxalertsize)){
				printUsageAndExit();
		      }
		}
		elsif($arg eq '-e'){
		      $eventfile = '';
		      $eventfile = shift(@ARGV);
		      if($eventfile eq '' || !defined($eventfile)){
				printUsageAndExit();
		      }
		      $eventflag = 1;
		}
		# help or anything else is help
    		elsif($arg eq '-h'){
      			printUsageAndExit();
    		}
    		else{
      			printUsageAndExit();
    		}
	}
}

# ------------------------------------------------------------>
# update and make png file for flowcount
#####################################################

sub flowcount_update
{

unless (open(HANDLE, $flowcount)) {
	flowcount_create();
};	
close(HANDLE);

RRDs::update $flowcount, "$_[0]";
 if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$flowcount': $ERROR\n";
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    error with rrd/update on $flowcount: $ERROR\n";
  };

graph_flowcount("flowcount.daily-png", "daily: FLOWS "." : ".$start_time, "-2000m");
graph_flowcount("flowcount.weekly-png", "weekly: FLOWS "." : ".$start_time, "-12000m");
graph_flowcount("flowcount.monthly-png", "monthly: FLOWS "." : ".$start_time, "-800h");
graph_flowcount("flowcount.yearly-png", "yearly: FLOWS "." : ".$start_time, "-400d");

#RRDs::graph "flowcount.png",
#  "--title", "FLOWS",
#  "--vertical-label", "bytes",
#  "--imgformat","PNG",
#  "DEF:a=$flowcount:tcp:AVERAGE",
#  "DEF:b=$flowcount:udp:AVERAGE",
#  "DEF:c=$flowcount:icmp:AVERAGE",
#  "DEF:d=$flowcount:xtra:AVERAGE",
#  "LINE1:a#0000ff:$_[1]",
#  "LINE1:b#00ff00:$_[3]",
#  "LINE1:c#ff0000:$_[5]",
#  "LINE1:d#0ffff0:$_[7]",
#;

}

# stored as flows, divide by 30 (secs)
#
sub graph_flowcount
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "flows/sec",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 

"DEF:ip=$flowcount:ip:AVERAGE",
"CDEF:pip=ip,30,/",

"DEF:tcp=$flowcount:tcp:AVERAGE",
"CDEF:ptcp=tcp,30,/",

"DEF:udp=$flowcount:udp:AVERAGE",
"CDEF:pudp=udp,30,/",

"DEF:icmp=$flowcount:icmp:AVERAGE",
"CDEF:picmp=icmp,30,/",


"LINE1:pip#0000ff:ip",
"LINE1:ptcp#00ff00:tcp",
"LINE1:pudp#ff0000:udp",
"LINE1:picmp#ff00ff:icmp",

"COMMENT:               \r",
"GPRINT:pip:MAX:Max ip %9.0lf %s",
"GPRINT:pip:AVERAGE:Average ip %9.0lf %s", 
"GPRINT:pip:LAST:Current ip %9.0lf %s", 
"GPRINT:ptcp:MAX:Max tcp %9.0lf %s",
"GPRINT:ptcp:AVERAGE:Average tcp %9.0lf %s", 
"GPRINT:ptcp:LAST:Current tcp %9.0lf %s", 
"GPRINT:pudp:MAX:Max udp %9.0lf %s",
"GPRINT:pudp:AVERAGE:Average udp %9.0lf %s", 
"GPRINT:pudp:LAST:Current udp %9.0lf %s", 
"GPRINT:picmp:MAX:Max icmp %9.0lf %s",
"GPRINT:picmp:AVERAGE:Average icmp %9.0lf %s", 
"GPRINT:picmp:LAST:Current icmp %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$flowcount': $ERROR\n";
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $flowcount: $ERROR\n";
#	print "$time, error with rrd/update on $flowcount: $ERROR\n";
};

}

sub flowcount_create
{
#######################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c, DS:d
#                4 data sources, all is GAUGE type, since they are already
#                 zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                 unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
#               
########################################################################
 
RRDs::create ($flowcount, "--start",$start-1, "--step", 30,
        "DS:ip:GAUGE:600:U:U",
        "DS:tcp:GAUGE:600:U:U",
        "DS:udp:GAUGE:600:U:U",
        "DS:icmp:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:2880:797",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:2880:797"
);

my $eflowcount = RRDs::error;
print ERR "rrd creation ERROR : $eflowcount\n" if $eflowcount;
}

#
# graph insert counter as it may provide clues
# about DOS attacks
# mon.lite:hs_stat: 42886: 656766: 357408: 247151: 52207: 31236: 52208: 11745:
#							         inserts
#
sub topn_stat
{
	my $insert = $_[7];

	unless (open(HANDLE, $topnstat)) {
		topnstat_create();
	};	
	close(HANDLE);


#	RRDs::update $topnstat, "$_[0]";
	RRDs::update $topnstat, "$t:"."$insert";
#print "$insert\n";
 	if ($ERROR = RRDs::error) {
     		$time=`date`;
     		$time=~ s/\n//g;
     		print ERR "$time    error with rrd/update on $topnstat: $ERROR\n";
  	};

	graph_topnstat("topnstat.daily-png", "daily: top inserts "." : ".$start_time, "-2000m");
	graph_topnstat("topnstat.weekly-png", "weekly: top inserts "." : ".$start_time, "-12000m");
	graph_topnstat("topnstat.monthly-png", "monthly: top inserts "." : ".$start_time, "-800h");
	graph_topnstat("topnstat.yearly-png", "yearly: top inserts "." : ".$start_time, "-400d");
}

# don't mod the insert number therefore per period
#
sub graph_topnstat
{

	my $fname = $_[0];
	my $title = $_[1];
	my $stime = $_[2];
 
	RRDs::graph( $fname,
	"--title", $title,
	"-s", $stime,
	"-e",  "now", 
	"-a",  "PNG", 
	"-v" , "count/period",
	"-b", "1000", 
	"--alt-y-grid", 
	"--lazy", 
	"-c", "MGRID#ee0000", 
	"-c", "GRID#000000", 

	"DEF:insert=$topnstat:insert:AVERAGE",
	"CDEF:pinsert=insert",
	"LINE1:pinsert#0000ff:insert",
	"COMMENT:               \r",
	"GPRINT:pinsert:MAX:Max insert %9.0lf %s",
	"GPRINT:pinsert:AVERAGE:Average insert %9.0lf %s", 
	"GPRINT:pinsert:LAST:Last insert %9.0lf %s", 
	);

	if ($ERROR = RRDs::error) {
		$time=`date`;
		$time=~ s/\n//g;
		print ERR "$time, error with rrd/graph on $topnsort: $ERROR\n";
	};

}

sub topnstat_create
{
#######################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c, DS:d
#                4 data sources, all is GAUGE type, since they are already
#                 zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                 unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
#               
########################################################################
 
	RRDs::create ($topnstat, "--start",$start-1, "--step", 30,
        	"DS:insert:GAUGE:600:U:U",

		"RRA:AVERAGE:0.5:1:6000",
		"RRA:AVERAGE:0.5:60:700",
		"RRA:AVERAGE:0.5:240:775",
		"RRA:AVERAGE:0.5:2880:797",

		"RRA:MAX:0.5:1:6000",
		"RRA:MAX:0.5:60:700",
		"RRA:MAX:0.5:240:775",
		"RRA:MAX:0.5:2880:797" );

	my $etopnstat = RRDs::error;
	print ERR "rrd creation ERROR : $etopnstat\n" if $etopnstat;
}

# graph tcp scan count with 3 measures
#	total: generated by this site: sent to this site
#
# This is a metadata graph that is generated as a side effect
# by the tcp topn syn counter.  Note: it is has nothing to do with
# any bpf tcp syn counter.
#
sub tworm_update
{
my $data;

unless (open(HANDLE, $twormcount)) {
	twormcount_create();
};	
close(HANDLE);

$data = "$t:" . $_[1] . ":" . $_[2] . ":" . $_[3];

RRDs::update $twormcount, $data;
 if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$twormcount': $ERROR\n";
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    error with rrd/update on $twormcount: $ERROR\n";
  };

graph_twormcount("twormcount.daily-png", "daily: tcp scan count "." : ".$start_time, "-2000m");
graph_twormcount("twormcount.weekly-png", "weekly: tcp scan count "." : ".$start_time, "-12000m");
graph_twormcount("twormcount.monthly-png", "monthly: tcp scan count "." : ".$start_time, "-800h");
graph_twormcount("twormcount.yearly-png", "yearly: tcp scan count "." : ".$start_time, "-400d");

#RRDs::graph "twormcount.png",
#  "--title", "FLOWS",
#  "--vertical-label", "bytes",
#  "--imgformat","PNG",
#  "DEF:a=$twormcount:tcp:AVERAGE",
#  "DEF:b=$twormcount:udp:AVERAGE",
#  "DEF:c=$twormcount:icmp:AVERAGE",
#  "DEF:d=$twormcount:xtra:AVERAGE",
#  "LINE1:a#0000ff:$_[1]",
#  "LINE1:b#00ff00:$_[3]",
#  "LINE1:c#ff0000:$_[5]",
#  "LINE1:d#0ffff0:$_[7]",
#;

}

# do not divide by 30 seconds, we want this per period
#
# there are 3 lines: count, us, them.
sub graph_twormcount
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "count/period",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 

"DEF:count=$twormcount:count:AVERAGE",
"CDEF:pcount=count",

"DEF:us=$twormcount:us:AVERAGE",
"CDEF:pus=us",

"DEF:them=$twormcount:them:AVERAGE",
"CDEF:pthem=them",


"LINE1:pcount#0000ff:count",
"LINE2:pus#00ff00:us",
"LINE3:pthem#ff0000:them",

"COMMENT:               \r",
"GPRINT:pcount:MAX:Max count %9.0lf %s",
"GPRINT:pcount:AVERAGE:Average count %9.0lf %s", 
"GPRINT:pcount:LAST:Current count %9.0lf %s", 
"GPRINT:pus:MAX:Max us %9.0lf %s",
"GPRINT:pus:AVERAGE:Average us %9.0lf %s", 
"GPRINT:pus:LAST:Current us %9.0lf %s", 
"GPRINT:pthem:MAX:Max them %9.0lf %s",
"GPRINT:pthem:AVERAGE:Average them %9.0lf %s", 
"GPRINT:pthem:LAST:Current them %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$twormcount': $ERROR\n";
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $twormcount: $ERROR\n";
#	print "$time, error with rrd/update on $twormcount: $ERROR\n";
};

}

sub twormcount_create
{
#######################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c, DS:d
#                4 data sources, all is GAUGE type, since they are already
#                 zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                 unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
#               
########################################################################
 
RRDs::create ($twormcount, "--start",$start-1, "--step", 30,
        "DS:count:GAUGE:600:U:U",
        "DS:us:GAUGE:600:U:U",
        "DS:them:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
);

my $etwormcount = RRDs::error;
	print ERR "rrd creation ERROR : $etwormcount\n" if $etwormcount;
}


# handle 4 port scan filters
#
# callings args:
#	$label - used in graph labels
#	$basefile - used to make unique base file name for pics 1 .. N
#	list of args from mon.lite

sub ip_scan_generic
{
	my $label = $_[0];
	my $picfile = $_[1];
	my $i=3;
	#my $data_src="";
	my $count;
	my @data=(); #initialize data values to 0
	my $graph=1;
	my $j;
	my $bar_count = 0;  #tracking number of histogram bars to see how many pics needed
	my $ncount = 0;
	my $range;
	my $tl;
 
	# if no data
	if ( $_[$i]==0 ) {
		my $zpicfile = $picfile . "1.png";
    		`$topnprog -t "Top $label (no flows)" -v "dst/period" -s "$start_time" -l 0 -f $zpicfile -n 0`;
		if ($topndebug) {
			print "$topnprog -t \"Top $label (no flows)\" -v \"dst/period\" -s \"$start_time\" -l 0 -f $zpicfile -n 0\n";
		}
    		return;
	} 

	$i = 4;
	# walk the flow tuples
	while ( ! ($_[$i] eq "") ) {
		# do not do anything to the count (no scaling)
		$count = ($_[$i+1] ) ;
		$count = int($count);

		$data[$graph] = $data[$graph] . '"' . $_[$i] . '"' . " " . $count . " ";

#print "arg=$_[$i], bar_count = $bar_count, graph=$graph, $data[$graph]\n";

		$i = $i + 2;
		$bar_count++;
		if( (($bar_count) % 10) == 0) {
			$graph++ ;
			$lastone = 1;
		} 
		else {
			$lastone = 0;
		} 
#print "XXX: arg=$_[$i], bar_count = $bar_count, graph=$graph, $data[$graph]\n\n";
	} 

	if ($lastone) {
	   	--$graph;
	}
#print "graph=$graph, bar_count=$bar_count\n";
	for ($j=1; $j <= $graph; $j++) { 
		my $top;
		my $rs;
		$png_file = $picfile . "$j" . ".png";
		$range = ($j-1) * 10;
  		if ($bar_count > 10) {
			$bar_count = $bar_count - 10; 
			$ncount = 10;
  		}
  		else {
			$ncount = $bar_count;
  		}
		$top = $range+$ncount; 
		$rs = $range + 1;

#print "bar_count=$bar_count j=$j, png_file=$png_file, top=$top, rs=$rs, range=$range, ncount=$ncount\n";
  		`$topnprog -t "Top [$rs..$top] $label" -v "dst/period" -s "$start_time"  -l 0 -f $png_file -n $ncount -d $data[$j]`;
   		if ($topndebug) {
			print "$label: range=$range, top=$top, graph=$graph j=$j pc=$bar_count, ncount=$ncount\n";
   			$print_line = "$topnprog -t \"Top [$rs..$top] $label\" -v \"dst/period\" -s \"$start_time\" -l 0 -f $png_file -n $ncount -d $data[$j]";
   			print "$label flow drawtopn: $print_line \n\n";
   		}
	}
}

#
# note: for now we are going with a one-week backup
# scheme:
#	1. cron zeroes the next days dir e.g., logs/Mon
#	2. omupdate.pl creates the log file and appends to it.
#
# In the future, we might just create logs based on the Month/day
# e.g., date gives us Mon Apr 5, so we might go with a month_date scheme.
# This would give us a month at a time; e.g.,
#	1. at midnight, cron zeros out the current log directory, and
#	put in a mon_date stamp (Apr_5)
#	2. omupdate creates logs within that directory.
#
# more or less syslog format in terms
# of date (although the year is in the date)
# followed by message.
#
# note: we have the nice property that the file
# is created on 1st log entry.
#
# calling convention:
# $mylogdir="/tmp/foo.log";
# $mylogname="topn_tcp";
# omsyslog($mylogdirectory, $mylogname, "my message");
#
# globals: start_time
#
sub omsyslog
{
	my $logbase = $_[0];
	my $logfile = $_[1];
	my $msg = $_[2];
	my $local_start_time= $start_time;
	$local_start_time=~ s/  / /g;
	my @time = split(/ /, $local_start_time);
	my $syslog_time = $time[1] . ' ' . $time[2] . ' ' . $time[3] . ' ' . $time[4] . ' ' . $time[5];
	# pathname is log dir + day of week + logfile name.
	my $pathname = "$logbase/$time[0]/$logfile";
	
	open(LOG, ">>$pathname") || die "can't open $pathname: $!\n";
	print LOG "$syslog_time: $msg\n";
	close(LOG);
}


# storeEvent($eventfile, "new worm signature from:", $sbuf);				
#
# storeEvent(file, prependString, eventString)
#
# eventString may or may not have a newline.  we strip it if it does.
#
sub
storeEvent
{

	if ($eventflag == 0) {
		return;
	}
	my $eventfile = $_[0];
	my $prependString = $_[1];
	my $eventString = $_[2];
	my $s;

	$eventString=~ s/\n//g;
	$s = "$start_time: " . $prependString . $eventString;

 	open(FILE, ">>$eventfile") || die "can't open $eventfile: $!\n";
 	print FILE "$s\n";
 	close(FILE);
}


# irc stats in RRDtool graphs
#
# count: JOIN: PING: PONGS: PRIVMSGS
#
sub irc_update
{
my $data;

unless (open(HANDLE, $irccount)) {
	irccount_create();
};	
close(HANDLE);

$data = "$t:" . $_[1] . ":" . $_[2] . ":" . $_[3] . ":" .$_[4];

RRDs::update $irccount, $data;
 if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$irccount': $ERROR\n";
     $time=`date`;
     $time=~ s/\n//g;
     print ERR "$time    error with rrd/update on $irccount: $ERROR\n";
  };

graph_irccount("irccount.daily-png", "daily: irc stats "." : ".$start_time, "-2000m");
graph_irccount("irccount.weekly-png", "weekly: irc stats "." : ".$start_time, "-12000m");
graph_irccount("irccount.monthly-png", "monthly: irc stats "." : ".$start_time, "-800h");
graph_irccount("irccount.yearly-png", "yearly: irc stats "." : ".$start_time, "-400d");

}

# do not divide by 30 seconds, we want this per period
#
# there are 4 lines: JOINS, PINGS, PONGS, PRIVMSGS
sub graph_irccount
{

my $fname = $_[0];
my $title = $_[1];
my $stime = $_[2];
 
RRDs::graph( $fname,
"--title", $title,
"-s", $stime,
"-e",  "now", 
"-a",  "PNG", 
"-v" , "count/period",
#"-w", "400",
#"-h",  "100", 
"-b", "1000", 
"--alt-y-grid", 
"--lazy", 
"-c", "MGRID#ee0000", 
"-c", "GRID#000000", 

"DEF:joins=$irccount:joins:AVERAGE",
"CDEF:pjoins=joins",

"DEF:pings=$irccount:pings:AVERAGE",
"CDEF:ppings=pings",

"DEF:pongs=$irccount:pongs:AVERAGE",
"CDEF:ppongs=pongs",

"DEF:pmsgs=$irccount:pmsgs:AVERAGE",
"CDEF:ppmsgs=pmsgs",


"LINE1:pjoins#0000ff:joins",
"LINE2:ppings#00ff00:pings",
"LINE3:ppongs#ff0000:pongs",
"LINE1:ppmsgs#0f0f0f:pmsgs",

"COMMENT:               \r",
"GPRINT:pjoins:MAX:Max joins %9.0lf %s",
"GPRINT:pjoins:AVERAGE:Average joins %9.0lf %s", 
"GPRINT:pjoins:LAST:Current joins %9.0lf %s", 

"GPRINT:ppings:MAX:Max pings %9.0lf %s",
"GPRINT:ppings:AVERAGE:Average pings %9.0lf %s", 
"GPRINT:ppings:LAST:Current pings %9.0lf %s", 

"GPRINT:ppongs:MAX:Max pongs %9.0lf %s",
"GPRINT:ppongs:AVERAGE:Average pongs %9.0lf %s", 
"GPRINT:ppongs:LAST:Current pongs %9.0lf %s", 

"GPRINT:ppmsgs:MAX:Max privmsgs %9.0lf %s",
"GPRINT:ppmsgs:AVERAGE:Average privmsgs %9.0lf %s", 
"GPRINT:ppmsgs:LAST:Current privmsgs %9.0lf %s", 
);

if ($ERROR = RRDs::error) {
#    die "$0: unable to update `$irccount': $ERROR\n";
      	$time=`date`;
      	$time=~ s/\n//g;
 	print ERR "$time, error with rrd/graph on $irccount: $ERROR\n";
#	print "$time, error with rrd/update on $irccount: $ERROR\n";
};

}

sub irccount_create
{
#######################################################################
#
#  $start-1
#                the time slot for starting
#  "--step", 30
#                data updated for every 30 seconds
#  DS:a, DS:b, DS:c, DS:d
#                4 data sources, all is GAUGE type, since they are already
#                 zeroed for every 30 seconds
#  600:U:U
#                if no data update for 600 seconds, data will be shown as
#                 unknown. U:U means no max:min limit
#  0.1:1:2880
#                sample data for every 30 seconds, 30 * 2880 = 24 hours
#               
########################################################################
 
RRDs::create ($irccount, "--start",$start-1, "--step", 30,
        "DS:joins:GAUGE:600:U:U",
        "DS:pings:GAUGE:600:U:U",
        "DS:pongs:GAUGE:600:U:U",
        "DS:pmsgs:GAUGE:600:U:U",
        "RRA:AVERAGE:0.5:1:6000",
        "RRA:AVERAGE:0.5:60:700",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:AVERAGE:0.5:240:775",
        "RRA:MAX:0.5:1:6000",
        "RRA:MAX:0.5:60:700",
        "RRA:MAX:0.5:240:775",
        "RRA:MAX:0.5:240:775",
);

my $eirccount = RRDs::error;
	print ERR "rrd creation ERROR : $eirccount\n" if $eirccount;
}

###############################################################################

#udpReportOpen("/tmp/udpreport.txt");
#udpReport("1.1.1.1",        100000, 1000, 10, 1009, 12, 3, "100,10,4,5,6,7");
#udpReport("131.212.211.111", 500,    100, 300, 30, 12, 1, "1,2");
#udpReport("101.212.211.111", 400,    100, 300, 30, 12, 3, "4,5,6,7,8,9");
#udpReport("15.11.211.111",   300,    100, 300, 30, 12, 5, "8,2,7,4,6,6,5,8,1,1");
#udpReportClose();

sub
udpReportOpen
{
	my $pathname = $_[0];
	my $sbuf;
 
	open(udpfd, ">$pathname") || die "can't open $pathname: $!\n";
	print udpfd "topn udp error list: port signatures\n\n";
	$sbuf = sprintf("%-16.16s %-12.12s %8s: %8s: %8s: %9s:   %10s: %s\n\n", 
		"ip src:", 
		"weight:", 
		"udp_sent", 
		"udp_recv", 
		"unreachs", 
		"ping", 
		"port_count", "port_sig[port, count]");
	print udpfd "$sbuf";
}

sub
udpReport
{
	my $ip = $_[0];
	my $weight = $_[1];
	my $udpsent = $_[2];
	my $udprecv = $_[3];
	my $unreach = $_[4];
	my $icmpcount = $_[5];		# pings sent by ip src
	my $ports = $_[6];
	my $portstring = $_[7];
	my @pset;
	my $pindex;
	my $pcount = 0;
	my @sp;
	my $nps="";

	# format is port,count etc.
     	my @ps = split(/,/,$portstring);


	# first compute the total port count
 	# meaning: the total number of dst sampled packets.	
	# which is NOT necessarily the total number of packets sent.
	for (my $i = 0; $i < $ports; $i++) {
		$pindex = $i * 2;
		$pcount += $ps[$pindex+1];
	}
	# now go thru and compute a percent for the 
	# destination port count (hits/total) and store it in an hash
	# array
	for (my $i = 0; $i < $ports; $i++) {
		$pindex = $i * 2;
		if ($pcount) {
			$pset{$ps[$pindex]} = int(($ps[$pindex+1] * 100.0)/ $pcount);
#print "$pset{$ps[$pindex]}, $ps[$pindex], $ps[$pindex+1]\n";
		}
	}

	# now sort the array based on the port number itself. 
	# sortedPorts is a list of the ports themselves
	@sp  = sort p_by_key keys(%pset);

	# now loop thru the ports
	# putting the ports into one string that is a signature tuple
	for ($i = 0; $i < $ports; $i++) {
		$nps = $nps . "[$sp[$i],$pset{$sp[$i]}]";
		#$ups = $ups . "[$sp[$i]]";			# ports only
	}

	foreach $key (keys(%pset)){
		delete $pset{$key};
	}
	#$portSignature{$ipsrc} = $nps;		# key is ip, value is ports

	# print it out
	$sbuf = sprintf("%-16.16s %-12.12s %9d %9d %9d %9d     %3d: %s", 
		$ip, $weight, $udpsent, $udprecv, $unreach, $icmpcount, $ports, $nps); 
	print udpfd "$sbuf\n";
}

sub
udpReportClose
{
	close(udpfd);
} 

# sort by the key for the ports array 
# above
sub p_by_key 
{
        if ($a < $b) {
                -1;
        }
        elsif ($a == $b) {
                0;
        }
        elsif ($a > $b) {
                1;
        }
}
