#! /usr/bin/perl

require 'bigint.pl';

#
# input params:
#
# % ombatchipsrc.pl logfile web_ipsrc  web_ipdst
#
# logfile - name of logfile to parse as input
# web_ipsrc - web output file for ip srcs
# web_ipdst - web output file for ip dsts
#
# goal of program:
#
# take logfile as input (the topN ip log file)
# and produce two reports, 1. top of top ip srcs,
# and 2. top of top ip destinations.  
# We add all the ip src and ip dst together ( as separate
# cases ) and report on the biggest ip src/dst in terms of flows.
# Sort by bytes, and report the number of ourmon flows (which
# are typically 30 second sample periods).
#
# flow database scheme: 
#
# ipflow_src{ ip_src } = cumulative amount
# ipflow_src_fcounts { ip_src } = total number of 30 sec flows
# ipflow_dst{ ip_dst } = cumulative amount
# ipflow_dst_fcounts { ip_dst } = total number of 30 sec flows
#
# thus we map ip_src and ip_dst to a cumulative bits/sec measure
#	and for each ip_src/ip_dst produce a total flow count;
#	that is, how many times we showed up in the topN logs.
#
# output note: the total cumulative output must be greater than
# 1 Mbyte, else the result will be 0; that is, we only do megabytes,
# not kbytes 
#
# There are NO dependencies.  It is assumed that omupdate.pl
# is creating an IP topN logfile of course.

$debug = 0;
# tweakable param
# max number of things to store in output report
my $asciitopn = 35;
#############################################

# use socket library because of gethostbyname
use Socket;
# set mon.lite input file - this is where it is found
use Time::Local;
# comment in for ctime, note that it overrides localtime
use Time::localtime;

if ($#ARGV != 2) {
	print "usage: ombatchipsrc.pl input src_output dst_output\n";
	die "need 3 input arguments\n";
}

# month list - look up internal value, based on external string tag 
#
%month = ('Jan', 0, 'Feb', 1, 'Mar', 2, 'Apr', 3, 'May', 4, 'Jun',
             5, 'Jul', 6, 'Aug', 7, 'Sep', 8, 'Oct', 9, 'Nov', 10, 'Dec', 11);

# ip topn log file
my $iplogfile = $ARGV[0];

# output files
my $ip_srcascii = $ARGV[1];
my $ip_dstascii = $ARGV[2];

################################################### 
$start_time=`date`;
$start_time=~ s/\n//g;

$unixtime=time;

# parse the input file and put info in 
# associative arrays
ipflow_parse();

# sort it
ipflow_sortit();

# print it
ipflow_printit();

exit(0);

######################################################################
# functions

# globals
#	$iplogfile - input log file
#	%ipflow
#	%ipflow_time
#
# ipflow_parse - parse the syslog file output by omupdate.pl
#

sub ipflow_parse() { 
	
	my $curline;
	my $time;
	my $timeval;
	my $words;
	my $count;
	my $countpairs;
	my $fid;
	my $lhs_ip, $lhs_port, $rhs_ip, $rhs_port, $proto;
	my $realtuples;

	open(FILE, $iplogfile) || die "can't open $iplogfile: $!\n";
	while(<FILE>) {

		s/  / /g;
		$curline = $_;
		@time = split(/ /, $curline);
		#print "$time[0]\n";
		#print "$time[1]\n";
		#print "$time[2]\n";
		$curline =~ s/ //g;    
		$curline =~ s/\n//g;
		# convert greater than to zilch, until we figure out
		# how to escape it (we put it back later)
		# this is probably unnecessary.
		$curline =~ s/\>//g;

		@words = split(/:/, $curline);

		#if ($words[3] eq "topn_ip") {
			# save the time stamp, and shift the array
			# past it and the tag
			# timeval is count in seconds.
			$timeval = getTime($time[0], $time[1], $time[2]);
			$x = shift(@words);	# time 1 (ignored)
			$x = shift(@words);	# time 2	
			$x = shift(@words);	# time 3
			$x = shift(@words);	# tag 
			$realtuples = shift(@words);	# count of flows
			$count = $#words;	# number of words
			# number of pairs - xtra which we ignore
			$countpairs = $count - 2  / 2;  

			if ($realtuples == 0) {
				next;
			}

			for ($i = 0; $i < $countpairs; ) {
				$fid = $words[$i];
				$fcount = $words[$i+1];
				# parse flowid into components
				($lhs_ip, $lhs_port, $rhs_ip, 
					$rhs_port, $proto) = 
					parse_flowid($fid);
				# if already stored, we don't
				# add a new one, instead add the old values
				update_ipsrc($fcount, $lhs_ip, $rhs_ip);
	#			printf("%d: %s, %s\n", $i, $words[$i],
	#				$words[$i+1]);
				$i = $i + 2;
			}
		#}

	}  # end loop
	close(FILE);
}

