#!/usr/bin/perl
# <crashev@ebox.pl>
#
# (crashev) Dodane get_uo_code ,zmiana autoryzacji z local na UO ,local nie 
#	    dziala juz?
# (crashev) Zmiana hosta czata onetu
# (crashev) Dodane validip i pobieranie hostu docelowego z Real UserName
# (crashev) zmieniona funkcja get_uo_code - onet zaczal szyfrowac wszystko
#           na swojej stronie stad,nowa parsacja danych z html i 2 nowe
#           funkcje:
# (crashev) unescape - zamiana znakow pobranych ze strony z %XX na ascii
# (crashev) dodane dekoduj - dekoduje uoKey ze strony do postaci demona
# 
# $TOHOST = "62.21.52.235"; // stary host ,co z nim?
# $TOPORT = 5020;

$TOHOST = "213.180.128.182";
$TOPORT = 5015;
#$TOPORT = 5015;
#$TOHOST = "127.0.0.1";
#$TOPORT = 6667;


my $VERSION=0.5;

use strict;
use IO::Socket ();

use vars qw($debug $PORT $TOHOST $TOPORT $goodpass $listen);
$PORT = shift;
my $arg2 = shift;

my $configfile;

if ($PORT eq '-c') {
	open FH, $arg2 or die "Cannot open config file: $!";
	my %p;
	while (<FH>){
		chomp $_;
		$_=~ s/( |\r|\t)//sg;
			if ($_ !~ /^#/ && $_ ne ''){
			my ($key, $value)=split /=/, $_;
			$p{$key}=$value;
		}		
	}	
	close FH;
	$PORT=$p{ONET_PORT};
	$listen=$p{ONET_INTERFACE};
	$goodpass=$p{ONET_PASS};

} else {
	if ($arg2 && -e $arg2){
		open FH, $arg2;
		$goodpass=<FH>;
		chomp $goodpass;
		close FH;
	} 
}

$| = 1;


$PORT=5016 if (! $PORT);
$listen='0.0.0.0' if (! $listen);

################################################################################################

{

	my %o = ('port' => $PORT,
           'toport' => $TOPORT,
           'tohost' => $TOHOST);


    my $ah = IO::Socket::INET->new('LocalAddr' => $listen,
                                 'LocalPort' => $PORT,
                                 'Reuse' => 1,
                                 'Listen' => 10)
    || die "Failed to bind to local socket: $!";

  $SIG{'CHLD'} = 'IGNORE';
  my $num = 0;
  while (1) {
    my $ch = $ah->accept();
    if (!$ch) {
      print STDERR "Failed to accept: $!\n";
      next;
    }

    ++$num;

    my $pid = fork();
    if (!defined($pid)) {
      print STDERR "Failed to fork: $!\n";
    } elsif ($pid == 0) {

      # This is the child
      $ah->close();
      my $io = new IO::Handle;
      my $tbuffer;

      if ($io->fdopen(fileno($ch),"r")) {
	        } else { 
	    print "Nie moge wykonac fdopen()\n";
	    exit;
	}

	my $petla = 0;
	my $nick;
	
  while ( $ch && $petla!=2 ) {
        $tbuffer = $io->getline;
    	    if (!$tbuffer) {
    	    	  $ch->close();
	          exit(0);
	     }

      if ($tbuffer=~ /^USER/s) {
		$tbuffer =~ s/\r//g;
		$tbuffer =~ s/\n//g;
		$tbuffer =~ /^USER\s[^ ]*\s[^ ]*\s[^ ]*\s:(.*)/;
		my $ret = validip($1);
	        if ($ret) {
	            $TOHOST = $1;
	         }
    		$tbuffer='';
		$petla++;
      }

      if ($tbuffer=~ /NICK/ && ! $nick) {
	$nick=$tbuffer;
	$nick=~ s/NICK ~/NICK /s;
	$nick=~ s/^.*NICK (.*?)(\r|\n| ).*$/$1/s;
	$tbuffer ='';
	$petla++;
      };	

      }
    my %o = ('port' => $PORT,
       'toport' => $TOPORT,
       'tohost' => $TOHOST);

    Run(\%o, $ch, $num, $nick );

    } else {
      $ch->close();
    }
  }
}







