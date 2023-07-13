#! /usr/bin/perl

#
# topipa.pl - consolidates ip address activity
#
# For syntax:
#	topipa.pl -n num -s "A,B,C" -d <directory> -f "mon.lite_file0 ... monlite_fileN" > stdout
#
# input file(s):
#	mon.lite  - file which is output of ourmon front-end binary
#	   	note: mon.lite specifies individual filters by name
#
# outputs:	   
#	stdout
#
#
# Modified from omupdate.pl
# <derek@cs.pdx.edu> 10/24/2004
#
# Sunday 31 Oct 04:
# Removed L4_scan, augmented command line parsing with getopt,
# added -s option for weighted scale, added icmperror and udperror 
# columns, modified scoring algorithm.
#

##########################################################

#
# Each element consists of a counter incremented when the key ip 
# address is observed in the corresponding field of a mon.lite
# file.
#
# %hosts hash of hashes
#
# %host = (
#	 ipaddr0 => {
#		 count => 0,
#		 topn_tcp => 0,
#		 topn_udp => 0,
#		 topn_icmp => 0,
#		 syn_list => 0,
#		 ip_scan => 0,
#		 tcp_scan => 0,
#		 udp_scan => 0,
#		 icmperror => 0,
#                udperror => 0,
#		 flows => 0,
#		 metrics => 0,
#		 scans => 0,
#	 },
#	 ipaddr1 => { 
#		 ...
#	 {
#

use vars qw/ %opt /;
use Getopt::Std;

my $opt_string = 'hn:s:d:f:';
my %hosts = ();
my @fields = qw/topn_tcp topn_udp topn_icmp syn_list udperror icmperror ip_scan tcp_scan udp_scanr/ ;
my @flows = qw/topn_tcp topn_udp topn_icmp/ ;
my @metrics = qw/syn_list icmperror udperror/ ;
my @scans = qw/ip_scan tcp_scan udp_scan/ ;
my $scoreFlows = 1;
my $scoreMetrics = 1;
my $scoreScans = 1;
my $numlines = -1;

my $DEBUG = 0;
my $delimiter = "\n";

if ( (scalar @ARGV) < 1 ) { printUsageAndExit(); }

getopts( "$opt_string", \%opt ) or printUsageAndExit();

if ( $opt{'h'} ) {
	printUsageAndExit();
}
if ( $opt{'n'} ) {
	$numlines = $opt{'n'};
}
if ( $opt{'d'} ) {

	my $directory = $opt{'d'};

	#
	# read all files in a directory into the array and
	# prepend directory path to filename
	#
	opendir (DIR, $directory) or die "can't open $directory directory: $!\n";
	@FILES = readdir DIR;
	closedir DIR;
	$size = scalar @FILES;
	while ($size > -1) {
		if ( -f "$directory/$FILES[$size]" ) {
			$FILES[$size] = $directory.'/'.$FILES[$size];
		}
		$size--;
	}
} 
if ( $opt{'f'} ) {
	@FILES = split / /, $opt{'f'};
}
if ( $opt{'s'} ) {
	($scoreFlows, $scoreMetrics, $scoreScans) = split /,/, $opt{'s'};
}

if ($DEBUG) { print "Running...\n"; }

