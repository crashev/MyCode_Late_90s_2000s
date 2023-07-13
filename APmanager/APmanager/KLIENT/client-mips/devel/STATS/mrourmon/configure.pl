#!/usr/bin/perl

#
# ourmon install/configure script 
#
# the script will install the front-end
# and/or the back-end as two different parts.
# Either front-end or back-end can be installed
# separately or together.
#
# 5 shellscripts are generated:
#	1. ourmon.sh - driver for running ourmon front-end.
#	2. omupdate.sh - driver for omupdate.pl.  called from crontab.
#	3. batchip.sh - driver for hourly log of log report run.  called from crontab.
#	4. batchipall.sh - driver for daily log of log report run.  called from crontab.
#	5. webget.sh - optional script for using wget to obtain mon.lite from remote probe host.
#
# All parts of ourmon are installed in the ourmon bin.
#
# TODO:
#	figure out way to sanity check build of omupdate.pl in terms of rrdtool.
#	Problem: may not have rrdtool installed in a system location.
#
# question protocol works as follows:
# 1. set default in variable
#$YES_CORRECT="y";	
# 2. ask question, mention default
# print "do you want to install /etc/ourmon.conf? [y] ";
# $line = <STDIN>;
# if line is empty, we use default
# if ($line =~ /^\n/) { 
# }
# else {
# 	chomp($YES_CORRECT = $line);  
# }
# lc reduces to lower-case in case answer was Y
# if (lc($YES_CORRECT) eq "y") {
#	print "$YES_CORRECT is y\n";
#}
# else {
# print "$YES_CORRECT is not y\n";
# }
#
######################################################################

use File::Basename();


# See if there's args. This means its being installed from freebsd ports.
# 
if ($ARGV[0] eq "FREEBSD_PORT_INSTALL") {
	$FREEBSD_PORT_INSTALL=1 ;
	$FINAL_SCRIPT_DIR = $ARGV[1] . "/mrourmon";
	$INSTALL_PREFIX = ".";
}
printf "
***** Building/Installing for FreeBSD via ports system.
***** Will install to $FINAL_SCRIPT_DIR\n" if defined($FREEBSD_PORT_INSTALL);
########################################################
# Variables
########################################################
# INSTALL_PREFIX is used to say "install here". If doing a freebsd port install,
# its going to reference '.' (the work/mrourmon dir). Else, its /home/mrourmon
# FINAL_SCRIPT_DIR is where the final install ends up, and where scripts have
# to reference.
#
$INSTALL_PREFIX = $FINAL_SCRIPT_DIR = "/home/mrourmon" if !defined($FREEBSD_PORT_INSTALL);
$FRONTEND="y";
$BACKEND="y";
$LOGGING="n";		# off by default
$interface="lo0";
$ourmon_script = "ourmon.sh";
$omupdate_script = "omupdate.sh";

# default absolute paths
$webpath = "/usr/local/apache/htdocs/ourmon";
$webpath = "/usr/local/www/data/ourmon" if defined($FREEBSD_PORT_INSTALL);
$crontab_file = "/tmp/crontab.ourmon.xxx";
$wgetpath = "/usr/local/bin/wget";

$rrdbase = $FINAL_SCRIPT_DIR . "/rrddata";
$flowsize = 10000000;
$batchip_file = "batchip.sh";
$batchipall_file = "batchipall.sh";
$topnip = "";
$topnudp = "";
$topntcp = "";
$topnalert = "";
$syslog_conf_file = "scripts/syslog.conf";
$nsyslog_conf_file = "scripts/newsyslog.conf";
$drawtopn_ok = 0;
$ourmon_ok = 0;
$rc;

# everything should not be relative to the build directory
$ourpath = $FINAL_SCRIPT_DIR;
chomp($ourpath);

# make sure the bin/tmp directories exist
system("mkdir bin");
system("mkdir tmp");
$ourbin = $ourpath . "/bin";
$logpath = $ourpath . "/logs";
$tmppath = $ourpath . "/tmp";
$ourmon_conf = $ourpath . "/etc/ourmon.conf";
$monlite = $ourpath . "/tmp/mon.lite";
$tcpworm = $ourpath . "/tmp/tcpworm.txt";
my $olibs="LIBS=";

# figure out systype
$systype = `uname`;
chomp($systype);

print "configuration script to install ourmon.\n";
print "note: default is suggested like so: [default]\n";
print "note: just hit carriage-return for default actions\n";
print "---------------------------------------------------\n";

print "Would you like to install the front-end, ourmon sniffer? [y] ";
$line = <STDIN>;
unless ($line =~ /^\n/) {
  chomp($FRONTEND = $line);
}

