#! /usr/bin/perl

#
# tcpworm.pl
#
# syntax:
#
#	tcpworm.pl [-d -N -p port_file.txt -D dbfile] input_file > output_file
#
# -d: turn debug on
# -N: do dns processing for ip addresses.  note this can take
#	a long time. 
# -p: produce separate port signature file output.  Normally
#	it is not produced.   
# -D: dbfile for this instance of tcpworm.pl (allows different
#	front-ends on same back-end system).
#	note: if no -D default is /tmp/portdb.  
# 	This file may grow (although it doesn't seem to get too big in
#	real-life).
#	note: /tmp/foo here produces /tmp/foo.db.
#
#  input_file: mon.lite output (tcpworm.txt)
#  output_file: tcpworm report
#
# in general:
#	process tcpworm.txt file and produce stats, port signature
#	report and other things.
#
# port store:
# ipport{ipdstport} = count
# WORMipport{ipdstport} = count
#
# ip address:
# ipaddrs{ipid} = count
#
# portSignature{} used for storing per ip_src portSig info
# portSignature{$ipsrc} = $nps;
# portSignature_flags{$ipsrc} = $flags;
# portSignature_work{$ipsrc} = $percent;
# portSignature_sample{$ipsrc} = store the number of dst packets and the number of pkts sent.
# portSignature_server{$ipsrc} = synacks divided by syns.  
#
# unique port sig report:
# upsSig{} used for storing unique port sig tuple.
# index is e.g., "135,139,445" as string.  frequency/count not included.
# value is (date: count of instances seen: ip_src list (,,,,)),
# thus key: (date: instances: ip_src,...)
#
# this info is stored in the dbm database if it is new.
# it is looked up from the dbm database, and if not new this is shown
# in the report with the date and the original set of ips.

# this is the default portdb.db file for port signatures
# at this point it always exists but may be overridden
# with a runtime switch
$dbmfile = "/tmp/portdb";

# no events unless we are told we have an event file
# no default 
$eventflag = 0;

use Socket;
use Getopt::Std;


# global variables
# set to 1 for dns lookup.  dns lookup is SLOW
# so this is off by default.  
$dodns = 0;

# HACK: ignore
# set output name for synthesized counters in file to add to mon.lite
$monlite2 = "/tmp/mon.xxx";
$xtramon = 0;		# turn off if we don't want syn. counters

# set to top N for number of sorted ports to be output in report
$topnports = 20;

# set top N ip addresses for wormy ip address report
$topnipaddr = 20;

# debug on, off by default
$debug = 0;

# options go here
# tcpworm.pl [-d -N -p port_report.txt] input_file > output_file
# -N turn dns on (default is off)
# -p output_filename for port sig report (portsig.txt)
# -d debug on
# -D dbfile
# 
# stdout: 
%options=();
getopts("dNp:D:E:",\%options);
# like the shell getopt, "d:" means d takes an argument
#print "Unprocessed by Getopt::Std:\n" if $ARGV[0];
#foreach (@ARGV) {
#  print "$_\n";
#}
if (defined $options{d}) {
	$debug = 1;
}
if (defined $options{N}) {
	$dodns = 1;
}
$portreport = "";

if (defined $options{p}) {
	$portreport = $options{p};
	$portreport2 = $portreport .  "2";
}
if (defined $options{D}) {
	$dbmfile = $options{D};
}

if (defined $options{E}) {
	$eventfile = $options{E};	# name of ASCII log file
	$eventflag = 1;			# capability is on
		
}

if ($ARGV[0]) {
	$inputfile = $ARGV[0];
}
else {
	print "syntax: tcpworm.pl [-d -N -p port_report.txt -D dbfile -E eventfile] input_file > output_file\n";
	print "\t-d debug on\n";
	print "\t-N dns lookup on\n";
	print "\t-p file place port sig xtra report here\n";
	exit(1);
}

################################################### 
# get the UNIX time in seconds
$start_time=`date`;
$start_time=~ s/\n//g;

# init counters

# set various metric/weight constants
$wormconstant = 20;		# sins - fins > this constant
$econstant = 100000;		# E in EWORM constant
$ouchfins = 5;			# O in WORM constant
$Rresets = 5;			# R in WORM constant

# work weight used with port signature instance tuple
# we only store a port sig in the instance hash if
# it's weight is >= to this variable.
$ups_weight = 90;

# simple flag counters
$totaltuples = 0;
$Ecount = 0;	# number of E flags
$Wcount = 0;	# number of W flags
$wcount = 0;	# number of w flags
$Ocount = 0;	# number of O flags
$Rcount = 0;	# number of R flags
$Mcount = 0;	# number of M flags
$WWcount = 0;	# number of times S - F > C satisfied
# combo flag counters
$WOcount = 0;
$EWcount = 0;
$WORcount = 0;
$WORMcount = 0;
$WOMcount = 0;
$WMcount = 0;
$WRMcount = 0;
$comboTotal = 0;

#####################################################################
# averages for various weights
$allwork1 = 0;		# work average for all tuples
$WORMwork1 = 0;		# work average for all WORM tuples
$Wflagwork = 0;		# work average for any Wflag tuple

# work without fins in it
$allworkprime = 0;
$WORMworkprime = 0;

# fins divided by syns
$allfinpercent = 0;
$WORMfinpercent = 0;

# packets recv divided by packets sent
$allrecvpercent = 0;
$WORMrecvpercent = 0;

# resets divided by syns sent
$allresetpercent = 0;
$WORMresetpercent = 0;

