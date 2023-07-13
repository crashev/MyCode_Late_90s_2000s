#!/usr/bin/perl
#
# Kodowanie onetowskiego czatu
# Pawel Furtak <crashev@ebox.pl>, <paul@eweb-pro.com>
# // Dekoduje html z czatu onetu

#var xrpggR =
my $out = unescape('%7bvar xjjZHN %3d %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%27%3b%3bfunction __zb8%28z94%29%7bvar z8d%3d%5b%5d%2czc9%3d%5b%5d%3bvar zd2%3d%5b%5d%3bvar z3f%3d%22%5cx61%5cx7a%5cx41%5cx5a%5cx30%5cx39%22%3b%7bvar z9e%3d%280xd%2b263%2d0x114%29%3bfor%28var zd1%3d%280xcc7%2b5582%2d0x2295%29%3bzd1%3cz3f%2elength%3bzd1%2b%3d%280x12bb%2b4530%2d0x246b%29%29%7bvar zca%3dz3f%2echarCodeAt%28zd1%29%2cz56%3dz3f%2echarCodeAt%28zd1%2b%280x7ae%2b3578%2d0x15a7%29%29%3bfor%28%3bzca%3c%3dz56%3b%2b%2bzca%29%7bvar z14%3d%28z9e%2b%2b%25%280x3c5%2b7196%2d0x1fd1%29%29%3bz8d%5bzca%5d%3dz14%2a%280x204f%2b1178%2d0x24d9%29%3bzc9%5bzca%5d%3dz14%3b%7d%7dfor%28var z2d%3d%280x872%2b273%2d0x983%29%3bz2d%3c%280x1a85%2b2777%2d0x245e%29%3b%2b%2bz2d%29zd2%5bz2d%5d%3dString%2efromCharCode%28z2d%29%3b%7dvar z84%3d%27%27%3bfor%28var zd1%3d%280x6eb%2b4793%2d0x19a4%29%3bzd1%3cz94%2elength%3bzd1%2b%3d%280x111f%2b5358%2d0x260b%29%29%7bz84%2b%3dzd2%5bz8d%5bz94%2echarCodeAt%28zd1%29%5d%2bzc9%5bz94%2echarCodeAt%28zd1%2b%280x271%2b7213%2d0x1e9d%29%29%5d%5d%3b%7dreturn z84%3b%7d%3b xjjZHN %3d __zb8%28xjjZHN%29%3b eval%28xjjZHN%29%3b xjjZHN %3d %27%27%3b %7d');

print "[+] Unescaped \n";
print "$out\n";
print "[+] Decoding HTML \n";
my $kupa = "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";
my $cos = dekoduj_html($kupa);
print "$cos\n";


sub unescape {
    my ($wyrazenie) = @_;
    $wyrazenie =~ s/%7b/{/gi;
    $wyrazenie =~ s/%7d/}/gi;
    $wyrazenie =~ s/%3b/;/gi;    
    $wyrazenie =~ s/%3d/=/gi;    
    $wyrazenie =~ s/%3c/\</gi;    
    $wyrazenie =~ s/%3e/\>/gi;    
    $wyrazenie =~ s/%27/\'/gi;    
    $wyrazenie =~ s/%28/\(/gi;    
    $wyrazenie =~ s/%29/\)/gi;    
    $wyrazenie =~ s/%2b/\+/gi;    
    $wyrazenie =~ s/%2c/\,/gi;    
    $wyrazenie =~ s/%2d/\-/gi;    
    $wyrazenie =~ s/%2e/\./gi;     
    $wyrazenie =~ s/%2a/\*/gi;    
    $wyrazenie =~ s/%22/\"/gi;    
    $wyrazenie =~ s/%25/\%/gi;    
    $wyrazenie =~ s/%5b/\[/gi;    
    $wyrazenie =~ s/%5c/\\/gi;    
    $wyrazenie =~ s/%5d/\]/gi;    
    return $wyrazenie;
}

sub dekoduj_html {
    my ($z94) = @_;
    my $z3f = "\x61\x7a\x41\x5a\x30\x39";
    my @z8d = ();
    my @zc9 = ();
    my @zd2 = ();
    my @cos = ("\x61","\x7a","\x41","\x5a","\x30","\x39");

    my $z9e = 0;
    
    for (my $zd1=0; $zd1 < length($z3f); $zd1+=2) {
    my $zca = unpack('c',$cos[$zd1]); # zamiana znaku na liczbe
    my $z56 = unpack('c',$cos[$zd1 + 1]);
        for (; $zca<=$z56; ++$zca) {
	    my $z14 = ($z9e++%16);
	    $z8d[$zca] = $z14*16;
	    $zc9[$zca] = $z14;
	}
    }

    for ( my $z2d = 0; $z2d < 256; ++$z2d ) {
       $zd2[$z2d] = $z2d;
    }

    my $z84;
    
    for (my $zd1 = 0; $zd1 < length($z94); $zd1+=2 ) {
    my $sub = unpack('c',substr($z94,$zd1,1));
    my $sub2 = unpack('c',substr($z94,$zd1+1,1));
    $z84.=pack('c',$z8d[ $sub ]+$zc9[ $sub2 ]);
    }
    return $z84;

}

sub dekoduj_html1 {
    my ($z07) = @_;
    my $z82="\x5c";
    
    while (length($z82)<200) {
        $z82+=$z82+$z82+$z82+$z82+$z82;
    }
    
    my $z84;
    my $z5d=0;
    
#    while(1) {  
#	my $zff=index($z07,"^");
#	if($zff==-1)
#	    return $z84+substr($z07,$z5d);
#	    $z07.substr($z5d);
#	else {
#	$z84+=$z07.substr($z5d;$zff-$z5d);
#	my $zd1=2;
#	my $z21=$z07.charAt($zff+1);
#	
#	while(1) {
#		my $zab=$z07.charAt($zff+$zd1++);
#	if(!($zab>="0"&&$zab<="9"))
#		break;
#	    else 
#	    $z21+=$zab;
#	}
#	$z21=parseInt($z21);
#	$z5d=$zff+$zd1-1;
#	do { 
#	    my $z14=$z21>$z82.length?$z82.length:$z21;$z21-=$z14;
#	    $z84+=$z82.substr(0;$z14);
#	} while($z21>$z82.length);
#    }}


}