# install ourmon front-end
if (lc($FRONTEND) eq "y") {
	print "Front-end configuration phase started ####################\n";

	# find out if libpcap is in /usr/local or /usr/lib
	# go with /usr/local if found
	# tell user about it ...
	my $localpcap = 0;
	if (-e "/usr/local/lib/libpcap.a") {
		$olibs = $olibs . "/usr/local/lib/libpcap.a"; 
		$localpcap = 1;
	}
	else {
		if (-e "/usr/lib/libpcap.a") {
			$olibs = $olibs . "/usr/lib/libpcap.a"; 
			$localpcap = 0;
		}
		else {
			print "did not find libpcap.a in either /usr/local/lib or /usr/lib\n"; 
		}
	}

	print "pcap in general needs to be reinstalled from www.tcpdump.org before ourmon install\n";
	if ($localpcap == 1) {
		print "found pcap lib in /usr/local/lib/libpcap.a - which is a good thing\n";
	}
	else {
		print "did NOT find pcap lib in /usr/local/lib/libpcap.a\n";
		print "if ourmon fails at boot due to claimed syntax errors in ./etc/ourmon.conf\n";
		print "OR if on FreeBSD only ourmon fails to properly size BPF buffer (see INSTALL)\n";
		print "please download and reinstall libpcap and start the configure over again\n";
	} 
	print "hit <CR> to continue: ";
	$line = <STDIN>;

	#1. compile/install ourmon 
	$YES_CORRECT="y";
	print "\nWould you like to compile/install ourmon? [y] ";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  chomp($YES_CORRECT = $line);  
	}
	if (lc($YES_CORRECT) eq "y") {
		chdir("src/ourmon");
		# pass in libpcap whereever found
		if ($systype eq "FreeBSD") {
			print "ourmon build: using make -f Makefile.bsd $olibs\n";
			$rc = system("make -f Makefile.bsd \"$olibs\"");
		}
		if ($systype eq "Linux") {
			print "ourmon build: using make -f Makefile.linux $olibs\n";
			$rc = system("make -f Makefile.linux \"$olibs\"");
		}
		if ($rc == 0) {
			system("cp ourmon ../../bin");
			$ourmon_ok = 1;
		}
		else {
			print "ourmon compile failed\n";
			print "See INSTALL file for more info.\n";
			print "you may need to get a new libpcap library\n"; 
			print "or try making src/ourmon/ourmon by hand.\n";
			print "config terminated.\n";
			exit (1);
		}
		chdir("../..");
		system("chmod +x ./bin/ourmon");
	}
	#2 notify user of intent to use ./etc/ourmon.conf as ourmon.conf file
	print "\nNext we determine the ourmon config/filter file to use.\n";
	print "By default, we use the local $FINAL_SCRIPT_DIR/etc/ourmon.conf to provide input filters to ourmon.\n";
	print "WARNING: you should read/edit/understand ourmon.conf!\n";
	$YES_CORRECT="n";
	print "Do you want to use another ourmon.conf file in some other directory than $FINAL_SCRIPT_DIR/etc? [n] ";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  	chomp($YES_CORRECT = $line);  
	}
	if (lc($YES_CORRECT) eq "y") {
		print "enter the complete filename for the ourmon.conf file. [mumble/ourmon.conf] ";
		$line = <STDIN>;
		chomp($line);
		$ourmon_conf = $line;
		print "ourmon.conf file to be used: <$ourmon_conf>\n";
	}
	#3 create startup script for ourmon
	$YES_CORRECT="y";
	print "\nDo you want to install the ourmon startup script? [y] ";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  	chomp($YES_CORRECT = $line);  
	}
	if (lc($YES_CORRECT) eq "y") {

		print "WARNING: no sane defaults for interface.\n";
		print "WARNING: use #ifconfig -a to determine interfaces.\n";
		print "Please enter the input interface name to sniff from: [$interface] ";
		$line = <STDIN>;
		unless ($line =~ /^\n/) {
		  	chomp($interface = $line);  
		}
		if ($line eq "lo0") {
			print "WARNING: interface lo0 is only for testing\n";
		}
		if ($line eq "lo") {
			print "WARNING: interface lo is only for testing\n";
		}
		print "input interface is $interface\n";

		print "\nPlease enter statistics output file (mon.lite) absolute path: [$monlite] ";
		$line = <STDIN>;
		unless ($line =~ /^\n/) {
		  	chomp($monlite = $line);  
		}
		print "mon.lite output file name is: $monlite\n";
 
		print "\nCreating bin/ourmon.sh driver for startup of ourmon.\n";
		# create ourmon.sh driver to run ourmon with previous parameters
		chdir("bin");
		write_ourmon_shell($interface, $monlite, $ourmon_conf);
		chdir("..");

		print "ourmon.sh placed in $INSTALL_PREFIX/bin for ourmon front-end/probe startup\n";
		print "./ourmon.sh start\n"; 

		# append ourmon.sh to root rc file for boot startup
		$YES_CORRECT="y";
		$path = $INSTALL_PREFIX . "/" . "bin/ourmon.sh";
		if ($systype eq "FreeBSD") {
			print "\ncopy the startup script (bin/ourmon.sh) to /usr/local/etc/rc.d for boot startup? [y] ";
			$line = <STDIN>;
			unless ($line =~ /^\n/) {
		  		chomp($YES_CORRECT = $line);  
			}
			if (lc($YES_CORRECT) eq "y") {
				system("cp $path /usr/local/etc/rc.d");
				system("chmod +x /usr/local/etc/rc.d/ourmon.sh");
			}
		}
		else {
			print "\nPut the startup script (bin/ourmon.sh) in /etc/init.d  etc. for boot startup? [y] ";
			$line = <STDIN>;
			unless ($line =~ /^\n/) {
		  		chomp($YES_CORRECT = $line);  
			}
			if (lc($YES_CORRECT) eq "y") {
				if ($systype eq "Linux") {
					print "LINUX NOTE: for boot: we are assuming /etc/init.d exists \n";
					print "\tand we place ourmon.sh there with rc linkages\n";
					system("cp $path /etc/init.d");
					system("chmod +x /etc/init.d/ourmon.sh");
					make_linux_rc_linkage("/etc/init.d/ourmon.sh");
				}
			}
		}
		print "ourmon front-end install complete\n";
	}
	if ($ourmon_ok) {
		print "ourmon front-end build worked\n";
		print "\nYou may now run $FINAL_SCRIPT_DIR/bin/ourmon.sh to start ourmon\n";
		print "You can use ourmon.sh stop to stop ourmon\n";
		if ( $localpcap != 1 ) {
			print "WARNING: ourmon build used /usr/lib/libpcap.a\n";
			print "if default bpf expressions in ./etc/ourmon.conf fail to load/work\n";
			print "you should get a new libpcap from www.tcpdump.org\n";
		}
	}
	print "\n";
}

