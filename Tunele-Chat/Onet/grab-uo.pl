#!/usr/bin/perl
#http://czat.onet.pl/applet.html?N=pawq123&CH=16_17_18_lat&
my $VERSION=0.5;
use strict;
use IO::Socket ();

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
    my $msg = "GET /applet.html?N=$ksywka&CH=Kalambury7& HTTP/1.0\r\nHost: czat.onet.pl\r\n\r\n";
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
#+ unescape('%3cparam name=%22uoKey%22 value=') + dk42( unescape(
#'50%3E4%60%3E%3B%3B7k%3E88%3F80i%60%3C549%3C9h9da61%3A5' ) )
#
#
    print $tbuffer;
    if ($tbuffer =~ /uoKey/i) {
	   $tbuffer =~ /unescape\(\'%.*.param\sname=%22uoKey%22\svalue=\'\)\s*\+\s*dk42\(\s*unescape\s*\(\s*(.*?)\s*\)/gi; 

#printf "i tu mamy: $1 \n";

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
	    if ($l1l[$bcd]>58) {
		$l1l[$bcd] = (97)+$def[$bcd]%6;
	    } else {
	    $l1l[$bcd] = (48)+$def[$bcd]%10;
	    }	
	    $ret.=pack('c',$l1l[$bcd]);
    }    
    return $ret;
}


my $dupa = get_uo_code('pawq123');
$dupa =~ s/\'//g;
#print "[ESCAPED]: $dupa\n";
$dupa = unescape($dupa);
print "Dupa: ";print $dupa;print "\n";
my $cos = dekoduj($dupa);
print "UO_CODE: $cos\n";
#my $coss = 'gl24%3F5m8%3Dc3m%60%3C8h4%3D%3B8%3Cla185%3F5%60%3D%3Dc';
my $coss = 'f2%3C%3Aa78%3A3%3Dhj902026%3Fg49%3D16%3B72f04c';
$coss = unescape($coss);
my $cos2 = dekoduj($coss);
print "UO_CODE: $cos2\n";