# work averages for ports 25 and port 80
# of the non-worm variety
$port25work = 0;
$port25count = 0;
$port80work = 0;
$port80count = 0;

$portSigReport = "";		# string concatenation report
######################################################

# banner
tisaworm();

# parse the input file
doit();

# sort
sortit();

# final output printing
finalstats();

exit(0);

# end of main

######################################################################
# functions

# parse input file
sub doit() { 
	
	my $curline;
	my @words;
	my $sysname;
	my $synacks;

	open(FILE, $inputfile) || die "can't open $inputfile: $!\n";

	# line format:
	# ip src         : syn :synacks: fins: rsts  :tcpsent: tcpreturned: icmp error returned: # ports(1-4): (dst port, count) ... 
	# 131.252.123.160:19267:0:23:850:20180:   42:874:1:139,19267,:
	print "sorted syn tuples follow:\n\n";
	while(<FILE>) {

		$totaltuples++;
		s/\n//g;
		$curline = $_;
		@words = split(/:/, $curline);
	
		$flag="";
		$sysname = "";
		if ($dodns) {
			$sysname = gethostip($words[0]);
		}

		$syns = $words[1];
		$synacks = $words[2];
		$fins = $words[3];
		$rsts = $words[4];
		$tcpfrom = $words[5];		# tcp packets sent
		$tcpto = $words[6];		# tcp packets returned
		$icmpcount = $words[7];		# icmp packets (errors) returned
		$ports = $words[8];		# # port tuples
		$portstring = $words[9];	# portstring
		$total = $tcpfrom + $tcpto;	# total tcp pkts exchanged
		# avoid divide by 0 and reduce likely result to 0 value
		if ($total == 0) {
			$total = 1;
		}
		$tcontrol = $syns + $fins + $rsts;

		if (($syns - $fins) > $wormconstant) {
			$WWcount++;
			# TBD. event here.  
			# look this up in a database to see if it is too recent
			# if not too recent then store in event log
		}

		$allwork1 = $tcontrol/($total * 1.0) + $allwork1;
		$allworkprime = ($syns + rsts)/($total * 1.0) + $allworkprime;

                # worm opinion metric
		$wopinion = "";
                if (($tcontrol) > (0.90 * $total)) {
                        $wopinion = "W";
			$Wcount++;
                }
                elsif ($tcontrol > (0.50 * $total)) {
                        $wopinion = "w";
			$wcount++;
                }
                $percent = int(($tcontrol * 100.0)/$total);

                $rs = writeSynReport($words[0], $syns, $synacks, $fins, $rsts, $tcpfrom, $tcpto, $total,
                        $icmpcount, $ports, $portstring, $percent, $wopinion);

		if ($sysname) {
			print "($sysname):$rs\n";
		}
		else {
			print "$rs\n";
		}

	}  # end loop
	close(FILE);
}

# return dns from ip within timeout period of 5 seconds
# dnscache{} is associative array so that we can
# 	cache our own dns lookups
sub gethostip {
	my $ipaddr = $_[0];
	my $dnsname;

	if ($dnscache{$ipaddr}) {
		if ($debug) {
 			print "cached $ipaddr: $dnscache{$ipaddr}\n";
		}
		return ($dnscache{$ipaddr});
	}


	if ($debug) {
		print "gethostip: lookup of $ipaddr\n";
	}
  	eval {
		$SIG{ALRM} = \&sigalrm; 
 		alarm(5);
		eval {
			$dnsname = gethostbyaddr(inet_aton($ipaddr), 2);
		};
		alarm(0);
 	};
	alarm(0);
#print "ip: $ipaddr dns: $dnsname\n";
	if ($dnsname) {
		$dnscache{$ipaddr} = $dnsname;
		if ($debug) {
			print "gethostip: found $dnsname\n";
		}
	}
	else {
		$dnscache{$ipaddr} = $ipaddr;
		if ($debug) {
			print "gethostip: did not find $ipaddr\n";
		}
	}
	return($dnsname);
}

sub sigalrm {
	#alarm(0);
	#print "ALARM TIMEOUT\n";
}