sub Run {
  my($o, $ch, $num, $nick ) = @_;
  my $th = IO::Socket::INET->new('PeerAddr' => $o->{'tohost'},
				 'PeerPort' => $o->{'toport'});

  if (!$th) {
    exit 0;
  }
  my $fh;
  my $pass; 
  my $color=1;

  $ch->autoflush();
  $th->autoflush();
  
   my $res = syswrite($th, "AUTHKEY\n", 8);
    
   while ($ch || $th) {


    my $rin = "";

    vec($rin, fileno($ch), 1) = 1 if $ch;
    vec($rin, fileno($th), 1) = 1 if $th;
    my($rout, $eout);
    select($rout = $rin, undef, $eout = $rin, 120);
    if (!$rout  &&  !$eout) {
      print STDERR "Child: Timeout, terminating.\n";
    }
    my $cbuffer = "";
    my $tbuffer = "";


    if ($ch  &&  (vec($eout, fileno($ch), 1)  ||
		  vec($rout, fileno($ch), 1))) {
      my $result = sysread($ch, $tbuffer, 1024);

      if ($tbuffer=~ /^PASS/s && !$pass){
                $pass=$tbuffer;
                $pass=~ s/^.*PASS (.*?)(\r|\n).*$/$1/s;
        }
	
		
#      if ($tbuffer=~ /NICK/ && ! $nick) {
#	$nick=$tbuffer;
#	$nick=~ s/NICK ~/NICK /s;
#	$nick=~ s/^.*NICK (.*?)(\r|\n| ).*$/$1/s;
#	$tbuffer ='';
#	my $res = syswrite($th, "AUTHKEY\n", 8);
#	if ($goodpass && $pass ne $goodpass) {
#		my $msg="INVALID PASSWORD. PLEASE TRY AGAIN.\r\n";
#                my $res = syswrite($ch, $msg, length($msg));
#		exit;
#	}
#      };	

      ### modify VERSION reply...

      $tbuffer=~ s/(NOTICE.*?VERSION)/$1 onetkonekt $VERSION \(http:\/\/onetkonekt.apcoh.org - do not abuse it, please\)/s;
	
      if ($tbuffer=~ /^COLOR/s || $tbuffer=~ /(\r|\n)COLOR/s){
	if ($tbuffer=~ /COLOR ON/s){
		$color=1;  
		my $msg="COLOR IS NOW ON\r\n";
	        my $res = syswrite($ch, $msg, length($msg));

	}
	if ($tbuffer=~ /COLOR OFF/s){
		$color=0;
		 my $msg="COLOR IS NOW OFF\r\n";
                 my $res = syswrite($ch, $msg, length($msg)); 
	};
	$tbuffer=~ s/(\r|\n|)COLOR.*?(\r|\n)//s;
      }

	### kolorki misiu!

	$tbuffer=~ s/\002(.*?)\002/%Fb%$1%Fn%/sg;
	
	$tbuffer=~ s/(\003\d+),\d+/$1/sg;	# wycinam zmiany tla

	$tbuffer=~ s/\0037([^\d])/%Cc86c00%$1/sg;  # pomaranczowy
        $tbuffer=~ s/\00312([^\d])/%C0f2ab1%$1/sg; # niebieski - lub kolor 2 - w drugo stronem
	$tbuffer=~ s/\0032([^\d])/%C0f2ab1%$1/sg; # niebieski - lub kolor 2 - w drugo stronem
        $tbuffer=~ s/\0034([^\d])/%Ce40f0f%$1/sg;  # czerwony
        $tbuffer=~ s/\00310([^\d])/%C1a866e%$1/sg; # navy
        $tbuffer=~ s/\003([^\d])/%C%$1/sg;         # czyszczenie
	$tbuffer=~ s/\\(\w+)/%I$1%/sg;
	
	# wszystkie inne kolorki out
	$tbuffer=~ s/\003\d+/%C%/sg;
	

      if (!defined($result)) {
	print STDERR "Child: Error while reading from client: $!\n";
	exit 0;
      }
      if ($result == 0) {
	exit 0;
      }
    }
    if ($th  &&  (vec($eout, fileno($th), 1)  ||
		  vec($rout, fileno($th), 1))) {
      my $result = sysread($th, $cbuffer, 1024);
      if (!defined($result)) {
	print STDERR "Child: Error while reading from tunnel: $!\n";
	exit 0;
      }
      if ($result == 0) {
	exit 0;
      }

      # sprawdzanie czy mesydz ma numer 006 - autoryzacja onetu	 
	
     if ( get_code($cbuffer) eq '006'){ # autoryzujemy sie w onecie
        my $kod=$cbuffer; $kod=~ s/^.*://;
	$kod=dekode($kod);
	my $uocode = get_uo_code($nick);
#	print "UOCODE: $uocode\n";
	$uocode =~ s/\'//g;
	$uocode = unescape($uocode);
	my $realcode = dekoduj($uocode);
#	print "Wysylamy: AUTHKEY $kod\nONETAUTH UO $realcode 0 2.2.8\n";
	my $msg="AUTHKEY $kod\nONETAUTH UO $realcode 0 2.2.8\n";
	my $res = syswrite($th, $msg, length($msg));
	$cbuffer='';
	$nick='';
    };
    if ( $cbuffer =~ /^:.*.\s525\s\*\s\:Invalid UO key/i )
    {
	syswrite($ch,$cbuffer,length($cbuffer));
	$th->close();
	$ch->close();
	exit(0);
    }
    

    ### tu trzeba pofiltrowac troche smieci...

	# kolorki -  	%Ce40f0f%
	
	if ($color){
		$cbuffer=~ s/%Cc86c00%/\0037/sg;  # pomaranczowy
		$cbuffer=~ s/%C0f2ab1%/\00312/sg; # niebieski - lub kolor 2 - w drugo stronem
		$cbuffer=~ s/%Ce40f0f%/\0034/sg;  # czerwony
		$cbuffer=~ s/%C1a866e%/\00310/sg; # navy
		$cbuffer=~ s/%C%/\003/sg; 	  # czyszczenie
	}

	$cbuffer=~ s/%C\w+?%//sg;

	# support dla kursywy i pogrubienia 
	if ($color) {		
		$cbuffer=~ s/%Fi/%F/sg;
		$cbuffer=~ s/%Fb(.*?)%/\002/sg;
		$cbuffer=~ s/(\002.*?)%Fn%/$1\002/;
	}

	$cbuffer=~ s/%F.*?%//sg;

	$cbuffer=~ s/%I(\w+?)%/\ <$1> /sg;
	

    ##########################################################	
	
   }

    if ($fh  &&  $tbuffer) {
      (print $fh $tbuffer);
    }

    while (my $len = length($tbuffer)) {
      my $res = syswrite($th, $tbuffer, $len);
      if ($res > 0) {
	$tbuffer = substr($tbuffer, $res);
      } else {
	print STDERR "Child: Failed to write to tunnel: $!\n";
      }
    }
    while (my $len = length($cbuffer)) {
      my $res = syswrite($ch, $cbuffer, $len);
      if ($res > 0) {
	$cbuffer = substr($cbuffer, $res);
      } else {
	print STDERR "Child: Failed to write to tunnel: $!\n";
      }
    }
  }
}