# update the associative arrays

sub update_ipsrc {
	my $fcount = $_[0];
	my $ip_src = $_[1];
	my $ip_dst = $_[2];


	# turn count into bytes, not bits
	$fcount = $fcount >> 3;

	# see if id (ip_src) set, if so, update
	if ($ipflow_src{$ip_src}) {
		$ipflow_src{$ip_src} += $fcount;
		$ipflow_src_fcounts{$ip_src}++;
	}
	# else simply store it for 1st time
	else {
		$ipflow_src{$ip_src} = $fcount;
		$ipflow_src_fcounts{$ip_src} = 1;
	}

	# now destination
	if ($ipflow_dst{$ip_dst}) {
		$ipflow_dst{$ip_dst} += $fcount;
		$ipflow_dst_fcounts{$ip_dst}++;
	}
	# else simply store it for 1st time
	else {
		$ipflow_dst{$ip_dst} = $fcount;
		$ipflow_dst_fcounts{$ip_dst} = 1;
	}
}

#sub dumpFlows {
#	foreach $key (keys %ipflow) {
#		printf("key %s: flow %s, time %d\n",          
#			$key, $ipflow{$key}, $ipflow_time{$key});
#	}
#}

# ipflow_src{ ip_src } = cumulative amount
# ipflow_src_fcounts { ip_src } = total number of 30 sec flows
# ipflow_dst{ ip_dst } = cumulative amount
# ipflow_dst_fcounts { ip_dst } = total number of 30 sec flows
# sort the associative arrays
sub ipflow_sortit()
{
	@sortedip_srcflows = sort ip_srcflow_by_value keys(%ipflow_src);
	@sortedip_dstflows = sort ip_dstflow_by_value keys(%ipflow_dst);
}

sub ip_srcflow_by_value {
        if ($ipflow_src{$a} < $ipflow_src{$b}) {
                1;
        }
        elsif ($ipflow_src{$a} == $ipflow_src{$b}) {
                0;
        }
        elsif ($ipflow_src{$a} > $ipflow_src{$b}) {
                -1;
        }
}

sub ip_dstflow_by_value {
        if ($ipflow_dst{$a} < $ipflow_dst{$b}) {
                1;
        }
        elsif ($ipflow_dst{$a} == $ipflow_dst{$b}) {
                0;
        }
        elsif ($ipflow_dst{$a} > $ipflow_dst{$b}) {
                -1;
        }
}

# ipflow_src{ ip_src } = cumulative amount
# ipflow_src_fcounts { ip_src } = total number of 30 sec flows
# ipflow_dst{ ip_dst } = cumulative amount
# ipflow_dst_fcounts { ip_dst } = total number of 30 sec flows
#
# first print src, then destination
# @sortedip_srcflows 
# @sortedip_dstflows
sub ipflow_printit {
	my $flowid;
	my $counts;
	my $bits;
	my $bytes;
	my $lhs_ip, $lhs_port, $rhs_ip, $rhs_port, $proto;
	my $dnsname;
	my $lflow;
	my $sbuf;
	my $flowcount;
	my $mbytesflag = 1;

	# ip src output file
	open(OUTPUT_HANDLE, ">$ip_srcascii") || die "can not open: $!\n";
	$sbuf = sprintf("%-50.50s %-10.10s %12.12s", "ip source", 
		"#instances", "total Mbytes");
	print OUTPUT_HANDLE "$sbuf\n";

	$flowcount = $#sortedip_srcflows;
	if ($flowcount > $asciitopn) {
		$flowcount = $asciitopn; 
	}

	# print out src counts
	for ( $i = 0; $i < $flowcount; $i++) {
		$flowid = $sortedip_srcflows[$i];
		$counts = $ipflow_src_fcounts{$flowid};
		$bytes = int($ipflow_src{$flowid});
		# convert to Mbytes
		$mbytesflag = 1;
		if ($bytes >= 1048576) {
			$bytes = int($bytes / 1048576);
		}
		else {
			$mbytesflag = 0;
		}
		#$bytes = $ipflow_src{$flowid};

		$dnsname = gethostip($flowid);
		if ($dnsname) {
			$flowid = $dnsname; 
		}
		#$lflow = $lhs_ip  . '.' .  $lhs_port . "->" . $rhs_ip . '.' . 
		#	$rhs_port . ':' . '[' . $proto . ']'; 
		
		if ($mbytesflag) {
			$sbuf = sprintf("%-50.50s %-10.10s %12.12s", "$flowid",
				"$counts", "$bytes");
		} else {
			$sbuf = sprintf("%-50.50s %-10.10s %12.12s(kbytes)", "$flowid",
				"$counts", "$bytes");
		}
		print OUTPUT_HANDLE "$sbuf\n";
	}
	close(OUTPUT_HANDLE);

	# ip dst output file
	open(OUTPUT_HANDLE, ">$ip_dstascii") || die "can not open: $!\n";
	$sbuf = sprintf("%-50.50s %-10.10s %12.12s", "ip destination", 
		"#instances", "total Mbytes");
	print OUTPUT_HANDLE "$sbuf\n";

	$flowcount = $#sortedip_srcflows;
	if ($flowcount > $asciitopn) {
		$flowcount = $asciitopn; 
	}
	# print out dst counts
	for ( $i = 0; $i < $flowcount; $i++) {
		$flowid = $sortedip_dstflows[$i];
		$counts = $ipflow_dst_fcounts{$flowid};
		$bytes = int($ipflow_dst{$flowid});
		# convert to Mbytes
		$mbytesflag = 1;
		if ($bytes >= 1048576) {
			$bytes = int($bytes / 1048576);
		}
		else {
			$mbytesflag = 0;
		}

		$dnsname = gethostip($flowid);
		if ($dnsname) {
			$flowid = $dnsname; 
		}
		#$lflow = $lhs_ip  . '.' .  $lhs_port . "->" . $rhs_ip . '.' . 
		#	$rhs_port . ':' . '[' . $proto . ']'; 
		
		if ($mbytesflag) {
			$sbuf = sprintf("%-50.50s %-10.10s %12.12s", "$flowid",
				"$counts", "$bytes");
		}
		else {
			$sbuf = sprintf("%-50.50s %-10.10s %12.12s(kbytes)", "$flowid",
				"$counts", "$bytes");
		}
		print OUTPUT_HANDLE "$sbuf\n";
	}
	close(OUTPUT_HANDLE);
}

