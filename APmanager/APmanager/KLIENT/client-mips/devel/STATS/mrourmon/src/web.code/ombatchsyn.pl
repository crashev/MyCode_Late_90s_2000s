#! /usr/bin/perl

#
# ombatchsyn.pl
#
# syntax:
# 	ombatchsyn.pl [-W workweight -c count -N -n pattern] fullpath_logfile relative_asciifile
#
#	-W workweight: post-filter entries, any entry on output must be higher than the workweight.
#		note: this is an average then of all entries seen over the N log entries computed.
#		It is highly likely that it will leave mail servers out.  It may leave wormy systems
#		out too.
#	-c count: limit output to N items
#	-N: dns lookups on (off by default).  If on, this can take a long time as reverse
#		lookups are iffy and may timeout eventually.  this is why we have a cache
#		of our own in this code.
#	-n pattern: prefilter input tuples based on pattern applied to ip addresses.

#
# 2nd parameter is not relevant at this time.
#
# purpose: sort the log file to find the largest flows
#	in the log file, and write a report to a 
#	text file.  
#
# input: fullpath_logfile - unix log file output produced
#	by omupdate.pl.  This is the top syn log,
#	named topsyn.log (unless the name was changed).
#
# output: relative_asciifile - report
#
# syntax:
#	# ombatchsyn.pl foo input_syn_logfile output_file
#
# flow database scheme 
# ipflow{ ipaddress } = syn count
# ipflow_time { ipaddress } = time
# ipflow_fins { ipaddress } = fins
# ipflow_rsts { ipaddress } = rsts
# ipflow_tcount{ipaddress} = total packet count
# ipflow_lasttime {ipaddress} = time
# ipflow_instances(ipaddress} = instance count
# ipflow_ports{ipaddress} = small set of concatenated port strings.
# ipflow_port_counts{ipaddress} = count of concatenated port strings.
#
# TBD:
#	2. add ports to the mix as 3rd line
#	3. -i sort by instance count, not syn count
#	4. -w N: pre-filter by work weight = N; i.e., forget about it if work weight
#		is less.  Note this only makes sense if the input file is the wormlist front-end output,
#		and maybe not the synlog.
#
# examples:
#
#	
# ombatchsyn.pl -w 90 -n 131.252 -c 100  input output
#
# print out psu specific syn_list items, sorted by syn count, BUT prefiltered
#	based on both IP address, and work weight.  print no more than 100 of them (we can't
#	stand any more than that).
#
#


#!/usr/bin/perl
# script is "./g"
use Getopt::Std;
%options=();			# null it out
getopts("n:c:NW:",\%options);
$totalcount = 100;
if (defined $options{c}) {
	$totalcount = $options{c};
}
$netpattern = "";
if (defined $options{n}) {
	$netpattern= $options{n};
}
if (defined $options{N}) {
	$dodns = $options{N};
}
$workweight = 0;
if (defined $options{W}) {
	$workweight = $options{W};
}

if ($ARGV[0]) {
	# ip topn log file
	$iplogfile = $ARGV[0];
}
else {
	usage();
}

if ($ARGV[1]) {
	# absolute output filename
	$ipascii = $ARGV[1];
}
else {
	usage();
}

$debug = 0;

# globals for ourmon

# number of ip tuples seen
my $totalips = 0;

# max number of port strings displayed
my $portmax = 5;

use Socket;

# set mon.lite input file - this is where it is found
use Time::Local;
# comment in for ctime, note that it overrides localtime
use Time::localtime;

# month list - look up internal value, based on external string tag 
#
%month = ('Jan', 0, 'Feb', 1, 'Mar', 2, 'Apr', 3, 'May', 4, 'Jun',
             5, 'Jul', 6, 'Aug', 7, 'Sep', 8, 'Oct', 9, 'Nov', 10, 'Dec', 11);


################################################### 
$start_time=`date`;
$start_time=~ s/\n//g;

#  set rrd file names, error log file name, top N png file
#  name.  rrds and error log must have rrdbase as path
######################################################

$unixtime=time;

# parse the file and put info in 
# associative arrays
ipflow_parse();

if ($debug) {
	print "before sort\n";
	dumpFlows();
}

# sort it by syn count
ipflow_sortit();

if ($debug) {
	print "after sort\n";
	dumpFlows();
}

# print it by syn count
ipflow_printit();

exit(0);

# end of main

######################################################################
# functions

