#!/usr/bin/perl
sub unescape {
    my ($wyrazenie) = @_;
    $wyrazenie =~ s/%([0-9A-Fa-f]{2})/pack('c',hex($1))/ge;
    return $wyrazenie;
}

sub dupek {
    $c++;
    return "\$l1l\[$c\]\=\$def";
}
     $tmp=unescape("function __zb8%28z94%29%7bvar z8d%3d%5b%5d%2czc9%3d%5b%5d%3bvar zd2%3d%5b%5d%3bvar z3f%3d%22%5cx61%5cx7a%5cx41%5cx5a%5cx30%5cx39%22%3b%7bvar z9e%3d%280xd%2b263%2d0x114%29%3bfor%28var zd1%3d%280xcc7%2b5582%2d0x2295%29%3bzd1%3cz3f%2elength%3bzd1%2b%3d%280x12bb%2b4530%2d0x246b%29%29%7bvar zca%3dz3f%2echarCodeAt%28zd1%29%2cz56%3dz3f%2echarCodeAt%28zd1%2b%280x7ae%2b3578%2d0x15a7%29%29%3bfor%28%3bzca%3c%3dz56%3b%2b%2bzca%29%7bvar z14%3d%28z9e%2b%2b%25%280x3c5%2b7196%2d0x1fd1%29%29%3bz8d%5bzca%5d%3dz14%2a%280x204f%2b1178%2d0x24d9%29%3bzc9%5bzca%5d%3dz14%3b%7d%7dfor%28var z2d%3d%280x872%2b273%2d0x983%29%3bz2d%3c%280x1a85%2b2777%2d0x245e%29%3b%2b%2bz2d%29zd2%5bz2d%5d%3dString%2efromCharCode%28z2d%29%3b%7dvar z84%3d%27%27%3bfor%28var zd1%3d%280x6eb%2b4793%2d0x19a4%29%3bzd1%3cz94%2elength%3bzd1%2b%3d%280x111f%2b5358%2d0x260b%29%29%7bz84%2b%3dzd2%5bz8d%5bz94%2echarCodeAt%28zd1%29%5d%2bzc9%5bz94%2echarCodeAt%28zd1%2b%280x271%2b7213%2d0x1e9d%29%29%5d%5d%3b%7dreturn z84%3b%7d%3b xjjZHN %3d __zb8%28xjjZHN%29%3b eval%28xjjZHN%29%3b xjjZHN %3d %27%27%3b %7d');{var z82=\"\x5c\";function __z9b(z07){while(z82.length<(0x5b0+4306-0x15ba)){z82+=z82+z82+z82+z82+z82;}var z84='',z5d=(0x613+5902-0x1d21);while((0x6ae+4399-0x17dc)){var zff=z07.indexOf(\"\x5e\",z5d);if(zff==-(0x2203+1166-0x2690))return z84+z07.substr(z5d);else{z84+=z07.substr(z5d,zff-z5d);var zd1=(0x1e93+1313-0x23b2);var z21=z07.charAt(zff+(0x907+1781-0xffb));while((0x1451+3608-0x2268)){var zab=z07.charAt(zff+zd1++);if(!(zab>=\"\x30\"&&zab<=\"\x39\"))break;else z21+=zab;}z21=parseInt(z21);z5d=zff+zd1-(0x194+2657-0xbf4);do{var z14=z21>z82.length?z82.length:z21;z21-=z14;z84+=z82.substr((0x27b+7467-0x1fa6),z14);}while(z21>z82.length);}}return z84;}};xrpggR = __z9b(xrpggR);eval(xrpggR);");
     $tmp =~ s/\((0x.*?)\+(.*?)\-(.*?)\)/hex($1)+$2-hex($3)/ge;
     $tmp =~ s/lIl/dupek($1)/ge;
     $tmp =~ s/\,/\;\n/g;
printf "out: $tmp\n";