sub parse_flowid {
	$fid = $_[0];
	my @words;
	my @tmp2;
	my $lhs_ip, $lhs_port, $rhs_ip, $rhs_port, $proto;

	@words = split(/\./, $fid);
	$lhs_ip = $words[0] . '.' . $words[1] . '.'  .
			$words[2] . '.' . $words[3];
	@tmp2 = split(/\-/, $words[4]);
	$lhs_port = $tmp2[0];
	$rhs_ip = $tmp2[1] . '.' . $words[5] . '.'  .
			$words[6] . '.' . $words[7];
	@tmp2 = split(/\[/, $words[8]);
	$rhs_port = $tmp2[0];
	$tmp2[1] =~ s/\]//;
 	$proto = $tmp2[1]; 
	#print "$lhs_ip\n";
	#print "$lhs_port\n";
	#print "$rhs_ip\n";
	#print "$rhs_port\n";
	#print "$proto\n";
	return($lhs_ip, $lhs_port, $rhs_ip, $rhs_port, $proto);
}

# getTime - parse time stamp and return in unix epoch seconds
#
# large assumption: log file is same year as now,
# although this probably won't hurt much at year wrap time.
# (as the year isn't usually in the log file anyway).
# the only inputs we are paying attention to are month/day/hr/min/sec.
#
sub getTime()
{
	my $t1 = $_[0];  # month
	my $t2 = $_[1];  # day
	my $t3 = $_[2];  # hour:min:sec
	my $time;
	my $ltime;
        #%daylist = ( 0, 'Sun', 1, 'Mon', 2, 'Tue', 3, 'Wed', 
	# 	4, 'Thu', 5, 'Fri', 6, 'Sat');

	# use perl core routine for this
	# get now and simply patch 
	($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = 
			CORE::localtime(time);

	#print "sec ", $sec, "\n";
	#print "min ", $min, "\n";
	#print "hour ", $hour, "\n";
	#print "mday ", $mday, "\n";
	#print "mon ", $mon, "\n";
	#print "year ", $year, "\n";

        @hours = split(/:/, $t3);
	$hour = $hours[0];
	$min = $hours[1];
	$sec = $hours[2];
	$mday = $t2;
	$mon = $month{$t1};

	$time = timelocal($sec,$min,$hour,$mday,$mon,$year);

	$ltime = time;
	#printf("ltime: %s\n", ctime($ltime));
	#printf("time: %s\n", ctime($time));

	return($time);
}

# return dnsname within timeout period of 5 seconds
# note: this may have the odd effect of working the 2nd time.
# but at least we won't get hung here.
#
#sub gethostip {
#	my $ipaddr = $_[0];
#	my $dnsname;
#
##print "$ipaddr\n";
# 	eval {
# 		#alarm(5);
#		$dnsname = gethostbyaddr(inet_aton($ipaddr), 2);
#		#alarm(0);
#	};
#	return($dnsname);
#}

#print OUTPUT_HANDLE "$sbuf\n";
		
#$sbuf = sprintf("%-50.50s %-10.10s %12.12s", 
#	"$flowid", "$counts", "$bytes");
#print OUTPUT_HANDLE "$sbuf\n";

# return dns from ip within timeout period of 5 seconds
#
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