# globals
#	$iplogfile - input log file
sub ipflow_parse() { 
	
	my $curline;
	my $time;
	my $timeval;
	my $words;
	my $count;
	my $tuples;
	my $fid;
	my $fins;
	my $resets;
	my $pcount;
	my $pstring;
	my $tuplecount;
	my $logentries = 0;		# count log entries
	my $tupleitems = 10;		# size of a single tuple

	open(FILE, $iplogfile) || die "can't open $iplogfile: $!\n";
	# loop on the next log entry
	while(<FILE>) {

		# multiple blanks to a single blank just in case
		s/  / /g;
		$curline = $_;
		# split on blanks
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

		# save the time stamp, and shift the array
		# past it and the tag
		$timeval = getTime($time[0], $time[1], $time[2]);
		$x = shift(@words);	# time 1 (ignored)
		$x = shift(@words);	# time 2	
		$x = shift(@words);	# time 3

		$x = shift(@words);	# tag 
		$tuplecount = shift(@words);	# count of tuples after tag
		$count = $#words + 1;	# number of words
		$tuples = ($count / $tupleitems);  # number of tuples  
		#print "$count $tuples\n";

		# there may be no syn tuples at all
		if ($tuplecount == 0) {
			next;
		}

		$logentries++;
		for ($i = 0; $i < $count; ) {
			$fid = $words[$i];		# ip address
			$fcount = $words[$i+1];		# syn count
			$synack = $words[$i+2];		# syn+ack count
			$fins = $words[$i+3];		# fin count
			$resets = $words[$i+4];		# reset count
			$tcount = $words[$i+5] + $words[$i+6];	# total count
			# icmpcount is ignored
			$pcount = $words[$i+8];		# count of ports
			$pstring = $words[$i+9];	# port string

			# pre-filters
			#
			# netpattern matches the ip address then fine, else continue
			# TBD. XXX. BUG!@.  fix this so that it somehow uses a netmask
			if ($fid !~ /^$netpattern/) {
#print "$fid NOMATCH\n"; 
				$i = $i + $tupleitems;
				next;
			}
		
			# workweight pre-filter.  ignore the tuple if the workweight is too low.
			#if ($workweight) {
			#	if ($tcount == 0) {
			#		$i = $i + $tupleitems;
			#		next;
			#	}
			#	$ww = (($fcount + $fins + $resets) * 100) / $tcount;
			#	if ($ww < $workweight) {
			#		$i = $i + $tupleitems;
			#		next;
	 		#	}
			#}

			if ($debug) {
				print "i=$i\n";
				print "$fid $fcount $fins $resets $tcount\n";
			}

			# if there is a hit in the cache
			# for the flow id
			if ($ipflow{$fid}) {
				# add the flows together (total)
				# we want to know who the big dogs are
				# on the other hand, leave time alone
				# this means we get the start time
				$ipflow{$fid} += $fcount;
				$ipflow_fins{$fid} += $fins;
				$ipflow_rsts{$fid} += $resets;
				$ipflow_tcount{$fid} += $tcount;
				$ipflow_lasttime{$fid} = $timeval;
				$ipflow_instances{$fid} += 1;
				portString($fid,$pcount, $pstring);
			}
			# else simply store it
			else {
				$ipflow{$fid} = $fcount;
				# so this is the start time
				$ipflow_time{$fid} = $timeval;
				$ipflow_lasttime{$fid} = $timeval;
				$ipflow_instances{$fid} = 1;
				$ipflow_fins{$fid} = $fins;
				$ipflow_rsts{$fid} = $resets;
				$ipflow_tcount{$fid} = $tcount;
				portString($fid,$pcount, $pstring);
				$totalips++;
			}
	#		printf("%d: %s, %s\n", $i, $words[$i],
	#				$words[$i+1]);
			$i = $i + $tupleitems;
		}

	}  # end loop
	close(FILE);

}

sub dumpFlows {
	foreach $key (keys %ipflow) {
		printf("key %s: flow %s, time %d\n",          
			$key, $ipflow{$key}, $ipflow_time{$key});
	}
}