print "part 2: install the back-end, omupdate.pl, etc. (web part)? [y] ";
$line = <STDIN>;
unless ($line =~ /^\n/) {
  	chomp($BACKEND = $line);  
}

# install of backend, omupdate.pl, drawtopn, logging
# 
if (lc($BACKEND) eq "y") {
	print "Back-end configuration phase started ######################################\n";

	# assume logging is on
	$LOGGING = "on";

	# DRAWTOPN
	# get drawtopn C program installed
	print "\nNow we compile drawtopn program - dependency libraries must be installed\n";
	print "see INSTALL for details\n";
	compile_drawtopn();
	# drawtopn path is not changeable
	$drawtopn = $ourpath . "/" . "bin/drawtopn"; 

	# MON.LITE
	print "\nIf different, enter front-end mon.lite stats output file absolute path: [$monlite] ";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  	chomp($monlite = $line);  
	}
	print "mon.lite (back-end input) path is $monlite\n\n";

	# WEBDIR
	print "Now we need a local web directory for web output.\n";
	print "hint: the webpath given here is a guess: give the base web directory with /ourmon at the end\n";
	print "enter absolute web path directory: [$webpath] ";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  	chomp($webpath = $line);  
	}
	print "your output web path is: $webpath\n";

	# drop in symlink for webpath in mrourmon path
	
	print "\nDo you want to make the web directory for ourmon? [y] ";
	$YES_CORRECT = "y";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  	chomp($YES_CORRECT = $line);  
	}
	if (lc($YES_CORRECT) eq "y") {
		system("mkdir $webpath");
		print "Done and also creating symlink named \"web.pages\" for webpath in ~mrourmon directory\n\n";
		system("ln -s $webpath web.pages"); 
	}

	# COPY OVER HTML FILES TO WEBDIR
	print "\nNow we copy supplied .html files to the web directory for later editing\n";
	print "WARNING - if doing a reinstall, you probably do not want to do this!\n";
	print "do you want to copy base web files to the web directory? [y] ";
	$YES_CORRECT = "y";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  	chomp($YES_CORRECT = $line);  
	}
	if (lc($YES_CORRECT) eq "y") {
		system("cp src/web.html/* $webpath");
		system("cp INSTALL $webpath/INSTALL.txt");
		system("cp etc/ourmon.conf $webpath");
	}

	# RRDBASE
	# rrdbase is not settable.  local directory

	print "\nINFO only: setting up local rrdbase directory at $rrdbase\n";
	print "\your runtime rrds get stored in this directory, along with the rrd error log file\n";
	print "ignore directory creation warning if directory exists\n\n";
	system("mkdir $rrdbase");;
	print "hit CR to continue: ";
	$line = <STDIN>;
	
	# ALERT SIZE
	# get logging alert size
	if ($LOGGING eq "on") {
		print "\nlogging is on: this includes an alert log file for flows > N megabits\n";
		print "what should N be (in megabits): [$flowsize] ";
		$line = <STDIN>;
		unless ($line =~ /^\n/) {
		  	chomp($flowsize = $line);  
		}
		print "\n";
	}

	# logging is always on now

        #$sysdir = File::Basename::dirname($syslog_ip);
        #$sysbase = File::Basename::basename($syslog_ip);

	# get all of the perl drivers into the bin directory
	# note: this does omupdate.pl again, but no big deal
	system("cp src/web.code/*.pl bin");
	system("chmod +x bin/*.pl");

	# ourbin - bin directory path
	# webpath - current web path 
	chdir ("bin");	
	write_logging_scripts($ourbin, $webpath, $logpath);
	chdir ("..");	

	# create crontab-driven shellscript omupdate.sh to drive omupdate.pl
	# note: we may embed code in here to both move the mon.lite and tworm files
	chdir ("bin");
	write_omupdate_shell($LOGGING, $drawtopn, $monlite, $webpath, $rrdbase, $flowsize, $logpath);
	chdir ("..");

	print "\nmaking needed logging directories in $logpath\n";
	# need to have logs directory
	mklog_dir();

	print "\nMake sure you check out the newly created bin/omupdate.sh.\n";
	# ask if crontab file should be appended to /etc/crontab
	# create crontab variables
	$YES_CORRECT="y";	
	print "\nSave backend crontab commands in /etc/crontab?\n";
	print "or NOT install them (answer n) but \n";
	print "put info in $crontab_file instead for manual inspection and install?\n";
	print "install crontab parts in /etc/crontab?: [y] ";
	$line = <STDIN>;
	if ($line =~ /^\n/) {
	}
	else { 
		chomp($YES_CORRECT=$line);
	}
	create_crontab_file($LOGGING, $YES_CORRECT, $crontab_file);

}

