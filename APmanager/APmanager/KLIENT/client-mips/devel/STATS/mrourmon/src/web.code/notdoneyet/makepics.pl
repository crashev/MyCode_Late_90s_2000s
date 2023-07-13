#! /usr/bin/perl

#
# makepics.pl
#
# input params:
#
# makepics.pl report_type start_time instance_count input_logfile output_dir
#
# report_type: icmp, tcp, udp, ip, syn
#
# at this point: syn is different from the other 4.
#
# Experimental utility.
# purpose: take the topn log file in question
#	and based on the start time and the number of sample instances
#	create topn pictures (and an html file to go with them)
#	in the output directory.
#
# This may be out of date.
#
# e.g.,
# makepics.pl icmp 11:00 30 ../logs/topicmp.log ../topnpics.icmp
#
# The imcp log is taken and 30 pictures (covering fifteen minutes)
# starting at 11:00 am are created in the topnpics.icmp directory.
#
# The filename for output pics is
#
# icmp.instance#.png
#
# That is the report_type, and the instance number are shown.
# The picture in question has the time shown as well in the picture.
#

# TBD. put this in an include file
$topnprog = "/home/mrourmon/bin/drawtopn";

$debug = 0;

# globals for ourmon

use Socket;

# set mon.lite input file - this is where it is found
use Time::Local;
# comment in for ctime, note that it overrides localtime
use Time::localtime;

# month list - look up internal value, based on external string tag 
#
%month = ('Jan', 0, 'Feb', 1, 'Mar', 2, 'Apr', 3, 'May', 4, 'Jun',
             5, 'Jul', 6, 'Aug', 7, 'Sep', 8, 'Oct', 9, 'Nov', 10, 'Dec', 11);

# makepics.pl report_type start_time instance_count input_logfile output_dir
my $reporttype = $ARGV[0];
my $starttime = $ARGV[1];
my $instances = $ARGV[2];
my $iplogfile = $ARGV[3];
my $outdir = $ARGV[4];

$argc=$#ARGV+1;
#print $argc,"\n";

if ($argc==1 && $ARGV[0]=~ /\-h/) { 
	usage();
  	exit 0;
}

if ($argc != 5) { 
	usage();
  	exit 1;
}

if ($instances <= 0) {
	print "instances must be a positive value\n";
	exit 1;
}

################################################### 
$start_time=`date`;
$start_time=~ s/\n//g;

$unixtime=time;

makepics();

writeindex_file($outdir, $instances, $starttime);

exit(0);

# end of main

######################################################################
# functions

sub makepics() 
{ 
	my $curline;
	my $time;
	my $timeval;
	my $words;
	my $count;
	my $countpairs;
	my $fid;
	my $icount = $instances;
	my $j = 0;
	my $itrigger = 0;
	my $stime = $starttime;
	# only care about hours:minutes
	my @targettime = split(/:/, $stime);	
	my $flowstring;

	open(FILE, $iplogfile) || die "can't open $iplogfile: $!\n";
	while(<FILE>) {

#		print "top of loop, $j $icount\n";
		# stop when we get to the correct number of desired
		# instances
		if ($itrigger) {
			if ($j == $icount) {
#				print "done  $j $icount\n";
				close(FILE);
				return(0);
			}
		}
		s/  / /g;
		$curline = $_;
		@time = split(/ /, $curline);
		#print "$time[0]\n";	# month
		#print "$time[1]\n";	# day
		#print "$time[2]\n";	# hour:min:sec (2 digits)
		# trigger on the time to count instances
		#
		@curtime = split(/:/, $time[2]);	
		#print "$curtime[0]:$curtime[1]:$targettime[0]:$targettime[1]\n";
		if ($itrigger == 0) {
			if ($curtime[0] == $targettime[0]) {
				if ($curtime[1] == $targettime[1]) {
#		 			#print "TRIGGER: $j\n";
					$itrigger = 1;	
					$j = 0;
					system("mkdir $outdir");
				}
			}
		}

		if ($itrigger) {
			$curline =~ s/ //g;    
			$curline =~ s/\n//g;
			# convert greater than to zilch, until we figure out
			# how to escape it 
			$curline =~ s/\>//g;

			@words = split(/:/, $curline);

			# save the time stamp, and shift the array
			# past it and the tag
			#$timeval = getTime($time[0], $time[1], $time[2]);
			$x = shift(@words);	# time 1 (ignored)
			$x = shift(@words);	# time 2	
			$x = shift(@words);	# time 3
			$x = shift(@words);	# tag 
			$x = shift(@words);	# count of flows
			$count = $#words;	# number of words
			$countpairs = $count + 1;  
			if ($reporttype eq "syn") {
				$pairs = $countpairs/4;
			}
			else {
				$pairs = $countpairs/2;
			}
			#print "count $count, words $#words, countpairs $countpairs, pairs $pairs\n";

			$flowstring = "-n $pairs -d ";
			for ($i = 0; $i < $countpairs; ) {
				$fid = $words[$i];
				$fcount = $words[$i+1];
				if ($reporttype eq "syn") {
					$fins = $words[$i+2];
					$resets = $words[$i+3];
				}
				
				# syn format is ip: syns, fins, resets 
				if ($reporttype eq "syn") {
					$flowstring = $flowstring . $fid . ":" . $fins . ":" .
						$resets . ' ' . $fcount . ' ';
				}
				else {
					@afid = split(/-/, $fid);	
					$flowstring = $flowstring . '"' . $afid[0] . "->" . $afid[1] .  '"' . ' ' . $fcount . ' ';
				}
				if ($reporttype eq "syn") {
					$i = $i + 4;
				}
				else {
					$i = $i + 2;
				}
			}
	 		#print "$flowstring\n";
			# construct the rest of the drawtopn parameters
			$tflag = "-t \"top N[$j] $reporttype [$time[0]:$time[1]:$time[2]]\"";
			if ($reporttype eq "syn") {
				$vflag = "-v \"syns/period\""; 
			}
			else {
				$vflag = "-v \"bits/sec\""; 
			}
			$fname = "$reporttype.$j.png";
			$fflag = "-f $outdir/$fname";
			$lflag = "-l 2";
			makeone_pic($outdir, $fname, $j, $icount, $tflag, $vflag, $fflag, $lflag, $flowstring);
			$j++;
		}
	}  # end loop
	close(FILE);
}