# 
# writeSynReport($i, $syns, $synacks, $fins, $rsts, $tcpto, $tcpfrom, $total, 
# 	$icmpcount, $ports, $portstring, $percent, $wopinion);
#
# similar function in omupdate.pl
#
sub writeSynReport
{
	my $ipsrc = $_[0];
	my $syns = $_[1];
	my $synacks = $_[2];
	my $fins = $_[3];
	my $rsts = $_[4];
	my $tcpfrom = $_[5];
	my $tcpto = $_[6];
	my $total = $_[7];
	my $icmpcount = $_[8];
	my $ports = $_[9];
	my $portstring = $_[10];
	my $percent = $_[11];
	my $wflag = $_[12];
	my $oflag = "";
	my $rflag = "";
	my $mflag = "";
	my $eflag = "";
	my $WORMflag = 0;
	my $JUSTWflag = 0;
	my $i;
	my $rs;
	my $total = $tcpfrom + $tcpto;	# total tcp pkts exchanged
	my $tcontrol = $syns + $fins + $rsts;
	my $sawtwo = 0;
	my $nps="";

     	@ps = split(/,/,$portstring);

	# just W flag
	if ($wflag eq "W") {
		$JUSTWflag = 1;
		$Wflagwork = $tcontrol/($total * 1.0) + $Wflagwork;
	}
	# do compute of remaining weights and flags

	$allfinpercent = 0;
	$allresetpercent = 0;
	if ($syns != 0) {
		$allfinpercent = ($fins / ($syns * 1.0)) + $allfinpercent;
		$allresetpercent = ($rsts / ($syns * 1.0)) + $allresetpercent;
	}
	$allrecvpercent = 0; 
	if ($tcpfrom != 0) {
		$allrecvpercent = ($tcpto / ($tcpfrom * 1.0)) + $allrecvpercent;
	}

	# 1. compute E, the quadratic weight
	# (sins - fins) * (rst + icmp)
	my $quadw = ($syns - $fins) * ($rsts + $icmpcount);
	if ($quadw > $econstant) {
		$eflag = "E";
		$Ecount++;
	}

	# Ouch flag - too few fins
	# 5. syns high (> 20), fins low < $ouchfins).  O is for Ouch.
	# really means no fins.
	if (($syns > 20) && ($fins < $ouchfins)) {
		$oflag = "O";
		$Ocount++;
	}

	# 6. no FINS, some resets flag: R is for Resets.
	if (($fins == 0) && ($rsts > $Rresets)) {
		$rflag = "R";
		$Rcount++;
	}
	# 7. no packets returned flag: M is for (keeping) Mum.
	if ($tcpto == 0) {
		$mflag = "M";
		$Mcount++;
	}

	# calculate combo flags

	# WO
	if (($wflag eq "W") && ($oflag eq "O")) {
		$WOcount++;
		$sawtwo = 1;
	}
	# EW
	if (($eflag eq "E") && ($wflag eq "W")) {
		$EWcount++;
		$sawtwo = 1;
	}
	# WOR	
	if (($wflag eq "W") && ($oflag eq "O") && ($rflag eq "R")) {
		$WORcount++;
		$sawtwo = 1;
	}
	# WORM
	if (($wflag eq "W") && ($oflag eq "O") && ($rflag eq "R") && ($mflag eq "M")) {
		$WORMcount++;
		$sawtwo = 1;
		$WORMwork1 = $tcontrol/($total * 1.0) + $WORMwork1;
		$WORMworkprime = $tcontrol/($total * 1.0) + $WORMworkprime;
		$WORMfinpercent = ($fins / ($syns * 1.0)) + $WORMfinpercent;
		$WORMresetpercent = ($rsts / ($syns * 1.0)) + $WORMresetpercent;
		$WORMrecvpercent = ($tcpto / ($tcpfrom * 1.0)) + $WORMrecvpercent;
		$WORMflag = 1;    # WORM set
	}
	# WOM
	if (($wflag eq "W") && ($oflag eq "O") && ($mflag eq "M")) {
		$WOMcount++;
		$sawtwo = 1;
	}
	# WM
	if (($wflag eq "W") && ($mflag eq "M")) {
		$WMcount++;
		$sawtwo = 1;
	}
	# WRM
	if (($wflag eq "W") && ($rflag eq "R") && ($mflag eq "M")) {
		$WRMcount++;
		$sawtwo = 1;
	}

	if ($sawtwo) {
		$comboTotal++;
	}
	
	$rs = "$ipsrc: s=$syns: sa=$synacks: f=$fins: r=$rsts: total=$total($tcpfrom:$tcpto): i=$icmpcount\n";
	$rs = $rs . "\tweights/flags: ";
	$rs = $rs . "work=$percent%, Quad=:$quadw, Flags:($eflag$wflag$oflag$rflag$mflag)\n";
	my $flagstring = "($eflag$wflag$oflag$rflag$mflag)";

	#storeIpAddress($ipsrc, $WORMflag);
	# Wflag is good enough, bin IPs on that basis (if set)
	storeIpAddress($ipsrc, $JUSTWflag);

	# now loop through ports

	$rs = $rs . "\tdst ports=$ports: (dst port, count)\n";
	$pindex = 0;
	for ($i = 0; $i < $ports; $i++) {
		$pindex = $i * 2;
		$rs = $rs . "\t\tport[$i]=($ps[$pindex], $ps[$pindex+1])\n";

		# calculate some global stats. 
		# W flag port counts vs non-W flag port counts
		# also for non-W flag, do average work weight for
		# ports 80/25 as matter of interest
		if ($JUSTWflag) {
			$WORMipports{$ps[$pindex]} += $ps[$pindex+1];
		}
		# W flag is not set
		else {
			if ($ps[$pindex] == 80) {
				$port80work = $tcontrol/($total * 1.0) + $port80work;
				$port80count++;
			}
			elsif ($ps[$pindex] == 25) {
				$port25work = $tcontrol/($total * 1.0) + $port25work;
				$port25count++;
			}
			$allipports{$ps[$pindex]} += $ps[$pindex+1];
		}
	}
	portSig($ipsrc, 
		$flagstring, 
		$percent, 
		$ports, 
		$ps, 
		$tcpfrom,
		$syns,
		$synacks,
		$fins,
		$rsts,
		$tcpfrom,
		$tcpto,
		$total,
		$icmpcount);
	#$portSigReport = $portSigReport . "$ipsrc: \t$nps\n";
	$rs = $rs . "\t\t$nps\n";
	return($rs);
}


# you must be a worm if 
#
# do worms have red necks?
#
# print worm EWORM flag and weight guide

