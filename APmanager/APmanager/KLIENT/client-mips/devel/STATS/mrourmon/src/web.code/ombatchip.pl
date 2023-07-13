#! /usr/bin/perl

#
# ombatchip.pl
#
# input params:
#
# ombatchip.pl report_type fullpath_logfile relative_asciifile
#
# report_type, either icmp or anything else
#
# purpose: sort the log file to find the largest flows
#	in the log file, and write a report to a 
#	text file.  
#
# input: fullpath_logfile - unix log file output produced
#	by omupdate.pl.  This is the top IP flows log,
#	named topn.log (unless the name was changed).
#
# output: relative_asciifile - report
#
# flow database scheme 
# ipflow{ flow id } = amount
# ipflow_time { flow id} = time
#
# thus we map flow_id (string) to (time, amount) pairs
#
# does a cd to web directory.
#

$debug = 0;

# globals for ourmon

################################
# things that may be tweaked
# max number of outputs to store in a report
my $asciitopn = 100;

################################
use Socket;

# set mon.lite input file - this is where it is found
use Time::Local;
# comment in for ctime, note that it overrides localtime
use Time::localtime;

# month list - look up internal value, based on external string tag 
#
%month = ('Jan', 0, 'Feb', 1, 'Mar', 2, 'Apr', 3, 'May', 4, 'Jun',
             5, 'Jul', 6, 'Aug', 7, 'Sep', 8, 'Oct', 9, 'Nov', 10, 'Dec', 11);

my $reporttype = $ARGV[0];

$icmpreport = 0;
if ($reporttype eq "icmp") {
	$icmpreport = 1;
}

# ip topn log file
my $iplogfile = $ARGV[1];

# absolute output filename
my $ipascii = $ARGV[2];

################################################### 
$start_time=`date`;
$start_time=~ s/\n//g;

#  set rrd file names, error log file name, top N png file
#  name.  rrds and error log must have rrdbase as path
######################################################

$unixtime=time;

# parse the file and put info in 
# associative arrays
if ($debug) {
	print "parsing stage\n";
}
ipflow_parse();

#dumpFlows();

# sort it
if ($debug) {
	print "sorting stage\n";
}
ipflow_sortit();

# print it
if ($debug) {
	print "printing stage\n";
}
ipflow_printit();

exit(0);

# end of main

######################################################################
# functions

# globals
#	$iplogfile - input log file
#	%ipflow
#	%ipflow_time
sub ipflow_parse() { 
	
	my $curline;
	my $time;
	my $timeval;
	my $words;
	my $count;
	my $countpairs;
	my $fid;
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
		# how to escape it 
		$curline =~ s/\>//g;

		@words = split(/:/, $curline);

		#if ($words[3] eq "topn_ip") {
			# save the time stamp, and shift the array
			# past it and the tag
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

				# if there is a hit in the ass. cache
				# for the flow id
				if ($ipflow{$fid}) {
	#printf("HIT %s, keys %d\n", $fid, keys(%ipflow));
	#				if ($fcount > $ipflow{$fid}) {
	#printf("replace %s\ old new %d %dn", $fid, $ipflow{$fid}, $fcount);
	#					$ipflow{$fid} = $fcount;
	#printf("keys %d\n", keys(%ipflow));
	#					$ipflow_time{$fid} = $timeval;
	#				}
					# add the flows together (total)
					# we want to know who the big dogs are
					# on the other hand, leave time alone
					# this means we get the start time
					$ipflow{$fid} += $fcount;
					$ipflow_lasttime{$fid} = $timeval;
					$ipflow_instances{$fid} += 1;
				}
				# else simply store it
				else {
					$ipflow{$words[$i]} = $words[$i+1]; 
					# so this is the start time
					$ipflow_time{$words[$i]} = $timeval;
					$ipflow_lasttime{$words[$i]} = $timeval;
					$ipflow_instances{$words[$i]} = 1;
				}
	#			printf("%d: %s, %s\n", $i, $words[$i],
	#				$words[$i+1]);
				$i = $i + 2;
			}

			#$global_line=~ s/\(/[/g;
			#$global_line=~ s/\)/]/g;
		#}

	}  # end loop
	close(FILE);
}

sub dumpFlows {
	foreach $key (keys %ipflow) {
		printf("key %s: flow %s, time %d\n",          
			$key, $ipflow{$key}, $ipflow_time{$key});
	}
}

sub ipflow_sortit()
{
	@sortedipflows = sort ipflow_by_value keys(%ipflow);
}

sub ipflow_by_value {
        if ($ipflow{$a} < $ipflow{$b}) {
                1;
        }
        elsif ($ipflow{$a} == $ipflow{$b}) {
                0;
        }
        elsif ($ipflow{$a} > $ipflow{$b}) {
                -1;
        }
}