print "\nourmon system config complete\n";
print "see INSTALL for post-config sanity checking\n";

# end of script main
exit (0);

#
# create bin/ourmon.sh script to drive running ourmon
# at boot.
#
# input params:
#	interface:
#	mon.lite pathname:
#	config file
sub write_ourmon_shell
{
	my $interface = $_[0];
	my $outputfile = $_[1];
	my $config = $_[2];
	my $path = $ourpath . "/" . "bin/ourmon";
	my $shell = "/bin/sh";

	if ($systype eq "FreeBSD") {
		write_bsd_shell($interface, $outputfile, $config);
		return;
	}

	write_linux_shell($interface, $outputfile, $config);
	return;
}

# write bsd 5.x style startup script
# which can be used on 4.X systems as well. 
#
sub write_bsd_shell
{
	my $interface = $_[0];
	my $outputfile = $_[1];
	my $config = $_[2];
	my $path = $ourpath . "/" . "bin/ourmon";
	my $shell = "/bin/sh";

  	open(HANDLE, ">$ourmon_script") || die "Cannot open $ourmon_script";

	print HANDLE "#!$shell\n";
 	print HANDLE<<END_OF_TEXT;
# ourmon.sh (auto-generated)
# the sysctl variables exist to reduce dropped packets
# note: on BSD, you may need a recent version of libpcap for this to work
# get it from: www.tcpdump.org
# On BSD: compare sysctl -a outputs to /var/log/messages, bpf bufsizes should match! 
start_om()
{
	sysctl -w debug.bpf_bufsize=8388608
	sysctl -w debug.bpf_maxbufsize=8388608 
END_OF_TEXT
	print HANDLE "\t$path -a 30 -f $config -i $interface -m $outputfile &\n";

 	print HANDLE<<END_OF_TEXT;
	echo -n ' ourmon'
}

stop_om()
{
        kill -9 `cat /var/run/ourmon.pid`
        echo -n ' ourmon'
}

restart_om()
{
        stop_om
        start_om
}

_action="\$1";

if [ -z "\$_action" ]; then
        _action="start"
fi

case \$_action in
start)
        start_om
        ;;
stop)
        stop_om
        ;;
restart)
        restart_om
        ;;
*)      echo "Usage: ourmon.sh {start|stop|restart}" >&2
        exit 64
esac
exit 0
END_OF_TEXT

	if (system "chmod +x $ourmon_script") {
	  	print "Unable to chmod $ourmon_script\n";
	}
}