sub tisaworm 
{
	print "ourmon tcpworm syn tuple report at ($start_time):\n"; 
	print "\ttuples are output here iff for an ip src: SYNS - FINS > 20\n\n";
	print "syn tuple syntax: ip src: syns: fins: resets: tcp pkts total(sent/returned): icmp errors:\n";
	print "\tweight schemes, flags: (EWORM)\n";
	print "\tport tuples (TCP dst port/count of pkts)\n\n";

	print "Flag guide: it might be a worm if:\n";
	print "\tE is for Errors: quadratic weight: (SYNS - FINS) * (ICMP errors + RSTS) is > $econstant:\n";
	print "\tWflag (w/W): work (tcp control/total pkts) is high: W > 90%,  w = > 50%\n";
	print "\tOuch flag (O): too few fins: SYNS > 20, FINS < $ouchfins\n";
	print "\tResets flag (R): no FINS, some RSTS > $Rresets\n";
	print "\tM is for mum flag: no TCP packets returned to ip src\n";
	print "\tWhat's that spell?  WORM (or EWORM)\n\n";
	print "note: the syn tuple is now sorted as follows:\n";
	print "\tIt is sorted by the work weight, therefore 100% at the top:\n";
	print "\twithin 100%, tuples are sorted by the syn count\n"; 
	print "\tafter 100%, tuples are NOT sorted by the syn count (just the weight)\n\n";
}

sub finalstats
{
	my $t;

	print "#########################################################################\n";
	print "final stats: \n";
	print "#########################################################################\n";

	print "total tuples: $totaltuples\n";
	print "#########################################################################\n";
	print "print simple flags:\n";
	print "\ttotal E flags: $Ecount\n";
	print "\ttotal W flags: $Wcount\n";
	print "\ttotal w flags: $wcount\n";
	print "\ttotal O flags: $Ocount\n";
	print "\ttotal R flags: $Rcount\n";
	print "\ttotal M flags: $Mcount\n";
	print "#########################################################################\n";
	print "print combo flags:\n";
	print "\ttotal WORM flags: $WORMcount\n";
	print "\ttotal WOR flags: $WORcount\n";
	print "\ttotal WOM flags: $WOMcount\n";
	print "\ttotal WRM flags: $WRMcount\n";
	print "\ttotal WO flags: $WOcount\n";
	print "\ttotal WM flags: $WMcount\n";
	print "\ttotal EW flags: $EWcount\n";
	print "\ttotal combo flags (2 or more, at least one W): $comboTotal\n";
	print "#########################################################################\n";
	print "averages/weight info:\n";
	print "\ttotal worm metric count (S-F>C): $WWcount\n";

	if ($totaltuples == 0) {
		print "\ttotal work average for all tuples: 0%\n";
	}
	else {
		my $t = int($allwork1/($totaltuples*1.0) * 100);
		print "\ttotal work average for all tuples: $t%\n";
	}

	if ($totaltuples == 0) {
		print "\ttotal work prime (s+r/total) average for all tuples: 0%\n";
	}
	else {
		my $t = int($allworkprime/($totaltuples*1.0) * 100);
		print "\ttotal work prime (s+r/total) average for all tuples: $t%\n";
	}

	if ($WORMcount == 0) {
		print "\ttotal work average for all WORM tuples: 0%\n";
	}
	else {
		my $t = int($WORMwork1/($WORMcount*1.0) * 100);
		print "\ttotal work average for all WORM tuples: $t%\n";
	}

	if ($Wcount == 0) {
		print "\ttotal work average for all W tuples: 0%\n";
	}
	else {
		my $t = int($Wflagwork/($Wcount*1.0) * 100);
		print "\ttotal work average for all W tuples: $t%\n";
	}

	if ($WORMcount == 0) {
		print "\ttotal work prime average for all WORM tuples: 0%\n";
	}
	else {
		my $t = int($WORMworkprime/($WORMcount*1.0) * 100);
		print "\ttotal work prime average for all WORM tuples: $t%\n";
	}

	# fin percent
	if ($totaltuples == 0) {
		print "\ttotal fin percent average for all tuples: 0%\n";
	}
	else {
		my $t = int($allfinpercent/($totaltuples*1.0) * 100);
		print "\ttotal fin percent average for all tuples: $t%\n";
	}

	if ($WORMcount == 0) {
		print "\ttotal fin percent average for all WORM tuples: 0%\n";
	}
	else {
		my $t = int($WORMfinpercent/($WORMcount*1.0) * 100);
		print "\ttotal fin percent average for all WORM tuples: $t%\n";
	}

	# tcp recv/sent percent
	if ($totaltuples == 0) {
		print "\ttotal recv (tcpto/tcpfrom) percent average for all tuples: 0%\n";
	}
	else {
		my $t = int($allrecvpercent/($totaltuples*1.0) * 100);
		print "\ttotal recv (tcpto/tcpfrom) percent average for all tuples: $t%\n";
	}

	if ($WORMcount == 0) {
		print "\ttotal recv percent average for all WORM tuples: 0%\n";
	}
	else {
		my $t = int($WORMrecvpercent/($WORMcount*1.0) * 100);
		print "\ttotal recv percent average for all WORM tuples: $t%\n";
	}

	# reset percent
	if ($totaltuples == 0) {
		print "\ttotal reset percent (reset/syn) average for all tuples: 0%\n";
	}
	else {
		my $t = int($allresetpercent/($totaltuples*1.0) * 100);
		print "\ttotal reset percent (reset/syn) average for all tuples: $t%\n";
	}

	if ($WORMcount == 0) {
		print "\ttotal reset percent average for all WORM tuples: 0%\n";
	}
	else {
		my $t = int($WORMresetpercent/($WORMcount*1.0) * 100);
		print "\ttotal reset percent average for all WORM tuples: $t%\n";
	}

	if ($port80count == 0) {
		print "\ttotal port 80 percent average for non Wflag tuples: 0%\n";
	}
	else {
		my $t = int($port80work/($port80count*1.0) * 100);
		print "\ttotal port 80 percent average for non Wflag tuples: $t%\n";
	}

	if ($port25count == 0) {
		print "\ttotal port 25 percent average for non Wflag tuples: 0%\n";
	}
	else {
		my $t = int($port25work/($port25count*1.0) * 100);
		print "\ttotal port 25 percent average for non Wflag tuples: $t%\n";
	}

	# figure out numeric distance of IP source addresses
	# synthetic count tuples for RRDTOOL graphs
	if (xtramon) {
		open(tfd, ">$monlite2") || die "cannot open $monlite2: $!\n";
		print tfd "wsflag: $Ecount: $Wcount: $wcount: $Ocount: $Rcount: $Mcount:\n";
		print tfd "wcflag: $WORMcount: $WORcount: $WOMcount: $WRMcount: $WOcount: $WMcount:\n";
		close(tfd);
	}

	# first print out some stats
	print "#########################################################################\n";
	# port stats divided into Wflag ports and the rest
	printPorts();
	print "#########################################################################\n";
	# Wflag IP bins
	printIp();

	# print the port sig report, possibly optionally to the portreport file
	print "#########################################################################\n";
	printPortSigs(STDOUT);
	if ($portreport) {
		open(tfd, ">$portreport") || die "cannot open $portreport: $!\n";
		printPortSigs(tfd);
		printUpsSigs(tfd);
		close(tfd);
		open(tfd, ">$portreport2") || die "cannot open $portreport2: $!\n";
		printPortSigs2(tfd);
		close(tfd);
	}

	# print out unique port signatures -- we need the db database for this
	printUpsSigs(STDOUT);
}

