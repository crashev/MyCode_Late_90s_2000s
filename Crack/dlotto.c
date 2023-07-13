//
// Duzy Lotek v1.3 by Breko http://www.breko.pl
// For Educational Purposes Only
//      BUY THIS GREAT SOFTWARE!!      - KUPUJCIE SOFT POLSKi
//
// Linux/Windows(djgpp): gcc dlotto.c -o dlotto/dlotto.exe
//

/*
Snip Code:
        EAX = length of Name+SecondName+1 we can say,+1 = space between

        MOV    EBX,EAX
        IMUL   EAX,EBX,000FA144
        MOV    EBX,EAX
        MOV    EAX,EBX
        MOV    ECX,00000003
        CDQ             ; EDX = 0 (depends on D flag? ;>)
        IDIV   ECX        ; maybe not familiar to everyone it divides value
                        ; in eax by value in ECX ,result is stored in eax
        MOV    EBX,EAX
        XOR    EBX,05
        MOV    EBX,[EBP+04]
        MOVZX  EAX,[EAX]
        ADD    EBX,EAX
        MOV    EBX,[EBP+04]
        MOVZX  EAX,[EAX+01]
        ADD    EBX,EAX
        MOV    EBX,[EBP+02]
        MOVZX  EAX,[EAX]
        ADD    EBX,EAX
        MOV    EBX,[EBP+03]
        MOVZX  EAX,[EAX]
        ADD    EBX,EAX

EBX = Our Serial Gnerated on Length of Name+SecondName+1
*/

#include <stdio.h>
#include <ctype.h>

// C interpretation

void chop(char *c)
{
	int i;
	for (i=0;i<=strlen(c);i++) if ((c[i]=='\r')||(c[i]=='\n')) c[i]=0x0;
}


int main(void)
{
   int ebx,eax,ecx;
   char name[30];
   char sname[30];

printf("
-= Serial KeyGenerator for Duzy Lotek v1.3 =-
           Coded by crashev'2002 
");
printf("+ Name      : ");fgets(name,sizeof(name)-1,stdin);
printf("+ SecName   : ");fgets(sname,sizeof(sname)-1,stdin);
	chop(name);
	chop(sname);
eax=strlen(name)+strlen(sname)+1;
printf("* FullName  : %s %s [%d/%x]\n",name,sname,eax,eax);
	ebx=eax;
        eax=ebx*0x000FA144;
        ebx=eax;
        eax=ebx;
        ecx=3;
        eax=eax / 3;
        ebx=eax;
        ebx=ebx^0x05;
        eax=name[0];
        ebx+=eax;
        eax=name[1];
        ebx+=eax;
        eax=name[2];
        ebx+=eax;
        eax=name[3];
        ebx+=eax;
printf("++ Serial   : %d",ebx,ebx);
return 0;
}
