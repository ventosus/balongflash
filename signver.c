// 

// Verfahren Verarbeitung digital Signaturen
// 
#include <stdio.h>
#include <stdint.h>
#ifndef WIN32
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#else
#include <windows.h>
#include "getopt.h"
#include "printf.h"
#include "buildno.h"
#endif

#include "hdlcio.h"
#include "ptable.h"
#include "flasher.h"
#include "util.h"
#include "zlib.h"

// Diagramm Parameter Schlüssel -g

struct {
  uint8_t type;
  uint32_t len;
  char* descr;
} signbase[] = {
  {1,2958,"Grundlegend Firmware"},
  {1,2694,"Firmware E3372s-stick"},
  {2,1110,"Webinterface+ISO für die HLINK-Modem"},
  {6,1110,"Webinterface+ISO für die HLINK-Modem"},
  {2,846,"ISO (dashboard) für die stick-Modem"},
  {7,3750,"Firmware+ISO+Webinterface"},
};

#define signbaselen 6

// Diagramm Arten von Signaturen
char* fwtypes[]={
"UNKNOWN",        // 0
"ONLY_FW",        // 1
"ONLY_ISO",       // 2
"FW_ISO",         // 3
"ONLY_WEBUI",     // 4
"FW_WEBUI",       // 5
"ISO_WEBUI",      // 6
"FW_ISO_WEBUI"    // 7
};  


// resultierend Linie ^signver-Teams
uint8_t signver[200];

// Flagge Regime digital Signaturen
extern int gflag;

// Flagge eingeben Firmware
extern int dflag;

// Parameter aktuell digital Signaturen
uint32_t signtype; // Geben Sie ein Firmware
uint32_t signlen;  // Länge Signaturen

int32_t serach_sign();

// Ja öffnen Schlüssel für die ^signver
char signver_hash[100]="778A8D175E602B7B779D9E05C330B5279B0661BF2EED99A20445B366D63DD697";

//****************************************************
//* Empfangen Beschreibung eingeben Firmware auf dem Code
//****************************************************
char* fw_description(uint8_t code) {
  
return fwtypes[code&0x7];  
}

//****************************************************
//* Empfangen Liste von Arten von Firmware
//****************************************************
void dlist() {
  
int i;

printf("\n #  Beschreibung\n--------------------------------------");
for(i=1;i<8;i++) {
  printf("\n %i  %s",i,fw_description(i));
}
printf("\n\n");
exit(0);
}

//***************************************************
//* Verarbeitung Parameter Schlüssel -d
//***************************************************
void dparm(char* sparm) {
  
if (dflag != 0) {
  printf("\n Duplizieren Hinweis -d\n\n");
  exit(-1);
}  

if (sparm[0] == 'l') {
  dlist();
  exit(0);
}  
sscanf(sparm,"%x",&dload_id);
if ((dload_id == 0) || (dload_id >7)) {
  printf("\n Falsch Bedeutung Schlüssel -d\n\n");
  exit(-1);
}
dflag=1;
}


//****************************************************
//* Empfangen Liste von Parameter Schlüssel -g
//****************************************************
void glist() {
  
int i;
printf("\n #  Länge  Geben Sie ein die Beschreibung \n--------------------------------------");
for (i=0; i<signbaselen; i++) {
  printf("\n%1i  %5i  %2i   %s",i,signbase[i].len,signbase[i].type,signbase[i].descr);
}
printf("\n\n Auch du kannst angeben beliebig Parameter Signaturen in der formatieren:\n  -g *,type,len\n\n");
exit(0);
}

//***************************************************
//* Verarbeitung Parameter Schlüssel -g
//***************************************************
void gparm(char* sparm) {
  
int index;  
char* sptr;
char parm[100];


if (gflag != 0) {
  printf("\n Duplizieren Hinweis -g\n\n");
  exit(-1);
}  

strcpy(parm,sparm); // lokal kopieren Parameter

if (parm[0] == 'l') {
  glist();
  exit(0);
}  

if (parm[0] == 'd') {
  // Verbot автоопределения Signaturen
  gflag = -1;
  return;
} 

if (strncmp(parm,"*,",2) == 0) {
  // beliebig Parameter
  // hervorheben lang
  sptr=strrchr(parm,',');
  if (sptr == 0) goto perror;
  signlen=atoi(sptr+1);
  *sptr=0;
  // hervorheben Geben Sie ein Abschnitt
  sptr=strrchr(parm,',');
  if (sptr == 0) goto perror;
  signtype=atoi(sptr+1);
  if (fw_description(signtype) == 0) {
    printf("\n Schlüssel -g: unbekannt Geben Sie ein Firmware - %i\n",signtype);
    exit(-1);
  }  
}
else {  
  index=atoi(parm);
  if (index >= signbaselen) goto perror;
  signlen=signbase[index].len;
  signtype=signbase[index].type;
}

gflag=1;
// printf("\nstr - %s",signver);
return;

perror:
 printf("\n Fehler in der Parameter Schlüssel -g\n");
 exit(-1);
} 
  

//***************************************************
//* Senden digital Signaturen
//***************************************************
void send_signver() {
  
uint32_t res;
// antworte von ^signver
unsigned char SVrsp[]={0x0d, 0x0a, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x4f, 0x4b, 0x0d, 0x0a};
uint8_t replybuf[200];
  
if (gflag == 0) {  
  // автоопределение digital Signaturen
  signtype=dload_id&0x7;
  signlen=serach_sign();
  if (signlen == -1) return; // Unterschrift in der Datei nicht gefunden
}

printf("\n Modus digital Signaturen: %s (%i Bytes)",fw_description(signtype),signlen);
sprintf(signver,"^SIGNVER=%i,0,%s,%i",signtype,signver_hash,signlen);
res=atcmd(signver,replybuf);
if ( (res<sizeof(SVrsp)) || (memcmp(replybuf,SVrsp,sizeof(SVrsp)) != 0) ) {
   printf("\n ! Fehler Überprüfung digital Signaturen - %02x\n",replybuf[2]);
   exit(-2);
}
}

//***************************************************
//* Suche digital Signaturen in der Firmware
//***************************************************
int32_t serach_sign() {

int i,j;
uint32_t pt;
uint32_t signsize;

for (i=0;i<2;i++) {
  if (i == npart) break;
  pt=*((uint32_t*)&ptable[i].pimage[ptable[i].hd.psize-4]);
  if (pt == 0xffaaaffa) { 
    // Unterschrift gefunden
    signsize=*((uint32_t*)&ptable[i].pimage[ptable[i].hd.psize-12]);
    // hervorheben ihre eigenen öffnen Schlüssel
    for(j=0;j<32;j++) {
     sprintf(signver_hash+2*j,"%02X",ptable[i].pimage[ptable[i].hd.psize-signsize+6+j]);
    }
    return signsize;
  }
}
// nicht gefunden
return -1;
}