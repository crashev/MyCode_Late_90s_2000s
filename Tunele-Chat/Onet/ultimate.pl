#!/usr/bin/perl
#
# Kombajn Czatowy ver 1.0 - Pawel Furtak
#  - laczy sie ze strona czatu i wyciaga z niej funkcje do dekodowania uoKey
#  1) Wyciaga eskapowany/zakodowany kod html i uoKEY
#  2) Rozkodowuje html,wyciaga z niego funkcje dekodujaca
#  3) Rozkodowuje tabele , wyciaga i przakazuje do funkcji decode
#  
#
# http://czat.onet.pl/applet.html?N=pawq123&CH=16_17_18_lat&
#
my $VERSION=1.0;

use strict;
use IO::Socket ();

my $global_uo="niema";

sub unescape {
    my ($wyrazenie) = @_;
    $wyrazenie =~ s/%([0-9A-Fa-f]{2})/pack('c',hex($1))/ge;
    return $wyrazenie;
}


sub grab_escaped_coded {
  my ($ksywka)=@_;
  my $onethost="213.180.130.201";
  my $onetport=80;
  my $val;
  my $th = IO::Socket::INET->new('PeerAddr' => $onethost,
				 'PeerPort' => $onetport,
				 'Proto'=>'tcp'
				 );
  if (!$th) {
    print "[-] Blad polaczenia \n";
    exit 0;
  }
    my $msg = "GET /applet.html?N=$ksywka&CH=Kalambury7& HTTP/1.0\r\nHost: czat.onet.pl\r\n\r\n";
    syswrite($th, $msg, length($msg));
    $th->autoflush();

while ( $th) {
    my $tbuffer = "";
    my $result = sysread($th, $tbuffer, 1024);
    my $b = 1;
    if ($result>0) {
    chomp $tbuffer;
#    print $tbuffer;
#    print "\n";
    if ($tbuffer =~ /uoKey/i) {
	print "[?] ZNALZLEM ciag uoKey! \n";
#	print $tbuffer;print "\n";
	   $tbuffer =~ /.*.uoKey.*.\svalue=\'\)\s*\+\s*dk42\(\s*unescape\s*\(\s*(.*?)\s*\)/gi; 
#	   /unescape\(\'%.*.param\sname=%22uoKey%22\svalue=\'\)\s\+\sdk42\(\s*unescape\s*\(\s*(.*?)\s*\)/gi; 
		$global_uo = $1;
	    }
    if ($tbuffer =~ /\s*=\s*unescape/i) {
	do {
	    my $znak;
	    $result =  sysread($th, $znak, 1);
		    if  (!$result) {
			 $th->close();
			 $th = 0;
		     }
		     $tbuffer.=$znak;
		     if ( $znak =~ /\)/i) {
		        $b = 0;
		     }
	    
	} while ( $b != 0);
#    print "$tbuffer \n";
    $tbuffer =~ /unescape\('(.*?)'\)/i; 
	    $val = $1;
	    }
    }
    if  (!$result) {
		 $th->close();
		 $th = 0;
	 }
    }
    return $val;
}


sub dekoduj_html {
    my ($z94) = @_;
    my $z3f = "\x61\x7a\x41\x5a\x30\x39";
    my $z9e = 0;
    my @z8d = ();
    my @zc9 = ();
    my @zd2 = ();
    my @cos = ("\x61","\x7a","\x41","\x5a","\x30","\x39");
    
    for (my $zd1=0; $zd1 < length($z3f); $zd1+=2) {
    my $zca = unpack('c',$cos[$zd1]); # zamiana znaku na liczbe
    my $z56 = unpack('c',$cos[$zd1 + 1]);
        for (; $zca<=$z56; ++$zca) {
	    my $z14 = ($z9e++%16);
	    $z8d[$zca] = $z14*16;
	    $zc9[$zca] = $z14;
	}
    }

#    for ( my $z2d = 0; $z2d < 256; ++$z2d ) {
#       $zd2[$z2d] = $z2d;
#    }

    my $z84;
    
    for (my $zd1 = 0; $zd1 < length($z94); $zd1+=2 ) {
    my $sub = unpack('c',substr($z94,$zd1,1));
    my $sub2 = unpack('c',substr($z94,$zd1+1,1));
    $z84.=pack('c',$z8d[ $sub ]+$zc9[ $sub2 ]);
    }
    return $z84;

}

my $c=-1;    

sub dupek {
    $c++;
    return "\$l1l\[$c\]\=\$def";
}

sub dekoduj {
    my ($abc) = shift;
    my ($tabela) = shift;
    my @whole = ();
    my @def   = ();
    my @l1l   = ();
    
    for (my $bcd = 0; $bcd < 32; $bcd++) {
	$def[$bcd] = unpack('c',substr($abc,$bcd,1));
    }
#
# Tabela kodow - generowana automatycznie!
#
#              $l1l[0]=$def[19]^10;
#	       $l1l[1]=$def[22]^3;
#		...............
#    print "[TABELA]: $tabela \n";
    eval($tabela);
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

#
# MAIN MAIN  MAIN
#

print "[+] Lacze sie z czatem by pobrac zakodowany kod html\n";
   my $dupa = grab_escaped_coded('pawq123');
   $dupa = unescape($dupa);
   $dupa =~ /\s*=\s*\'(.*?)\'/i;

print "[+] Dekoduje dane wejsciowe do postaci HTML/JAVASCRIPT\n";
   my $cos = dekoduj_html($1);

print "[+] Wyciagam TABELE kodowania\n";
   $cos =~ /l1l\s*=\s*new\s*Array\((.*?)\);/i;

my $tmp = $1;

# Konwersja ostateczna do perlowskiego skladnika

     $tmp =~ s/\((0x.*?)\+(.*?)\-(.*?)\)/hex($1)+$2-hex($3)/ge;
     $tmp =~ s/lIl/dupek($1)/ge;
     $tmp =~ s/\,/\;\n/g;

print "[+] Dekodowanie uoKEY z uzyciem nowej funkcji\n";
    print "[GLOBAL_UO]: $global_uo \n";
    $global_uo = unescape($global_uo);
    $global_uo =~ s/\'//g;
#    print "[GLOBAL_UO-UNESCAPED]: $global_uo\n";

    print "tablea:\n$tmp\n";
    my $st = dekoduj($global_uo,$tmp);
#print "[uoKEY]: $st\n";