# write linux startup script
# not very different from bsd startup script
# but convenient to have as separate function for coding.
#
sub write_linux_shell
{
	my $interface = $_[0];
	my $outputfile = $_[1];
	my $config = $_[2];
	my $path = $ourpath . "/" . "bin/ourmon";
	my $shell = "/bin/sh";

  	open(HANDLE, ">$ourmon_script") || die "Cannot open $ourmon_script";

	print HANDLE "#!$shell\n";
 	print HANDLE<<END_OF_TEXT;
# ourmon.sh (auto-generated)
# the sysctl variables exist to reduce dropped packets
# note: on BSD, you may need a recent version of libpcap for this to work
# get it from: www.tcpdump.org
# On BSD: compare sysctl -a outputs to /var/log/messages, bpf bufsizes should match! 
start_om()
{
	sysctl -w net.core.rmem_default=8388608
	sysctl -w net.core.rmem_max=8388608
END_OF_TEXT
	print HANDLE "\t$path -a 30 -f $config -i $interface -m $outputfile &\n";

 	print HANDLE<<END_OF_TEXT;
	echo -n ' ourmon'
}

stop_om()
{
        kill -9 `cat /var/run/ourmon.pid`
        echo -n ' ourmon'
}

restart_om()
{
        stop_om
        start_om
}

_action="\$1";

if [ -z "\$_action" ]; then
        _action="start"
fi

case \$_action in
start)
        start_om
        ;;
stop)
        stop_om
        ;;
restart)
        restart_om
        ;;
*)      echo "Usage: ourmon.sh {start|stop|restart}" >&2
        exit 64
esac
exit 0
END_OF_TEXT

	if (system "chmod +x $ourmon_script") {
	  	print "Unable to chmod $ourmon_script\n";
	}
}

# not really optional but if already done, no
# need to do it again
# check for dependencies here
#
sub compile_drawtopn
{
	my $rc = 0;
	my $libs="LIBS=";
	$YES_CORRECT="y";
	print "compile/install drawtopn? [y] ";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  chomp($YES_CORRECT = $line);  
	}
	if (lc($YES_CORRECT) eq "y") {

		# find libgd
		if (-e "/usr/local/lib/libgd.a") {
			$libs = $libs . "/usr/local/lib/libgd.a "; 
		}
		else {
			if (-e "/usr/lib/libgd.a") {
				$libs = $libs . "/usr/lib/libgd.a "; 
			}
			else {
				print ("did not find libgd.a in /usr/lib or /usr/local/lib -- SEE INSTALL file\n");
			}
		}
		# find libpng
		if (-e "/usr/local/lib/libpng.a") {
			$libs = $libs . "/usr/local/lib/libpng.a "; 
		}
		else {
			if (-e "/usr/lib/libpng.a") {
				$libs = $libs . "/usr/lib/libpng.a "; 
			}
			else {
				print ("did not find libpng.a in /usr/lib or /usr/local/lib -- SEE INSTALL file\n");
			}
		}
		if (-e "/usr/lib/libz.a") {
			$libs = $libs . "/usr/lib/libz.a "; 
		}
		else {
			if (-e "/usr/local/lib/libz.a") {
				$libs = $libs . "/usr/local/lib/libz.a "; 
			}
			else {
				print ("did not find zlib.a in /usr/lib or /usr/local/lib -- SEE INSTALL file\n");
			}
		}

		chdir("src/web.code");
		print "drawtopn build: using make $libs\n";
		$rc = system("make -f Makefile \"$libs\"");
#print $rc, "\n";
		# 0 means ok, else failed
		if ($rc eq 0) {
			$drawtopn_ok = 1;
		}
		else {
			print "compile failed: see INSTALL\n";
			print "config terminated\n";
			exit (1);
		}
		system("cp drawtopn ../../bin");
		system("chmod +x ../../bin");
		chdir("../..");
	}
}

# create omupdate.sh to run omupdate.pl and drive the back-end in general
#
# This script is called out of cron at a one minute basis, and actually
# does its work twice, as ourmon runs every thirty seconds. 
#
# these are the steps:
#	1. if probe is remote use SOME SYSTEM (assume wget here) to get mon.lite tcpworm.txt
#	2. use monbackup.pl to log those two files
#	3. run omupdate.pl to update rrds, make png files, and add to log files
#	4. run tcpworm.pl to create portreport.txt, add to event log, database, and create output tcpworm.txt
#	5. run monbackup.pl again to backup portreport.txt
#	6. run wormtolog.pl to sneakly turn tcpworm.txt into log-like file which
#		we do for the sake of the hourly batch runs.
#