sub ipflow_printit {
	my $flowid;
	my $tstring;
	my $tstring2;
	my $hits;
	my $bits;
	my $lhs_ip, $lhs_port, $rhs_ip, $rhs_port, $proto;
	my $dnsname;
	my $lflow;

	# plaintext file store
	open(OUTPUT_HANDLE, ">$ipascii") || die "can not open: $!\n";
	if ($icmpreport) {
		print OUTPUT_HANDLE "start log time          : instances: flow (ip_src->ip_dst[major/minor code]): total flow bits/sec\n";
	}
	else {
		print OUTPUT_HANDLE "start log time          : instances: flow (ip_src.port->ip_dst.port[proto]): total flow bits/sec\n";
	}
	print OUTPUT_HANDLE "end log time\n";  
	print OUTPUT_HANDLE "----------------------------------------------------------------------------------------\n";
	for ( $i = 0; $i < $asciitopn; $i++) {
		$flowid = $sortedipflows[$i];
		if ($flowid eq "") {
			close(OUTPUT_HANDLE);
			return;
		}
		$tstring = ctime($ipflow_time{$flowid}); 
		$tstring2 = ctime($ipflow_lasttime{$flowid}); 
		$hits = $ipflow_instances{$flowid}; 
		# convert to bits per second
		$bits = int(($ipflow{$flowid} * 8) / 30);
		($lhs_ip, $lhs_port, $rhs_ip, $rhs_port, $proto) = 
			lookup_flowid($flowid);
		$dnsname = gethostip($lhs_ip);
		if ($dnsname) {
			$lhs_ip = $dnsname; 
		}
		$dnsname = gethostip($rhs_ip);
		if ($dnsname) {
			$rhs_ip = $dnsname; 
		}
		if ( $icmpreport ) {
			my $imaj = icmp_major($lhs_port);
			my $imin = icmp_minor($lhs_port, $rhs_port);

			$lflow = $lhs_ip  .  "->" . $rhs_ip . ':' 
				. '[' . $imaj . "\/" .  $imin . ']'; 
		}
		else {
			$lflow = $lhs_ip  . '.' .  $lhs_port . "->" . $rhs_ip . '.' . 
				$rhs_port . ':' . '[' . $proto . ']'; 
		}
		print OUTPUT_HANDLE "$tstring", ": $hits",
			": ", "$lflow", 
			":", "$bits", "\n";
		print OUTPUT_HANDLE "$tstring2\n";
	}
	close(OUTPUT_HANDLE);
}

#
# turn icmp major # into string representation
# See BSD /sys/netinet/ip_icmp.h
#
sub icmp_major {
	my $major = $_[0];

        if ($major == 0) {
                return("reply");
        }
        elsif ($major == 3) {
                return("unreach");
        }
        elsif ($major == 4) {
                return("srcqnch");
        }
        elsif ($major == 5) {
                return("redirect");
        }
        elsif ($major == 8) {
                return("echo");
        }
        elsif ($major == 9) {
                return("rtadvrt");
        }
        elsif ($major == 10) {
                return("rtsolicit");
        }
        elsif ($major == 11) {
                return("timxceed");
        }
        elsif ($major == 12) {
                return("paramprob");
        }
        elsif ($major == 13) {
                return("tstamp");
        }
        elsif ($major == 14) {
                return("tstampreply");
        }
        elsif ($major == 15) {
                return("ireq");
        }
        elsif ($major == 16) {
                return("ireply");
        }
        elsif ($major == 17) {
                return("maskreq");
        }
        return($major);
}

sub icmp_minor {
	my $major = $_[0];
	my $minor = $_[1];

        if ($major == 3) {
		if ($minor == 0) {
			return("net");
		}
		elsif ($minor == 1) {
			return("host");
		}
		elsif ($minor == 2) {
			return("proto");
		}
		elsif ($minor == 3) {
			return("port");
		}
		elsif ($minor == 4) {
			return("needfrag");
		}
		elsif ($minor == 5) {
			return("srcfail");
		}
		elsif ($minor == 6) {
			return("netunknown");
		}
		elsif ($minor == 7) {
			return("hostunknown");
		}
		elsif ($minor == 8) {
			return("isolated");
		}
		elsif ($minor == 9) {
			return("netprohibit");
		}
		elsif ($minor == 13) {
			return("adminprohibit");
		}
        }
        elsif ($major == 5) {
		if ($minor == 0) {
                	return("net");
		}
		elsif ($minor == 1) {
                	return("host");
		}
		elsif ($minor == 2) {
                	return("tosnet");
		}
		elsif ($minor == 3) {
                	return("toshost");
		}
        }
        elsif ($major == 11) {
		if ($minor == 0) {
                	return("intransit");
		}
		elsif ($minor == 1) {
                	return("reass");
		}
        }
        return($minor);
}

sub lookup_flowid {
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
# ACK
#	print "$lhs_ip\n";
#	print "$lhs_port\n";
#	print "$rhs_ip\n";
#	print "$rhs_port\n";
#	print "$proto\n";
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