# sort by the value placed in the ipflow associative array
# in this case, #max syns
#
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
	my $syns;
	my $fins;
	my $rsts;
	my $tcount;
	my $lhs_ip, $lhs_port, $rhs_ip, $rhs_port, $proto;
	my $dnsname;
	my $lflow;
	my $tcontrol;
	my $flag;
	my $percent;

	
	# if the totalcount is set to 0, then we want all of them
	if ($totalcount == 0) {
		$totalcount = $totalips;
	}
	# else if the total number of instances in the log file is less than
	# the max. desired lines for output, reset the max lines of output
	#
	elsif ($totalips < $totalcount) {
		$totalcount = $totalips;
	}

	# plaintext file store
	open(OUTPUT_HANDLE, ">$ipascii") || die "can not open: $!\n";
	print OUTPUT_HANDLE "start log time          : instances: DNS/ip : syns/fins/resets/total pkts(%work info) total counts\n";
	print OUTPUT_HANDLE "end log time 	     :\tportsig info (max=$portmax) sampled port signatures format: [dst port, pkt count] [port, count] ...\n";
	print OUTPUT_HANDLE "----------------------------------------------------------------------------------------\n";
	$i = 0;
	while($totalcount--) {
	
		$flag = "";
		$flowid = $sortedipflows[$i];
		# sanity check: less entries than totalcount
		if ($flowid eq "") {
			close(OUTPUT_HANDLE);
			return;	
		}
		$tstring = ctime($ipflow_time{$flowid}); 
		$tstring2 = ctime($ipflow_lasttime{$flowid}); 
		$hits = $ipflow_instances{$flowid}; 
		# get the syn count
		$syns = $ipflow{$flowid};
		$fins = $ipflow_fins{$flowid};
		$rsts = $ipflow_rsts{$flowid};
		$tcontrol = $syns + $fins + $rsts;
		$tcount = $ipflow_tcount{$flowid};
		# print "$syns $fins $rsts $tcontrol $tcount\n";
		if ($tcount == 0) {
			next;
			#print "illegal zero divide\n";
		}
		$percent = int(($tcontrol * 100.0) / ($tcount));
		if ($tcontrol >= (0.90 * $tcount)) {
			$flag = "W";
		}
		elsif ($tcontrol >= (0.50 * $tcount)) {
			$flag = "w";
		}

		# workweight as post-filter, skip output
		if ($workweight) {
			$ww = (($syns + $fins + $rsts) * 100) / $tcount;
#print "$ww $workweight $flowid\n";
			if ($ww < $workweight) {
#print "SKIPPED\n";
				$i++;
				next;
			}
		}
		
		$lhs_ip = $flowid;
		# may replace lhs_ip with DNS name if reverse lookup works
		# and if the flag to do that is set (of course).
		if ($dodns) {
			$dnsname = gethostip($lhs_ip);
			# don't fall for this one
			if ($dnsname eq "localhost") {
				$lhs_ip = $lhs_ip . "(localhost)";
			}
			elsif ($dnsname) {
				$lhs_ip = $dnsname . "($lhs_ip)"; 
			}
		}
		print OUTPUT_HANDLE "$tstring", ": $hits",
			": ", "$lhs_ip", 
			":", "$syns:$fins:$rsts:$tcount($percent%:$flag)", "\n";
		print OUTPUT_HANDLE "$tstring2:\t";
		print OUTPUT_HANDLE "portsigs($ipflow_port_counts{$flowid}/$portmax): $ipflow_ports{$flowid}", "\n";
		$i++;
	}
	close(OUTPUT_HANDLE);
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
 			print "gethostip: cached $ipaddr: $dnscache{$ipaddr}\n";
		}
		return ($dnscache{$ipaddr});
	}

	$SIG{ALRM} = \&sigalrm; 

	if ($debug) {
		print "gethostip: lookup of $ipaddr\n";
	}
  	eval {
 		alarm(5);
		$dnsname = gethostbyaddr(inet_aton($ipaddr), 2);
		alarm(0);
 	};
	if ($debug) {
		print "gethostip: ip: $ipaddr dns: $dnsname\n";
	}
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

# TBD. make the counts a frequency, with a very rough granularity to
# smooth out the similarity.  E.g., 135,100, 445,900 becomes
# 135, 10%, 445, 90% so that there is no difference between
# 135,100 and 135, 99.
#
sub portString 
{
	my $fid = $_[0];
	my $pcount = $_[1];
	my $pstring = $_[2];
	my $tp;

	# if the count is 0, store it
	# if the count is 1, compare and do not store if it is the same
	# if the count is 1..max-1, concat the string at the end
	# if the count is max, don't store it.

	# any count or no count?
	if ($ipflow_ports{$fid}) {
		# if one, compare and do not store string if it is the same
		if ($ipflow_port_counts{$fid} == 1) {
#print "BOOP: MATCH TEST on 1: $fid: $pstring: $ipflow_port_counts{$fid}: $ipflow_ports{$fid}\n";
			$tp = "[" . $pstring . "] ";
			if ($ipflow_ports{$fid} eq $tp) {
#print "BOOP: MATCH on 1: $fid: $pstring: $ipflow_port_counts{$fid}: $ipflow_ports{$fid}\n";
				return;
			}
		}
		# if too many strings simply ignore
		elsif ($ipflow_port_counts{$fid} >= $portmax ) {
#print "BOOP: TOO MANY: $fid: $pstring: $ipflow_port_counts{$fid}: $ipflow_ports{$fid}\n";
			return;
		}
		# else increment and concat the string
		#
		$ipflow_port_counts{$fid} += 1;
		$ipflow_ports{$fid} .= "[" . $pstring . "] ";
#print "BOOP: else add to the pile: $fid: $ipflow_port_counts{$fid}: $pstring: $ipflow_ports{$fid}\n";
	}
	# count is none, so store it
	else {
		$ipflow_port_counts{$fid} = 1;
		$ipflow_ports{$fid} = "[" . $pstring . "] ";
#print "BOOP: initial count, $fid, $ipflow_port_counts{$fid}: $ipflow_ports{$fid}\n";
	}
}

sub sigalrm {
	#alarm(0);
	#print "ALARM TIMEOUT\n";
}

sub usage {
        print "syntax: ombatchsyn.pl [-c count -N] inputfile outputfile\n";
        print "\t-c count if > 0, max desired outputs, if 0, then all log entries \n";
        print "\t-N dns lookup on\n";
        print "\t-n network pattern match filter (prepend pattern-match) on\n";
        print "\t-W workweight post-filter on\n";
        exit(1);
}