sub write_omupdate_shell
{
	my $logging = $_[0];	# logging on off
	my $drawtopn = $_[1];	# path to drawtopn program
	my $monlite = $_[2];	# path to monlite file
	my $webpath = $_[3];	# path to web directory
	my $rrdbase = $_[4];	# path to rrdbase directory
	my $flowsize = $_[5];	# flowsize variable for big exchange alert
	my $logpath  = $_[6];	# path to log directory
	my $shell = "/bin/sh";
	my $dowget1="n";	# get mon.lite with wget
	my $backuptworm="n";	# backup tworm to logs?
	my $mline;
	my $tline;
	my $dotcpworm = 0;

	# form paths for perl programs
	my $path = $ourpath . "/" . "bin/omupdate.pl";
	my $tcpwormpl = $ourpath . "/" . "bin/tcpworm.pl";
	my $wormtologpl = $ourpath . "/" . "bin/wormtolog.pl";
	my $monbackuppl = $ourpath . "/" . "bin/monbackup.pl";

	# ask user if they have front-end on a different box and want to setup
	# to use wget to copy over mon.lite and tcpworm.txt files
	print "\nIf you are installing ourmon on separate front-end and back-end boxes\n";
	print "(as opposed to front-end and back-end on the same computer)\n";
	print "and plan on using wget to copy mon.lite/tcpworm.txt files from the front-end to back-end,\n";
	print "we can set that up in the omupdate.sh file to happen before omupdate.pl runs\n";
	print "note: default is n\n";
	$YES_CORRECT="n";
	print "note: you must install wget in $wgetpath.\n";
	print "do you want to use wget to copy mon.lite from the front-end? [n] ";
	$line = <STDIN>;
	unless ($line =~ /^\n/) {
	  	chomp($YES_CORRECT = $line);  
	}
	#  handle 1st case of wget of mon.lite file
	if (lc($YES_CORRECT) eq "y") {
		$dowget1 = "y";
		print "enter an IP address for the front-end box. [ip address] ";
		$line = <STDIN>;
		chomp($line);
		$ipaddr = $line;
		print "\nfront-end is at ip: <$ipaddr>\n\n";
	}
  	open(HANDLE, ">$omupdate_script") || die "Cannot open $omupdate_script";
	print HANDLE "#!$shell\n";
	# start for loop
	print HANDLE "for i in 1 2\n";
	print HANDLE "do\n";
	if ($dowget1 eq "y") {
		$mline = "$wgetpath -q --output-document=$monlite http://$ipaddr/mon.lite\n";
		print HANDLE "# use of wget to get remote mon.lite to local ~mrourmon/tmp\n";
		print HANDLE $mline;
		# setup tcpworm.txt the same way
		$tline = "$wgetpath -q --output-document=$tcpworm http://$ipaddr/tcpworm.txt\n";
		print HANDLE "# use of wget to get remote tcpworm.txt to local ~mrourmon/tmp\n";
		print HANDLE $tline;
	}

	# now handle case of backup of tcpworm.txt which we assume we will do because synlist is on by default

	print HANDLE "# backup of tcpworm.txt to logs/tworm directory\n"; 
	print HANDLE "$monbackuppl $tcpworm $logpath/tworm\n";
	
	# we need to also backup mon.lite and put tcpworm file into web dir
	print HANDLE "# backup of mon.lite to logs/mon.lite directory\n"; 
	print HANDLE "$monbackuppl $monlite $logpath/mon.lite\n";
	print HANDLE "cp $tcpworm $webpath/rawtcpworm.txt\n"; 

	my $ompathpl = $ourpath . "/" . "bin/omupdate.pl";
	
	# now take care of omupdate.pl
 	print HANDLE<<END_OF_TEXT;
# run omupdate.pl graph/report maker with appropriate parameters
$ompathpl -m $monlite -t $drawtopn -w $webpath -r $rrdbase -l $logpath -a $flowsize -e $webpath/event_today.txt

# run tcpworm.pl with the appropriate parameters
$tcpwormpl  -E $webpath/event_today.txt -p $webpath/portreport.txt  -D $logpath/topn_today/db $tcpworm > $webpath/tcpworm.txt

# use monbackup.pl to backup the portreport.txt output file
$monbackuppl $webpath/portreport.txt $logpath/portreport

# convert the tcpworm.txt file into a log entry for batch processing later on
$wormtologpl $webpath/rawtcpworm.txt  $logpath  syn_worm.log

sleep 30

# end for
done
END_OF_TEXT
	close HANDLE;
	if (system "chmod +x $omupdate_script") {
	  print "Unable to chmod $omupdate_script\n";
	}
}