# print out sorted port stats
sub printPorts
{
	my $i;
	my $port;
	my $count;

	#@Allsortedports  = sort allipports_by_value keys(%allipports);
	#@WORMsortedports  = sort WORMipports_by_value keys(%WORMipports);
	# print worm ports
	print "WORM tuples: top $topnports ports/counts:\n";
	for ( $i = 0; $i < $topnports; $i++) {
		$port = $WORMsortedports[$i];
		if ($port eq "") {
			next;
		}
		$count = $WORMipports{$port};
		print "\tport[$port]: $count\n";
	}
	print "\n";
	# print all ports
	print "all tuples: top $topnports ports/counts:\n";
	for ( $i = 0; $i < $topnports; $i++) {
		$port = $Allsortedports[$i];
		if ($port eq "") {
			next;
		}
		$count = $allipports{$port};
		print "\tport[$port]: $count\n";
	}
}


# bin IPs according to first byte as crude method for
# detecting similarity
#
# note: two bins
#	W flag set
#	not set (not printing this -- may be buggy)
#
sub storeIpAddress
{
	my $ip = $_[0];
	my $wflag = $_[1];
	my @words;
	my $ipid;

	# just keep the first byte
	$ip=~s/\./:/g;
	@words = split(/:/, $ip);
	# if class C, then 3 bytes, else 2 bytes
	if ($words[0] >= 192) {
		$ipid = $words[0] . '.' . $words[1] . '.' . $words[2];
	}
	else {
		$ipid = $words[0] . '.' . $words[1];
	}
#print "$ip/$ipid, $wflag\n";
	if ($wflag) {
		if ($WORMipaddrs{$ipid}) {
			$WORMipaddrs{$ipid} += 1;
		}
		else {
			$WORMipaddrs{$ipid} = 1;
		}
	}
	else {
		if ($ipaddrs{$ipid}) {
			$ipaddrs{$ipid} += 1;
		}
		else {
			$ipaddrs{$ipid} = 1;
		}
	}
}

#
# print out IP bins
# currently only doing Wflag related IP addresses.
#
sub printIp
{
	my $i = 0;
	my $ipaddr;
	my $count;

	#@Allsortedipaddrs  = sort allipaddrs_by_value keys(%ipaddrs);
	#@WORMsortedipaddrs  = sort WORMipaddrs_by_value keys(%WORMipaddrs);

	# print worm ipaddrs
	print "W flag IP address tuples: < $topnipaddr (first two bytes of address)/counts:\n";
	for ( $i = 0; $i < $topnipaddr; $i++) {
		$ipaddr = $WORMsortedipaddrs[$i];
		if ($ipaddr eq "") {
			next;
		}
		$count = $WORMipaddrs{$ipaddr};
		print "\t$ipaddr: $count\n";
	}
	print "\n";

	#print "\nall IP address tuples other than WORM: $topnipaddr (two bytes)/counts:\n";
	#for ( $i = 0; $i < $topnipaddr; $i++) {
	#	$ipaddr = $Allsortedipaddrs[$i];
	#	if ($ipaddr eq "") {
	#		next;
	#	}
	#	$count = $ipaddrs{$ipaddr};
	#	print "\t$ipaddr: $count\n";
	#	$i++;
	#}
	#print "\n";
}

sub dumpIpaddrs {
	foreach $key (keys %ipaddrs) {
		printf("key %s: count %s\n", $key, $ipaddrs{$key});
	}
}