while ( $_ = shift @FILES ) {

	open(FILE, $_) || die "can't open $_ file: $!\n";
	@FILE = <FILE>;
	close(FILE);

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

		# check the first item in each line, against the key words, 
		# to see  if filter is enabled.

		# sysname makes pkts unique per system with per-system label
		# front-end arranges to make sure this label comes first in
		# mon-lite file.

		# topn tcp flows
		if ($items_in_eachline[0] eq "topn_tcp") {
			topn_update("topn_tcp", @items_in_eachline);
		}
		# topn udp flows
		elsif ($items_in_eachline[0] eq "topn_udp") {
			topn_update("topn_udp", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "topn_icmp") {			
			topn_update("topn_icmp", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "syn_list") {
			topn_synlist_update("syn_list", @items_in_eachline);
		}
		elsif ($items_in_eachline[3] eq "worm_log") {
#			topn_synlist_update("worm", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "ip_scan") {
			ip_scan_generic("ip_scan", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "ip_portscan") {
#			ip_scan_generic("L4_scan", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "tcp_portscan") {
			ip_scan_generic("tcp_scan", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "udp_portscan") {
			ip_scan_generic("udp_scan", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "icmperror_list") {
			ip_scan_generic("icmperror", @items_in_eachline);
		}
		elsif ($items_in_eachline[0] eq "udperror_list") {
			ip_scan_generic("udperror", @items_in_eachline);
		}
	}  # end loop
} # end file loop

if ( $DEBUG ) {
	foreach $i ( keys %hosts ) {
		foreach $j ( @fields ) {
			$hosts{$i}{'sum'} += $hosts{$i}{$j};
		}
	}
}



# 
# calculate score:
#
#	 A * flows + B * metrics + C * scans
#
foreach $i ( keys %hosts ) {

	foreach $j ( @flows ) {
		$hosts{$i}{'flows'} += $hosts{$i}{$j};
	}
	foreach $j ( @metrics ) {
		$hosts{$i}{'metrics'} += $hosts{$i}{$j};
	}
	foreach $j ( @scans ) {
		$hosts{$i}{'scans'} += $hosts{$i}{$j};
	}

	$hosts{$i}{'score'} = ($scoreFlows * $hosts{$i}{'flows'}) + ($scoreMetrics * $hosts{$i}{'metrics'}) + ($scoreScans * $hosts{$i}{'scans'});
}


# print header
printf "%-16.16s", "ip address";
printf "%-10.10s", "score";
if ( $DEBUG ) { 
	printf "%-10.10s", "flows"; 
	printf "%-10.10s", "metrics"; 
	printf "%-10.10s", "scans"; 
	printf "%-10.10s", "sum"; 
}
printf "%-10.10s", "count";
foreach $i ( @fields ) { printf "%-10.10s", $i; }
printf "\n\n";

# sort host ip addrs by count in descending order
#foreach $i (sort { $hosts{$b}{'count'} <=> $hosts{$a}{'count'} } keys %hosts) {
foreach $i (sort { $hosts{$b}{'score'} <=> $hosts{$a}{'score'} } keys %hosts) {
	if ( $numlines == 0 ) { exit 0 };
	next if ( $hosts{$i}{'count'} < 2 );
	printf "%-16.16s", $i;

	if ( $hosts{$i}{'score'} eq "" ) { $hosts{$i}{'score'} = 0; }
	if ( $hosts{$i}{'count'} eq "" ) { $hosts{$i}{'count'} = 0; }
	printf "%-10.10s", $hosts{$i}{'score'}; 
	if ( $DEBUG ) { 
		foreach $j ( qw/flows metrics scans/ ) {
			printf "%-10.10s", $hosts{$i}{$j};
		}
		printf "%-10.10s", $hosts{$i}{'sum'}; 
	}
	printf "%-10.10s", $hosts{$i}{'count'}; 

	foreach $j ( @fields ) {
		if ( $hosts{$i}{$j} eq "" ) { $hosts{$i}{$j} = 0; }
		printf "%-10.10s", $hosts{$i}{$j};
	}

	printf "\n";
	$numlines--
}



sub printUsageAndExit()
{
	print "Usage: topipa.pl -n num -s \"A,B,C\" -d directory -f \"mon.lite_file0 ... mon.lite_fileN\"\n";
	print "     -n num        displays num lines\n";
	print "     -s A,B,C      weights for scoring algorithm where\n";
        print "                   A flows, B metrics, C scans\n";
        print "     -d directory  directory of mon.lite files\n";
        print "     -f file1 file2 ... fileN list of files to read\n";
	exit 1;
}



# update for topn_ip, topn_tcp, topn_udp, topn_icmp
#
# mon.lite form: topn_udp : 994 : 207.98.64.97.10->131.252.242.78.30: 4476 :  next-flow-id: byte-count: etc ...
#
# calling args:  topn_update("topn_udp", tuple args ...)
sub topn_update
{
	# labels are topn_ip, topn_tcp, topn_udp or topn_icmp
	my $label = $_[0];
	my $i=2;
	my $count;
	my $src;
	my $dst;
 
	# if no data
	if ( $_[$i]==0 ) {
		return;
	} 

	$i = 3;
	# walk the flow tuples
	while ( ! ($_[$i] eq "") ) {

		# unpack flow-id
		# extract source and destination IP addresses
		($src, $dest) = split /->/, $_[$i];
		# remove port
		$src =~ s/\.\d+$//;
		$hosts{$src}{'count'}++;
		$hosts{$src}{$label}++;

		$i = $i + 2;
	}
}



#
# new format:
#  ip: syn: fin: rst: tcpfrom: tcpto: icmpcount: Nports: port,count,port,count,port,count
#
##############################################################
  
sub topn_synlist_update
{
	# labels are syn_list and worm
	my $label = $_[0];
	my $i=3;	# first data tuple (ip addr)
	my $count;
 
#	if ( $label eq "worm" ) { $i = 6 };
	#  ip: syn: fin: rst: tcpfrom: tcpto: icmpcount: Nports: port,count,port,count,port,count
	#   0	1    2	  3      4       5        6         7           8
	while ( ! ($_[$i] eq "") ) {
		my $host = $_[$i];			  # hostip

		$hosts{$host}{'count'}++;
		$hosts{$host}{$label}++;

#	old format
#		$i = $i + 9;
#	new format
		$i = $i + 10;
	} 
}


# handle port scan filters
#
# callings args:
#	   $label - used in graph labels
#	   $basefile - used to make unique base file name for pics 1 .. N
#	   list of args from mon.lite
sub ip_scan_generic
{
	# labels are ip_scan, ip_portscan, tcp_portscan or udp_portscan
	my $label = $_[0];
	my $i=2;
	my $count;
	my $step = 2;
 
	# if no data
	if ( $_[$i]==0 ) {
		return;
	} 

	# set step corresponding to label
	if ( $label eq "icmperror" ) { $step = 8; }
	if ( $label eq "udperror" ) { $step = 8; }

	# set index to array offset of first IP address
	$i = 3;	
	# walk the flow tuples
	while ( ! ($_[$i] eq "") ) {
		$hosts{$_[$i]}{'count'}++;
		$hosts{$_[$i]}{$label}++;

		$i = $i + $step;
	} 
}