# create two shellscript drivers that are used
# to produce hourly reports called via crontab on the hour.
# They produce sorted results of various topn files
# on an hourly basis (today).
# we call this: "top of top", basically meaning
# a longer view than every thirty seconds.
#	
#	1. on the hour	
#	2. at midnight today's script becomes a baked
#	report for the day - there are a week's worth of these
#
# These scripts package up various perlscripts that do
# the actual work including:
#	ombatchip.pl
#	ombatchipsrc.pl
#	ombatchsyn.pl
#
# outputs:
#	batchip.sh - hourly script
#	batchipall.sh - daily script called at midnight

sub write_logging_scripts {
	my $bin = $_[0];
	my $webpath = $_[1];
	my $logpath = $_[2];
	# my $weight = $_[3];		# not yet
	my $script = $batchip_file;
	my $script2 = $batchipall_file; 
	my $syspath = $sysdir . "/" . $sysbase;
	my $shell = "/bin/sh";
	$weight = 80;

	# now create the hourly script - batchip.sh
  	open(HANDLE, ">$script") || die "Cannot open $script";
	print HANDLE "#!$shell\n";
 	print HANDLE<<END_OF_TEXT;
#!/bin/sh
# batchip.sh (auto-generated)
# assume that crontab starts this daily once per hour
OURBIN="$bin"
WEBDIR="$webpath"
LOGPATH="$logpath"
BASELOGFILE="$logpath/topn_today/topn_ip.log"
BASEICMPFILE="$logpath/topn_today/topn_icmp.log"
BASEUDPFILE="$logpath/topn_today/topn_udp.log"
BASESYNFILE="$logpath/topn_today/syn_list.log"

OMBATCHIP=\$OURBIN/ombatchip.pl
OMBATCHIPSRC=\$OURBIN/ombatchipsrc.pl
OMBATCHSYN=\$OURBIN/ombatchsyn.pl

# do top of top report for flows of interest and syn
\$OMBATCHIP other  \$BASELOGFILE \$WEBDIR/topn_today.txt
\$OMBATCHIP icmp \$BASEICMPFILE \$WEBDIR/topicmp_today.txt
\$OMBATCHIP other \$BASEUDPFILE \$WEBDIR/topudp_today.txt

# topn syn reporting
# top 1000 worms, weight is 80 for batch worm report
\$OMBATCHSYN -N \$BASESYNFILE \$WEBDIR/topsyn_today.txt
\$OMBATCHSYN -W $weight -N -c 1000 \$LOGPATH/topn_today/syn_worm.log \$WEBDIR/allworm_today.txt

# do top of top src/dst report
\$OMBATCHIPSRC \$BASELOGFILE \$WEBDIR/topn_ipsrc_today.txt \$WEBDIR/topn_ipdst_today.txt
\$OMBATCHIPSRC \$BASEICMPFILE \$WEBDIR/topicmp_ipsrc_today.txt \$WEBDIR/topicmp_ipdst_today.txt
\$OMBATCHIPSRC \$BASEUDPFILE \$WEBDIR/topudp_ipsrc_today.txt \$WEBDIR/topudp_ipdst_today.txt

END_OF_TEXT
	close HANDLE;
	if (system "chmod +x $script") {
	  	print "Unable to chmod $script\n";
	}

	# now for 2nd script - batchipall.sh
  	open(HANDLE, ">$script2") || die "Cannot open $script2";
	print HANDLE "#!$shell\n";
 	print HANDLE<<END_OF_TEXT;
#!/bin/sh
# batchipall.sh (auto-generated)
# run this script once a day at midnight from cron
# produces sorted results for previous week/rolling log files back.  
# assumptions: previous log files are NOT compressed.
# previous log files exist (no big deal if they don't)
# files munged in TMPDIR

# roll files back one slot (day)
# 6 to 7, 5 to 6 etc.

cd $webpath

for i in 7 6 5 4 3 2 1
do
        A=`expr \$i - 1`
        cp topn.\$A.txt topn.\$i.txt
        cp topn_ipdst.\$A.txt topn_ipdst.\$i.txt
        cp topn_ipsrc.\$A.txt topn_ipsrc.\$i.txt
        cp topicmp.\$A.txt topicmp.\$i.txt
        cp topicmp_ipdst.\$A.txt topicmp_ipdst.\$i.txt
        cp topicmp_ipsrc.\$A.txt topicmp_ipsrc.\$i.txt
        cp topudp.\$A.txt topudp.\$i.txt
        cp topudp_ipdst.\$A.txt topudp_ipdst.\$i.txt
        cp topudp_ipsrc.\$A.txt topudp_ipsrc.\$i.txt
        cp topsyn.\$A.txt topsyn.\$i.txt
        cp allworm.$A.txt allworm.$i.txt
        cp event.$A.txt event.$i.txt
done

# deal with current day rolled over to previous day
cp topn_today.txt topn.0.txt
cp topn_ipdst_today.txt topn_ipdst.0.txt
cp topn_ipsrc_today.txt topn_ipsrc.0.txt

cp topicmp_today.txt topicmp.0.txt
cp topicmp_ipdst_today.txt topicmp_ipdst.0.txt
cp topicmp_ipsrc_today.txt topicmp_ipsrc.0.txt

cp topudp_today.txt topudp.0.txt
cp topudp_ipdst_today.txt topudp_ipdst.0.txt
cp topudp_ipsrc_today.txt topudp_ipsrc.0.txt

cp topsyn_today.txt topsyn.0.txt
cp allworm_today.txt allworm.0.txt
cp event_today.txt event.0.txt
cp /dev/null event_today.txt

END_OF_TEXT
	close HANDLE;
	if (system "chmod +x $script2") {
	  print "Unable to chmod $script2\n";
	}
}