#
# portSig
# take the unsorted port tuple and sort it
# according to the following math. notion
#
# 1. sort acc. to the port value itself from decreasing to increasing
# 2. determine the total port count frequency
# 3. express the count as a fraction of that frequency.
# 4. store the ipsrc, flagstring, percent(work) and portsig
#	in an associative array.
# portSig($ipsrc, $flagstring, $percent, $ports, $ps) 
#

sub portSig
{
	my $ipsrc=$_[0];		# ip src
	my $flags=$_[1];		# flags combined into one string
	my $percent=$_[2];		# work weight
	my $ports=$_[3];		# number of ports
	my $ps=$_[4];			# unprocessed as of yet port string
	my $tcpfrom=$_[5];		# pkts sent by this ip src
	my $syns=$_[6];			
	my $synacks=$_[7];			
	my $fins=$_[8];
	my $rsts=$_[9];
	my $tcpfrom=$_[10];
	my $tcpto=$_[11];
	my $total=$_[12];
	my $icmpcount=$_[13];

	my $nps = "";			# nps is a signature (ports/frequency tuples)
	my $ups = "";			# ports only, no counts/frequency value
	my @pset;
	my @sp;
	my $pcount = 0;
	my $i;
	my @tuple;
	my $tmpx;

	# first compute the total port count
 	# meaning: the total number of dst sampled packets.	
	# which is NOT necessarily the total number of packets sent.
	for ($i = 0; $i < $ports; $i++) {
		my $pindex = $i * 2;
		$pcount += $ps[$pindex+1];
	}
	# now go thru and compute a percent for the 
	# destination port count (hits/total) and store it in an hash
	# array
	for ($i = 0; $i < $ports; $i++) {
		my $pindex = $i * 2;
		# ignore port if count is 1
		#if ($ps[pindex+1] <= 1) {
		#	next;
		#}
		if ($pcount) {
			$pset{$ps[$pindex]} = int(($ps[$pindex+1] * 100.0)/ $pcount);
		}
	}

	# now sort the array based on the port number itself. 
	# sortedPorts is a list of the ports themselves
	@sp  = sort p_by_key keys(%pset);

	# now loop thru the ports
	# putting the ports into one string that is a signature tuple
	for ($i = 0; $i < $ports; $i++) {
		$nps = $nps . "[$sp[$i],$pset{$sp[$i]}]";
		$ups = $ups . "[$sp[$i]]";			# ports only
	}

	foreach $key (keys(%pset)){
#print "pset:", $key, $pset{key}, "\n";
		delete $pset{$key};
	}
	#foreach $key (keys(%sp)){
	#	delete $sp{$key};
	#}
	$nps = $nps . ")";
	$portSignature{$ipsrc} = $nps;		# key is ip, value is ports
	$portSignature_flags{$ipsrc} = $flags;	# per ip src, flags 
	$portSignature_work{$ipsrc} = $percent; # per ip src, work
	$portSignature_sample{$ipsrc} = join("/", $pcount, $tcpfrom); 
	# synacks divided by syns is expressed as a percent.
	# note: synacks are counted as syns too.  
	if ($syns == 0) {
		$portSignature_server{$ipsrc} = 0;
	}
	else {
		$portSignature_server{$ipsrc} = int(($synacks * 100.0) / $syns);
	}

################################################################################ 
	# experiments coming out in "hidden" portreport2.txt
	# syns+resets/total
	if ($total == 0) {
		$total = 1;
	}
	$portSignature_work2{$ipsrc} = 
		int((( $syns + $rsts ) * 100)/$total); 

	# syns - fins
	$portSignature_work3{$ipsrc} = $syns - $fins; 

	# syns - fins * (icmp + resets)
	$portSignature_work4{$ipsrc} = ($syns - $fins) * ($icmpcount + $rsts);

	# total sent/total sent+total recv
	$tmpx = $tcpfrom + $tcpto;
	if ($tmpx == 0) {
		$tmpx = 1;
	}
		
	$portSignature_work5{$ipsrc} = 
		int(($tcpfrom*100.0)/$tmpx);
################################################################################ 

	# if work weight is above a configured constant,
	# we compute total signature stats and save info in a dbm database
	#
	# key=$ups, value=$ipsrc, $start_time, instance count
	# $ups is a set of dst ports, minus pkt counts.
	#
	# tuple plan for value
	# (ipsrc, time, instance_count)
	# ipsrc is colon separated (ip:ip:ip)
	# 
	if ( $percent >= $ups_weight ) {
		if ($upsSig{$ups}) {
			# take it apart, and then
			# split via commas
			@tuple = split(",", $upsSig{$ups});

			$ups_iplist = $tuple[0];
			$ups_time = $tuple[1];

			# instance count of this signature (ports only) in this particular tcpworm.txt
			$ups_instance = $tuple[2];

			# join it back together
			# ipsrc are separated by colon
			$ups_iplist = $ups_iplist . ":$ipsrc";
			$ups_instance++;
#print "XXX: $ups: $ups_iplist, $ups_time, $ups_instance\n";
			# note: time string has colons in it, we use comma
			# as delimiter
			$upsSig{$ups} = join(",", $ups_iplist, $ups_time, $ups_instance);
		}
		# no signature so make one
		# value is: (ipsrc, start time, instance count == 1)
		else {
			$upsSig{$ups} = join(",", $ipsrc, $start_time, 1); 
		}
	}
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

# printPortSigs
#
#	print out the port signature info
#
#	note parameter -- open handle where to print info
#
#	e.g., portSignature{$ipsrc} = $nps;
#	and other portSignature arrays used here.

sub printPortSigs
{
	my $fd = $_[0];
	my $ip;
	my $sbuf;
	my @sports;
	my $i;

	# sort by ip src in ascending order

	@sports = sort p_by_ipkey keys(%portSignature);

	print $fd "port signatures:\n";
	# ip src      flags    work weight   port sig tuples 1..10
	$sbuf = sprintf("%-16.16s %-8.8s %5s: %5s: %9s: %s\n\n", "ip src:", "flags", "work", "SA/S", "dst/total", "port signature(dst port, frequency)"); 
	print $fd $sbuf;

	$i = 0;
	while (1) {
		$ip = $sports[$i];
		if ($ip eq "") {
			return;
		}

		$sbuf = sprintf("%-16.16s %-10.10s %3d:   %3d: %-10.10s %s", 
			"$ip", 
			$portSignature_flags{$ip},   
			$portSignature_work{$ip} , 
			$portSignature_server{$ip} , 
			$portSignature_sample{$ip},
			$portSignature{$ip});
		print $fd "$sbuf\n";
		$i++;
	}
}

# work2 syns+resets/total
# work3 syns-fins
# work4 syns-fins * errors
# work5 sent/sent+recv
# experimental

sub printPortSigs2
{
	my $fd = $_[0];
	my $ip;
	my $sbuf;
	my @sports;
	my $i;

	# sort by ip src in ascending order

	@sports = sort p_by_ipkey keys(%portSignature);

	print $fd "port signatures:\n";
	# ip src      flags    work weight   port sig tuples 1..10
	$sbuf = sprintf("%-16.16s %-8.8s (%4s: %5s:   %3s:  %5s: %5s) %5s: %9s: %s\n\n", "ip src:", "flags", "work", 
		"s+r/t", "s-f", "s-f*e", "S/S+R",
		"SA/S",
		"dst/total", "port signature(dst port, frequency)"); 
	print $fd $sbuf;

	$i = 0;
	while (1) {
		$ip = $sports[$i];
		if ($ip eq "") {
			printWorkHistogram($fd);
			printWork2Histogram($fd);
			printWork5Histogram($fd);
			return;
		}

		$sbuf = sprintf("%-16.16s %-10.10s (%3d: %3d: %6d: %6d: %3d:)     %3d %-10.10s %s", "$ip", 
			$portSignature_flags{$ip},   
			$portSignature_work{$ip} , 
			$portSignature_work2{$ip} , 
			$portSignature_work3{$ip} , 
			$portSignature_work4{$ip} , 
			$portSignature_work5{$ip} , 
			$portSignature_server{$ip},
			$portSignature_sample{$ip},
			$portSignature{$ip});
		print $fd "$sbuf\n";
		storeWorkHistogram($ip);
		storeWork2Histogram($ip);
		storeWork5Histogram($ip);
		$i++;
	}

}


# Ups - unique port sig
#
# print out port sigs that are unique
# on this sample pass.  We essentially
# boil the single ip instances down into classes
# as a result.  E.g., maybe you are seeing 10 different
# sasser worms or related things from 10 diff. ip srcs.
#
sub
printUpsSigs
{
	my $fd = $_[0];
	my $key;
	my $value;
	my @newtuple;
	my @oldtuple;
	my $oldv;
	my $sbuf;
	my @iplist;
	my $ip;
	my $itime;
	my $instances;
	
	# open the database
	dbmopen(%map, $dbmfile, 0666);

	# print out a report header
	print $fd "\n\nUnique port signature report:\n";
	$sbuf = sprintf("port signature > %d% work weight.\n", $ups_weight);
	print $fd $sbuf;
	$sbuf = sprintf("if portsig is old, first time seen, and original ip sources are given.\n\n");
	print $fd $sbuf;
	$sbuf = sprintf("%-7.7s %-14.14s %s\n", "New/Old", "instances(now)", "port_signature");
	print $fd $sbuf;
	$sbuf = sprintf("-------------------------------------\n\n");
	print $fd $sbuf;
 
	# loop thru the keys in the upsSig hash array
	# the keys are a unique port signature for this run
	#
	# $key is $ups, a set of ports
	# 
 	foreach $key (keys %upsSig) {
		# get the value associated with the key
 		$value = $upsSig{$key};
		#print "$key $value\n";
		@newtuple = split(",", $value);
		# newtuple[0] = ipsrcs, time, instances
		$ip = $newtuple[0];
		$itime = $newtuple[1];
		$instances = $newtuple[2];
		#print "$newtuple[0],$newtuple[1], $newtuple[2]\n";
		# see if the key is in the database, else we must store it
		#
		if ($map{$key}) {
			# fetch database stored instance
			$oldv = $map{$key};	
			#print "oldv is $oldv\n";

			$sbuf = sprintf("%-7.7s %-14.14s %s\n", "Old", @newtuple[2], $key);	
			print $fd "$sbuf";
			@oldtuple = split(",", $oldv);
			$sbuf = sprintf("\toriginal time seen: @oldtuple[1]\n");
			print $fd "$sbuf";
			$sbuf = sprintf("\toriginal ip_srcs: ");
			print $fd "$sbuf";
			@iplist = split(":", $oldtuple[0]);
			$i = 0;
			$j = 0;
			while($iplist[$i]) {
				if ($j < 4) {
					print $fd "$iplist[$i++] ";	
					$j++;
				}
				else {
					$j = 0;
					print $fd "\n\t\t";
				}
			}
			print $fd "\n";
		}
		# key is not in the database and is 
		# therefore a new worm port sig
		else {
			# store in database
			$map{$key} = $value;	

			# info is new
			$sbuf = sprintf("%-16.16s %s\n", $ip, $key);	
			print $fd "$sbuf";
			if ($eventflag) {
		 		#storeEvent($eventfile, "new worm signature from: $value :", $sbuf);				
		 		storeEvent($eventfile, "new worm signature from:", $sbuf);				
			}
		}
	}
	dbmclose(%map);
}

sub 
p_by_ipkey 
{
        if (ipToNumber($a) < ipToNumber($b)) { 
                -1;
        }
        elsif (ipToNumber($a) == ipToNumber($b)) {
                0;
        }
        elsif (ipToNumber($a) > ipToNumber($b)) {
                1;
        }
}

sub
ipToNumber
{
	$ipaddr = $_[0];
	@w = split(/\./, $ipaddr);
	$hip = ($w[0] << 24) | ($w[1] << 16) | ($w[2] << 8) | $w[3];
	return($hip);	
}

sub sortit()
{
	# ipports
	@Allsortedports  = sort allipports_by_value keys(%allipports);
	@WORMsortedports  = sort WORMipports_by_value keys(%WORMipports);

	# ipaddrs
	@Allsortedipaddrs  = sort allipaddrs_by_value keys(%ipaddrs);
	@WORMsortedipaddrs  = sort WORMipaddrs_by_value keys(%WORMipaddrs);
}

sub allipaddrs_by_value 
{
        if ($ipaddrs{$a} < $ipaddrs{$b}) {
                1;
        }  
        elsif ($ipaddrs{$a} == $ipaddrs{$b}) {
                0;
        }
        elsif ($ipaddrs{$a} > $ipaddrs{$b}) {
                -1;
        }
}

sub WORMipaddrs_by_value 
{
        if ($WORMipaddrs{$a} < $WORMipaddrs{$b}) {
                1;
        }  
        elsif ($WORMipaddrs{$a} == $WORMipaddrs{$b}) {
                0;
        }
        elsif ($WORMipaddrs{$a} > $WORMipaddrs{$b}) {
                -1;
        }
}

sub allipports_by_value 
{
        if ($allipports{$a} < $allipports{$b}) {
                1;
        }  
        elsif ($allipports{$a} == $allipports{$b}) {
                0;
        }
        elsif ($allipports{$a} > $allipports{$b}) {
                -1;
        }
}

sub WORMipports_by_value 
{
        if ($WORMipports{$a} < $WORMipports{$b}) {
                1;
        }  
        elsif ($WORMipports{$a} == $WORMipports{$b}) {
                0;
        }
        elsif ($WORMipports{$a} > $WORMipports{$b}) {
                -1;
        }
}


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


# TBD. not used yet
# goal: create an "application flags" field in the portsig report
#

# bittorrent 6881..6889
# kazaa 1214
# edonkey 4661, 4662, 4665
# gnutella 6346 6347
# winmx 6699
#
# B - bittorrent
# K - kazaa
# E - edonkey
# G - gnutella
# M - microsoft (virus in fact aimed at Microsoft file share)
# W - winmx
sub
idports
{
	$port = $_[0];

	if ($port >= 6881 && $port <= 6889) {
		return("B");
	}
	elsif ($port == 1214) {
		return("K");
	}
	elsif ($port == 4661 || $port == 4662 || $port == 4665 ) {
		return("E");
	}
	elsif ($port == 6346 || $port == 6347) {
		return("G");
	}
	elsif ($port == 139 || $port == 135 || $port == 445 ) {
		return("M");
	}
	elsif ($port == 6699 ) {
		return("W");
	}
}

# given a work weight store a count
# in 10 possible bins to give us a rough
# idea of the work scatter
sub
storeWorkHistogram
{
	my $ip = $_[0];
	my $work = $portSignature_work{$ip}; 
 	my $workindex = $work - ($work % 10);
	$workCount{$workindex} = $workCount{$workindex} + 1;
}


sub
printWorkHistogram
{
	my $fd = $_[0];
	my $index = 0;

	print $fd "\nwork histogram:\n";
	for ($index = 0; $index <= 100;  $index = $index + 10) {
		print $fd "$index: $workCount{$index}\n";
	}
}

sub
storeWork2Histogram
{
	my $ip = $_[0];
	my $work = $portSignature_work2{$ip}; 
 	my $workindex = $work - ($work % 10);
	$work2Count{$workindex} = $work2Count{$workindex} + 1;
}


sub
printWork2Histogram
{
	my $fd = $_[0];
	my $index = 0;

	print $fd "\nwork2 (s+r/t) histogram:\n";
	for ($index = 0; $index <= 100;  $index = $index + 10) {
		print $fd "$index: $work2Count{$index}\n";
	}
}

sub
storeWork5Histogram
{
	my $ip = $_[0];
	my $work = $portSignature_work5{$ip}; 
 	my $workindex = $work - ($work % 10);
	$work5Count{$workindex} = $work5Count{$workindex} + 1;
#print "GOO $ip $work $workindex $workCount5{$workindex}\n";
}

sub
printWork5Histogram
{
	my $fd = $_[0];
	my $index = 0;

	print $fd "\nwork5 (total sent/total) histogram:\n";
	for ($index = 0; $index <= 100;  $index = $index + 10) {
		print $fd "$index: $work5Count{$index}\n";
	}
}
