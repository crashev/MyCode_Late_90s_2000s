#! /usr/bin/perl

#
# monbackup.pl
#
# This file exists as a script for backup of the mon.lite file or other back-end
# associated files every 30 seconds.  We assume a pre-existing log directory
# with a directory for each day of the week (as with UNIX date(8), Mon, etc.).
#
# It does not use syslog.  It may be used on other files.
#
# input params:
#
# monbackup.pl input_file output_dir 
#
# We take the input file, and prepend the date turning blanks
# into underlines.
#
# E.g.,
# mon.lite -> date_date_date.mon.lite 
#
# Assumption: output_dir has subdirs for days of week.  Mon, Tue, etc.
# Thus output format is:  log/Mon/date.mon.lite
# For example,
# Sun Mon Tue Wed Thu Fri Sat 
#

$debug = 0;
$argc=$#ARGV+1;

if ($argc != 2) {
	print "syntax: # monbackup.pl input_file output_dir\n";
	print "example: # monbackup.pl mon.lite /home/mrourmon/monlog\n";
	exit(1);
}
use Socket;
use File::Basename();

use Time::Local;
# comment in for ctime, note that it overrides localtime
use Time::localtime;

my $inputfile = $ARGV[0];
my $target = File::Basename::basename($inputfile);
my $outputdir = $ARGV[1];

################################################### 
$start_time=`date`;
$start_time=~ s/\n//g;		# zap the newline at the end of the date
@time = split(/ /, $start_time);
$dow = $time[0];
$start_time =~ s/ /_/g;
$opath=$outputdir . '/' . $dow .'/';
# output name is logdir/day of week/time_time_time.targetname
$oname=$opath . $start_time . '.' . $target;
system("cp $inputfile $oname");

exit(0);

# end of main