sub dekode {
    my ($s)=@_;
    my $s1;

    my (@f1)=(   14, 1, 48, 40, 34, 30, 52, 59, 2, 59,
		 6, 60, 22, 13, 9, 33, 19, 14, 58, 27,
		 22, 15, 36, 46, 8, 11, 44, 59, 30, 0,
		 23, 45, 2, 10, 23, 37, 41, 13, 34, 43,
		 11, 40, 42, 34, 53, 52, 6, 11, 4, 2,
		 38, 26, 18, 12, 10, 27, 24, 55, 25, 54,
		 55, 49, 38, 58, 59, 0, 33, 39, 14, 5,
		 21, 25, 45, 1, 60, 37, 53, 4
		 );
    
    my (@f2) = ( 25, 43, 43, 47, 41, 36, 32, 9, 44, 20,
		 49, 7, 55, 22, 10, 20, 15, 53, 5, 59,
		 30, 37, 20, 27, 11, 14, 41, 3, 17, 1,
		 23, 42, 45, 4, 28, 24, 41, 60, 34, 23,
		 19, 21, 31, 12, 43, 41, 32, 59, 32, 37,
		 56, 0, 13, 15, 28, 25, 29, 8, 28, 47,
		 9, 51, 27, 55, 55, 55, 18, 34, 54, 52,
		 58, 11, 12, 28, 24, 56, 7, 57
		 );
    my (@f3) = ( 27, 48, 27, 16, 3, 29, 56, 44, 5, 37,
		 45, 60, 59, 47, 58, 38, 29, 27, 46, 5,
		 51, 54, 7, 6, 49, 11, 54, 45, 13, 38,
		 60, 40, 24, 26, 57, 28, 55, 52, 11, 60,
		 28, 57, 58, 25, 42, 55, 2, 10, 21, 48,
		 16, 10, 40, 23, 16, 28, 35, 9, 11, 48,
		 47, 9, 27, 10, 35, 22, 38, 29, 13, 49,
		 28, 41, 45, 25, 4, 25, 19, 6
		 );
    my (@p1) = ( 1, 9, 12, 7, 2, 6, 0, 5, 3, 11,
		 4, 14, 10, 8, 13, 15
		 );
    my (@p2) = ( 12, 5, 1, 4, 15, 0, 2, 11, 10, 14,
		 7, 6, 8, 3, 9, 13
		 );

    my (@ai); my (@ai1);
    
    if (length($s) < 16){
	return "(key to short)";
    }
    

    for(my $i = 0; $i < 16; $i++)
    {
	my $c = ord(substr($s, $i, 1));
	$ai[$i] = ($c> ord('9') ? $c> ord('Z') ? ($c - 97) + 36 : ($c - 65) + 10 : $c - 48);
    }
    
    for(my $j = 0; $j < 16; $j++){
	$ai[$j] = $f1[ $ai[$j] + $j];
    }

    (@ai1)=(@ai);

    for(my $k = 0; $k < 16; $k++) {
	$ai[$k] = ($ai[$k] + $ai1[$p1[$k]]) % 62;
    }
    
    for(my $l = 0; $l < 16; $l++) {
	$ai[$l] = $f2[$ai[$l] + $l];
    }
 
    (@ai1)=(@ai);

    for(my $i1 = 0; $i1 < 16; $i1++)
    {
	$ai[$i1] = ($ai[$i1] + $ai1[$p2[$i1]]) % 62;
    }

    for(my $j1 = 0; $j1 < 16; $j1++)
    {
	$ai[$j1] = $f3[$ai[$j1] + $j1];
    }

    for(my $k1 = 0; $k1 < 16; $k1++)
    {
	my $l1 = $ai[$k1];
	$ai[$k1] = $l1 >= 10 ? $l1>= 36 ? (97 + $l1) - 36 : (65 + $l1) - 10 : 48 + $l1;
	$s1.=chr($ai[$k1]);
    }
    
    return $s1;
}


