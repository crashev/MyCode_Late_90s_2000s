// WinZip - KeyGenerator
// by crashev'2002 <crashev@crashev.ebox.pl>
// Linux/Windows(djgpp) -> gcc winzip.c -o winzip/winzip.exe
//
/*
ASM: 
    ECX = Name Name 

First generating second Part Of Serial:

          MOV    DL,[ECX]
          XOR    EBX,EBX
          XOR    EAX,EAX
          MOV    ESI,ECX        ; ESI = ECX = Name Name        
          XOR    EDI,EDI
test:     TEST   Dl,DL
          JZ     00407B6E(done)
          MOVZX  DX,DL
          IMUL   EDX,EDI
          ADD    EBX,EDX
          INC    EDI
          INC    ESI
          JMP    TEST     
done:     ...
          ...


Second part is more complicated,this generates first 4 digits of Serial:
ECX = Name Name
         MOV    ESI,ECX
         MOV    CL,[ECX]
         TEST   CL,CL
         JZ     DONE2
         MOVZX  CX,CL          ; 
         ....
inside CALL:
         XOR    ECX,ECX
         MOV    CH,[EBP+0C]       ; CH=name[x]
START:   MOV    ESI,EAX           ;        CH=XX
         XOR    ESI,ECX           ; ECX = 0000XX00
         TEST   SI,8000           ; if (!(esi&0x8000))
         JZ     FIRST
         ADD    EAX,EAX
         XOR    EAX,1021
         JMP    SECOND
FIRST:   SHL    EAX,1
SECOND:  SHL    ECX,1         
         DEC    EDX
         JMP    START
DONE2:
         ADD    EAX,63
         MOV    EAX,AX

EAX= Second Part Of Code ,that is realy first ;p



*/
#include <stdio.h>
#include <strings.h>
#include <ctype.h>

void chop(char *c)
{
	int i;
	for (i=0;i<=strlen(c);i++) if ((c[i]=='\r')||(c[i]=='\n')) c[i]=0x0;
}

int main(void) 
{
        char name[50];
        int  edi=0,esi=0,edx=0;
        int  ebx=0;
        int  dl=0;
        int  ecx,eax=0;
        int  c=0;
        
printf("
-= Serial KeyGenerator for WinZip =-
      Coded by crashev'2002 
* Do not use white spaces in front of Name * 

");

	printf("+ Enter Name: ");
	fgets(name,sizeof(name)-1,stdin);
        chop(name);
	printf("* Entered   : %s [%d]\n",name,strlen(name));


// Second Part Of Serial

dl=name[0];
while (1) 
{
        if (dl==0) goto done;
        edx=dl;
	edx=edx*edi;
        ebx+=edx;
	dl=name[esi+01];
        edi++;
        esi++;
}
done:

while (1) 
{
        ecx=name[c];      
	if (name[c]==0) goto done2;
        ecx=name[c]<<8;
        edx=8;
        esi=0; 
	while (edx) 
        {
	        esi=eax;
	        esi=esi^ecx;	

                  if  (!(esi&0x8000)) goto first;
	          eax+=eax;
	          eax=eax^0x1021;
	          goto second;
	   first:
	          eax=eax<<1;
	   second:
	          ecx=ecx<<1;
		  edx--;

	 }	
c++;
}
done2:
	eax+=0x63;
        printf("+ Serial    : %x%x \n",(eax&0x0000ffff),ebx);

}
