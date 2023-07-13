#! /usr/bin/perl

#
# daily.pl
#
# syntax:
#	# daily.pl base_log_dir
# e.g.,
#	# daily.pl /home/mrourmon/logs
#
# 3 tasks:
#
# Assumption here is that there are 4 logging directories:
#
# mon.lite logging
# twormlog logging 
# broken out topn logging
# portreport.txt for output portreport.txt files (processed tcpworm.txt)
#
# All are found under a base logs directory with the following format:
#
# /home/mrourmon/logs
#	mon.lite
#	tworm
#	topn
#	portreport
#
#	each directory above has Sun Mon Tue ... Sat daily directories
#
# mon.lite files are logged in mon.lite, tworm.txt files in tworm.
# broken-out topn info in topn on a per-filter basis.  portreport.txt into portreport.
#
# This perl script is called once at midnight to prepare for the next
# day's logging.
#
# it's job is to zero out the daily (Sun Mon Tue Wed Thu Fri Sat)
# directory in the base logging directory for all 3 of the above.
#
# for the topn logging directory it also creates a symlink
# to a cur for the current day.
#
# Assumption: output_dir has subdirs for days of week.  Mon, Tue, etc.
# Thus output format is:  log/Mon/date.mon.lite
# form is thus:
# Sun Mon Tue Wed Thu Fri Sat 
#
# NOTE: there is NO KNOWLEDGE OF INDIVIDUAL TYPES OF LOG FILES HERE

$debug = 0;
$argc=$#ARGV+1;

if ($argc != 1) {
	print "syntax: # daily.pl output_dir\n";
	print "example: # daily.pl /home/mrourmon/logs\n";
	exit(1);
}
use File::Basename();

use Time::Local;
# comment in for ctime, note that it overrides localtime
use Time::localtime;

my $outputdir = $ARGV[0];

################################################### 
$start_time=`date`;
$start_time=~ s/\n//g;		# zap the newline at the end of the date
@time = split(/ /, $start_time);
$dow = $time[0];

chdir($outputdir);

# mon.lite daily prep
system("rm -fr mon.lite/$dow"); 
system("rm -fr mon.lite/mon_today"); 
system("mkdir mon.lite/$dow"); 
system("ln -s $dow mon.lite/mon_today");

# tworm daily prep
system("rm -fr tworm/$dow"); 
system("rm -fr tworm/tworm_today"); 
system("mkdir tworm/$dow"); 
system("ln -s $dow tworm/tworm_today");

# topn daily prep
system("rm -fr $dow"); 
system("rm -f topn_today"); 
system("mkdir $dow"); 
system("ln -s $dow topn_today");

# portreport daily prep
system("rm -fr portreport/$dow"); 
system("rm -f portreport/portreport_today"); 
system("mkdir portreport/$dow"); 
system("ln -s $dow portreport/portreport_today");

exit(0);

# end of main

