/* 

   KeyGenerator for Easy CD EXtractor 5.x
   coded by crashev/2002
   
Extracted from ASM code of Easy CD Extractor 5.03:


EBX=our email -> pawq@pawq.eu.org

        MOV    EDI,00061A80
        MOV    DWORD PTR,[EBP-04],0000EA60
        MOV    DWORD PTR,[EBP-08],000F4240

loop:   MOVSX  EAX,BYTE PTR [EBX]
        IMUL   EDX,EAX,0000C322
        MOVSX  ECX,BYTE PTR [EBX]
        IMUL   EAX,ECX,0000C322
        ADD    EAX,7228F32A
        ADD    EDI,EDX
        IMUL   EAX,[EBP-04]
        MOV    [EBP-04],EAX
        ADD    EDI,03243845
        ADD    DWORD PTR [EBP-04],00032483

        MOVSX  EDX,BYTE PTR [EBX]
        IMUL   ECX,EDX,00023223
        ADD    ECX,5DC09A57
        MOV    EAX,[EBP-08]
        DIV    ECX
        MOV    [EBP-08],EAX
        SUB    DOWRD PTR [EBP-08],03483445
        INC    DWORD PTR [EBP-0C]            -> licznik
        JMP    loop

// next part(y)
        MOV ECX,EDI
...
     (cut ,very long) 
*/

#include <stdio.h>
#include <ctype.h>
#include <strings.h>

void chop(char *c)
{
	int i;
	for (i=0;i<=strlen(c);i++) if ((c[i]=='\r')||(c[i]=='\n')) c[i]=0x0;
}


int main(void)
{
      unsigned int eax,edx,ecx,edi;
      unsigned int ebp04,ebp08;
      unsigned int ebx,esi; // next part(y)
      unsigned char al;
      int ebp0c=0; 		// counter
      char *klucz="\xFD\xED\x9E\x9c\x98\xe8\xfa\x93\xef\xf9\x9f\xeb\xe1\x99\xe6\xe2\xe4\xf3\xe0\xfe\x92\xf2\x9d\xf0\xe9\xec\xff\xee\xe7\xf8";
      char name[50];
      char ser[20];

      bzero(name,sizeof(name));
      bzero(ser,sizeof(ser));
printf("
-= Serial KeyGenerator for Easy CD Extractor Ver 5.x =-
           Coded by crashev'2002 
");
      
      printf("+ Enter email      : ");fgets(name,sizeof(name)-1,stdin);
              chop(name);

      edi   = 0x00061A80;
      ebp04 = 0x0000EA60;
      ebp08 = 0x000F4240;
      printf("* Entered email : %s [%d] \n",name,strlen(name));

while (ebp0c<strlen(name)) 
{
      eax     = name[ebp0c];
      edx     = eax * 0x00007327;
      ecx     = name[ebp0c];
      eax     = ecx * 0x0000C322;
      eax     += 0x7228F32A;
      edi     += edx;
      eax     = eax * ebp04;
      ebp04   = eax;
      edi    += 0x03243845;
      ebp04  += 0x00032483;
   
      edx    = name[ebp0c];
      ecx    = edx * 0x00023223;
      ecx   += 0x5DC09A57;
      eax    = ebp08;
      eax    = eax / ecx;
      ebp08  = eax;
      ebp08 -= 0x03483445;
      ebp0c++;
}

// next part(y)
     ecx     = edi;
     ebx     = 0x0000001E;
     ecx     = ecx * ebp04;
     eax     = ecx;
     eax     = eax * ebp08;
     edx     = 0;
     edx     = eax % ebx;
     eax     = eax / ebx;
     esi     = edx;
     edx     = ebp08;
     ebx     = 0x0000001e;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[0]  = al;     
     
     eax     = ebp04;
     eax    += edi;
     eax    += edx;
     edx     = 0;
     edx     = eax % ebx;
     eax     = eax / ebx;
     esi     = edx;
     edx     = ebp08;
     ebx     = 0x0000001e;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[1]  = al;     


     eax     = ecx;
     eax    += edx;
     edx     = 0;
     edx     = eax % ebx;
     eax     = eax / ebx;
     ebx     = edi;
     esi     = edx;
     ebx     = ebx * ebp08;
     al      = klucz[esi];
     edx     = ebp04;
     al     ^= 0xAA;
     ser[2]  = al;   

     esi     = 0x0000001e;
     eax     = ebx;
     eax    += edx;
     edx     = 0;
     edx     = eax % esi;
     eax     = eax / esi;
     esi     = edx;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[3]  = al;   

     esi     = 0x0000001e;
     eax     = ebp04;     
     eax     = eax * ebp08;
     eax    += edi;
     edx     = 0;
     edx     = eax % esi;
     eax     = eax / esi;
     esi     = edx;
     edx     = ebp08;
     edx++;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[4]  = al;   
     
     eax     = ecx;
     ecx     = edx;
     edx     = 0;
     edx     = eax % ecx;
     eax     = eax / ecx;
     edx     = 0;
     ecx     = 0x0000001e;
     edx     = eax % ecx;
     eax     = eax / ecx;
     esi     = edx;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[5]  = al;   
     
     eax     = ebp08;
     eax     = eax * ebp04;
     edx     = edi + 0x1;
     ecx     = edx;
     edx     = 0;
     edx     = eax % ecx;
     eax     = eax / ecx;
     edx     = 0;
     ecx     = 0x0000001e;
     edx     = eax % ecx;
     eax     = eax / ecx;
     esi     = edx;
     edx     = ebp04;
     edx++;
     ecx     = edx;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[6]  = al;   

     edx     = 0;
     eax     = ebx;
     edx     = eax % ecx;
     eax     = eax / ecx;
     edx     = 0;
     ecx     = 0x0000001e;
     edx     = eax % ecx;
     eax     = eax / ecx;
     esi     = edx;
     edx     = ebp04;
     ecx     = ebp08;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[7]  = al;   
     
     eax     = edi;
     eax    -= edx;
     edx     = 0;
     eax    -= ecx;
     ecx     = 0x0000001e;
     edx     = eax % ecx;
     eax     = eax / ecx;
     esi     = edx;
     edx     = ebp08;
     ecx     = 0x0000001e;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[8]  = al;   
     
     eax     = ebp04;
     eax    += edi;
     eax    -= edx;
     edx     = 0;
     edx     = eax % ecx;
     eax     = eax / ecx;
     esi     = edx;
     al      = klucz[esi];
     al     ^= 0xAA;
     ser[9]  = al;   
       
     printf(">> Serial: %s \n",ser);
}