# make all the logging directories
#	topn at top
#	mon.lite/tworm/portreport as their own directories
#
sub mklog_dir()
{
	my $start_time;
	my $dow;

	# get current day of week into dow variable
	$start_time=`date`;
	$start_time=~ s/\n//g;          # zap the newline at the end of the date
	@time = split(/ /, $start_time);
	$dow = $time[0];

	system("mkdir logs");
	chdir ("logs");
	# 3 subdirs for mon.lite/tworm (mon.lite and tcpworm.txt)
	# and for processed portreport.txt output
	system("mkdir mon.lite");
	system("mkdir tworm");
	system("mkdir portreport");

	# topn logging at top no sub-dir
	mkdays();
	system("ln -s $webpath web.pages"); 		# why???
	# sym link for today to topn_today directory
	system("ln -s $dow topn_today");

	chdir("mon.lite");
	mkdays();
	system("ln -s $dow mon_today");
	# symlink 
	chdir("..");

	chdir("tworm");
	mkdays();
	system("ln -s $dow tworm_today");
	chdir ("..");

	chdir("portreport");
	mkdays();
	system("ln -s $dow portreport_today");
	chdir ("..");

	chdir ("..");
}

sub mkdays()
{
	system("mkdir Mon");
	system("mkdir Tue");
	system("mkdir Wed");
	system("mkdir Thu");
	system("mkdir Fri");
	system("mkdir Sat");
	system("mkdir Sun");
}

#
# /tmp/crontab.ourmon.xxx 
#
# 1st put in entry for running backend itself via shellscript driver
# 2nd if logging on, append entries for login drivers.
#
sub create_crontab_file() 
{
	my $logging = $_[0];    # add logging parts
	my $install = $_[1];	# to install in /etc/crontab or no
	my $file    = $_[2];    # name of tmp file 

	my $binpath = $ourpath . "/" . "bin/";
	my $omupdatepath = $binpath . $omupdate_script;
	my $dstring = $binpath . "daily.pl " . $logpath;
  	open(HANDLE, ">$file") || die "Cannot open $file";
	print HANDLE "############## ourmon crontab entries ###############\n";
	$batchip_file = $binpath . $batchip_file;
	$batchipall_file = $binpath . $batchipall_file;
	my $basic = "*/1	*	*	*	*	root	$omupdatepath\n";
	print HANDLE "# run ourmon back-end omupdate.pl etc. per minute\n";
 	print HANDLE "$basic";
	print HANDLE "# batchip.sh - hourly log summary\n";
	print HANDLE "0	*	*	*	*	root	$batchip_file\n";
	print HANDLE "# batchipall.sh - roll over logs at almost midnight\n";
	print HANDLE "59	23	*	*	*	root	$batchipall_file\n";
	print HANDLE "# daily.pl - re init logs at midnight\n";
	print HANDLE "0	0	*	*	*	root	$dstring\n";
	close HANDLE;
	# concat to /etc/crontab if desired
	if ($install eq "y") {
		system("cat $file >> /etc/crontab");
	}
	else {
		print "NOTE: suggested root crontab additions in $file\n";
	}
}

# sigh.
# try to make ourmon happen last
#
sub make_linux_rc_linkage()
{
	my $bootfile = $_[0]; 

	system("ln -s $bootfile /etc/rc0.d/K120ourmon");
	system("ln -s $bootfile /etc/rc1.d/K120ourmon");
	system("ln -s $bootfile /etc/rc2.d/S120ourmon");
	system("ln -s $bootfile /etc/rc3.d/S120ourmon");
	system("ln -s $bootfile /etc/rc4.d/S120ourmon");
	system("ln -s $bootfile /etc/rc5.d/S120ourmon");
	system("ln -s $bootfile /etc/rc6.d/K120ourmon");
}