sub get_code {
    my ($line)=(@_);
    my ($none, $code, $rest)=split(" ", $line);
        return $code;
}


sub unescape {
    my ($wyrazenie) = @_;
    $wyrazenie =~ s/%([0-9A-Fa-f]{2})/pack('c',hex($1))/ge;
    return $wyrazenie;
}


sub get_uo_code {
  my ($ksywka)=@_;
  my $onethost="213.180.130.201";
  my $onetport=80;
  my $val;
  my $th = IO::Socket::INET->new('PeerAddr' => $onethost,
				 'PeerPort' => $onetport,
				 'Proto'=>'tcp'
				 );
  if (!$th) {
    exit 0;
  }
    my $msg = "GET /applet.html?N=$ksywka&CH=16_17_18_lat& HTTP/1.0\r\nHost: czat.onet.pl\r\n\r\n";
    syswrite($th, $msg, length($msg));
    $th->autoflush();

# Porownania
# $lancuh =~ m/wyrazenie/; - poszukuje wyrazenie w lancuch i wtedy true
# $lanuch =~ /wyrazenie/i; - nie zwraca uwagi na wielkosc liter,m pominiete
# $lanuch =~ /^wszystko.*/ - wszystko co zaczyna sie od wszystko 
# /[ a-h]/ - rownoznaczne z / abcdefgh/
# /[ a-d1-4] - / abcd1234/
# /^(O|A)la/; - pasuje jesli zaczyna sie od Ola lub Ala
# /^(\S+) - wystapenienie jednego lub wiecej znakow

while ( $th) {
    my $tbuffer = "";
    my $result = sysread($th, $tbuffer, 8048);
    if ($result>0) {
    # <param name="uoKey" value="01583191ded7fef81b6e9481e6d5e98e">
    chomp $tbuffer;
    if ($tbuffer =~ /uoKey/i) {
	   $tbuffer =~ /unescape\(\'%.*.param\sname=%22uoKey%22\svalue=\'\)\s\+\sdk42\(\s*unescape\s*\(\s*(.*?)\s*\)/gi; 
		    $val = $1;
		    return $val;
	    }
    }
    if  (!$result) {
		 $th->close();
		 $th = 0;
	 }
    }
    return 0;
}



sub validip {
    my ($check) = @_;
    if (!$check) { return 0; }
    $check =~ /^([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3})$/;
    if ( (!defined($1))||(!defined($2))||(!defined($3))||(!defined($4)) ) {
	    return 0;
	}
    if ( ($1>255)||($2>255)||($3>255)||($4>255) ) { return 0;}
    return 1;
}


sub dekoduj {
    my ($abc) = @_;
    my @whole = ();
    my @def   = ();
    my @l1l   = ();
    
    for (my $bcd = 0; $bcd < 32; $bcd++) {
	$def[$bcd] = unpack('c',substr($abc,$bcd,1));
    }
           
$l1l[0]=$def[1]^4;
$l1l[1]=$def[5]^14;
$l1l[2]=$def[24]^14;
$l1l[3]=$def[12]^13;
$l1l[4]=$def[2]^10;
$l1l[5]=$def[25]^14;
$l1l[6]=$def[16]^11;
$l1l[7]=$def[29]^1;
$l1l[8]=$def[13]^8;
$l1l[9]=$def[3]^13;
$l1l[10]=$def[14]^10;
$l1l[11]=$def[19]^5;
$l1l[12]=$def[17]^6;
$l1l[13]=$def[28]^7;
$l1l[14]=$def[23]^9;
$l1l[15]=$def[6]^8;
$l1l[16]=$def[18]^9;
$l1l[17]=$def[7]^12;
$l1l[18]=$def[4]^5;
$l1l[19]=$def[21]^8;
$l1l[20]=$def[9]^13;
$l1l[21]=$def[30]^2;
$l1l[22]=$def[8]^4;
$l1l[23]=$def[15]^0;
$l1l[24]=$def[22]^10;
$l1l[25]=$def[27]^4;
$l1l[26]=$def[20]^5;
$l1l[27]=$def[11]^12;
$l1l[28]=$def[26]^5;
$l1l[29]=$def[0]^2;
$l1l[30]=$def[31]^2;
$l1l[31]=$def[10]^10;

    my $ret;
    for (my $bcd = 0 ;$bcd < 32; $bcd++) {
#	    if ($l1l[$bcd]>58) {
#		$l1l[$bcd] = (97)+$def[$bcd]%6;
#	    } else {
#	    $l1l[$bcd] = (48)+$def[$bcd]%10;
#	    }	
	    $ret.=pack('c',$l1l[$bcd]);
    }    
    return $ret;
}
