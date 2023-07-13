#! /usr/bin/perl

#
# wormtolog.pl
#
# convert a raw tcpworm.txt front-end output into a log-file entry and
# put it in a logfile
#
# syntax:
#
#	wormtolog.pl tcpworm.txt output_log_dir output_log_basename
#

use Socket;
use Getopt::Std;

%options=();
getopts("",\%options);
# like the shell getopt, "d:" means d takes an argument
#print "Unprocessed by Getopt::Std:\n" if $ARGV[0];
#foreach (@ARGV) {
#  print "$_\n";
#}
#if (defined $options{d}) {
#	$debug = 1;
#}
#if (defined $options{N}) {
#	$dodns = 1;
#}

if ($ARGV[0]) {
	$inputfile = $ARGV[0];
}
else {
	usage();
}

if ($ARGV[1]) {
	$logbase = $ARGV[1];
}
else {
	usage();
}

if ($ARGV[2]) {
	$logfile = $ARGV[2];
}
else {
	usage();
}

################################################### 
# get the UNIX time in seconds
$start_time=`date`;
$start_time=~ s/\n//g;

# in and out in one routine
doit();

exit(0);

# end of main

######################################################################
# functions

# parse input file
sub doit() { 
	
	my $curline = "";
	my @words;
	my $sysname;
	my $count = 0;

	open(FILE, $inputfile) || die "can't open $inputfile: $!\n";

	while(<FILE>) {

		# no new line
		s/\n//g;
		$curline = $curline . $_;
		$count++;

	}  # end loop
	close(FILE);

	$curline = "worm_log: $count :" . $curline;	# simulate the tag/count form
	omsyslog($logbase, $logfile, $curline);

#	print "$curline\n";
}

#
# log the output
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

sub usage {
	print "syntax: wormtolog.pl input_file output_file\n";
	exit(1);
}