#
# create one png file, and an instance html file
# to allow it to be walked
#
sub makeone_pic 
{
	my $outdir = $_[0];
	my $fname = $_[1];
	my $cur = $_[2];
	my $total = $_[3];
	my $tflag = $_[4];
	my $vflag = $_[5];
	my $fflag = $_[6];
	my $lflag = $_[7];
	my $flowstring = $_[8];

	print "making pic $cur\n";
	#print "drawtopn $outdir $fname: $tflag $vflag $fflag $lflag $flowstring\n";
	system("$topnprog $tflag $vflag $fflag $lflag $flowstring");

	# write per pic html file 
	writehtml_file($outdir, $fname, $cur, $total);
} 

sub writehtml_file
{
	my $outdir = $_[0];
	my $fname = $_[1];
	my $cur = $_[2] + 1;		# current index
	my $total = $_[3];		# total count
	my $path = $outdir . "\/" . "P$cur.html";	# name of html file
	my $first = 1;
	my $last = $total;
	my $halfway = int($total / 2);
	my $next;

	if ($halfway < 0) {
		$halfway = 1;
	}
	
	if ($cur == 1) {
		$prev = $cur;
	}
	else {
		$prev = $cur - 1;
	}
	if ($cur == $total) {
		$next = $total;
	}
	else {
		$next = $cur + 1;
	}
        open(HANDLE, ">$path") || die "Cannot open $path";
        print HANDLE<<END_OF_TEXT;
<TITLE> Page P$cur.html </TITLE>
<H2> $reporttype: Slide $cur of $total. </H2>
<BR>
Click slide for next, or goto
<A href="P$prev.html"> previous</A>,
<A href="P$first.html"> first</A>,
<A href="P$halfway.html"> halfway</A>,
<A href="P$last.html"> last</A> slides or
<A href="index.html"> back </A> to thumbnail layout.
<BR>
<A href="P$next.html"> <IMG SRC="$fname"> </A>
<BR>
Click slide for next, or goto
<A href="P$prev.html"> previous</A>, or
<A href="index.html"> back </A> to index html file.
<BR>
END_OF_TEXT
        close HANDLE;
}

sub writeindex_file
{ 
	my $outdir = $_[0];
	my $total = $_[1];
	my $path = $outdir . "\/" . "index.html";	
	my $first = 1;
	my $last = $total;
	my $i;
        open(HANDLE, ">$path") || die "Cannot open $path";
        print HANDLE<<END_OF_TEXT1;
<HTML>
<HEAD>
<TITLE> $reporttype </TITLE>
</HEAD>
<BODY>
<H1> report type: $reporttype log </H1>
<STRONG> ourmon syslog to pics web presentation </STRONG>
<br>
<STRONG> start time[$starttime]: $total instances </STRONG>

<P>
END_OF_TEXT1
	
	for ($i = 1; $i <= $total; $i++) {
		my $s = "instance[$i]: <A href=" . '"' . "P$i.html" . '"' . "> $i.png </A><br>";
      		print HANDLE "$s\n";
	}
        print HANDLE<<END_OF_TEXT2;
<P>
By clicking the above thumbnail page images, you can peruse the slides.

</BODY>
</HTML>
END_OF_TEXT2
        close HANDLE;
}

sub usage {
	print "makepics.pl type starttime instances logfile outputdir\n";
	print "type is [ip, icmp, udp, tcp, syn]\n";
	print "starttime is in format 01:00 (1 am)\n";
	print "logfile is ourmon logfile (full or relative path)\n";
	print "outputdir is directory to place pics and web wrappers in\n";
}

