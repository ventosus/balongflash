//  Low-Level Verfahren der Arbeit mit dem konsistent Hafen und HDLC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>

#include "hdlcio.h"
#include "util.h"

unsigned int nand_cmd=0x1b400000;
unsigned int spp=0;
unsigned int pagesize=0;
unsigned int sectorsize=512;
unsigned int maxblock=0;     // Allgemeine Informationen Anzahl von Blöcke флешки
char flash_mfr[30]={0};
char flash_descr[30]={0};
unsigned int oobsize=0;

static char pdev[500]; // Name konsistent der Hafen

int siofd; // fd für die der Arbeit mit dem Konsequent Hafen
struct termios sioparm;
//int siofd; // fd für die der Arbeit mit dem Konsequent Hafen

//*************************************************
//*    Referenz Puffer in der Modem
//*************************************************
unsigned int send_unframed_buf(char* outcmdbuf, unsigned int outlen) {


tcflush(siofd,TCIOFLUSH);  // verwerfen ungelesen Puffer Eingabe

write(siofd,"\x7e",1);  // senden Präfix

if (write(siofd,outcmdbuf,outlen) == 0) {   printf("\n Fehler Aufzeichnungen Teams");return 0;  }
tcdrain(siofd);  // warten auf Abschluss Ausgabe Block

return 1;
}

//******************************************************************************************
//* Empfang Puffer mit dem antworte von der Modem
//*
//*  masslen - Anzahl von Bytes, von der vereinheitlicht blockieren ohne Analyse Funktion das Ende 7F
//******************************************************************************************

unsigned int receive_reply(char* iobuf, int masslen) {
  
int i,iolen,escflag,incount;
unsigned char c;
unsigned int res;
unsigned char replybuf[14000];

incount=0;
if (read(siofd,&c,1) != 1) {
//  printf("\n Nein antworte aus Modem");
  return 0; // Modem nicht beantwortet oder beantwortet falsch
}
//if (c != 0x7e) {
//  printf("\n Der erste Bytes antworte - nicht 7e: %02x",c);
//  return 0; // Modem nicht beantwortet oder beantwortet falsch
//}
replybuf[incount++]=c;

// lesen Array von Daten vereinheitlicht blockieren an Verarbeitung Teams 03
if (masslen != 0) {
 res=read(siofd,replybuf+1,masslen-1);
 if (res != (masslen-1)) {
   printf("\nZu viel kurz antworte aus Modem: %i Bytes, wurde erwartet %i Bytes\n",res+1,masslen);
   dump(replybuf,res+1,0);
   return 0;
 }  
 incount+=masslen-1; // an uns in der Puffer schon da ist masslen Bytes
// printf("\n ------ it mass --------");
// dump(replybuf,incount,0);
}

// akzeptieren übrig Schwanz Puffer
while (read(siofd,&c,1) == 1)  {
 replybuf[incount++]=c;
// printf("\n-- %02x",c);
 if (c == 0x7e) break;
}

// Transformation angenommen Puffer für die löschen ESC-Zeichen
escflag=0;
iolen=0;
for (i=0;i<incount;i++) { 
  c=replybuf[i];
  if ((c == 0x7e)&&(iolen != 0)) {
    iobuf[iolen++]=0x7e;
    break;
  }  
  if (c == 0x7d) {
    escflag=1;
    continue;
  }
  if (escflag == 1) { 
    c|=0x20;
    escflag=0;
  }  
  iobuf[iolen++]=c;
}  
return iolen;

}

//***********************************************************
//* Transformation Befehl Puffer mit dem Escape-Substitution
//***********************************************************
unsigned int convert_cmdbuf(char* incmdbuf, int blen, char* outcmdbuf) {

int i,iolen,bcnt;
unsigned char cmdbuf[14096];

bcnt=blen;
memcpy(cmdbuf,incmdbuf,blen);
// Wir treten ein CRC in der das Ende Puffer
*((unsigned short*)(cmdbuf+bcnt))=crc16(cmdbuf,bcnt);
bcnt+=2;

// Bildung von Daten mit dem Screening ESC-Sequenzen
iolen=0;
outcmdbuf[iolen++]=cmdbuf[0];  // der erste Bytes kopieren ohne Änderungen
for(i=1;i<bcnt;i++) {
   switch (cmdbuf[i]) {
     case 0x7e:
       outcmdbuf[iolen++]=0x7d;
       outcmdbuf[iolen++]=0x5e;
       break;
      
     case 0x7d:
       outcmdbuf[iolen++]=0x7d;
       outcmdbuf[iolen++]=0x5d;
       break;
      
     default:
       outcmdbuf[iolen++]=cmdbuf[i];
   }
 }
outcmdbuf[iolen++]=0x7e; // Finale Bytes
outcmdbuf[iolen]=0;
return iolen;
}

//***************************************************
//*  Senden Teams in der Hafen und Empfangen Ergebnis  *
//***************************************************
int send_cmd(unsigned char* incmdbuf, int blen, unsigned char* iobuf) {
  
unsigned char outcmdbuf[14096];
unsigned int  iolen;

iolen=convert_cmdbuf(incmdbuf,blen,outcmdbuf);  
if (!send_unframed_buf(outcmdbuf,iolen)) return 0; // Fehler Übertragung von Teams
return receive_reply(iobuf,0);
}

//***************************************************
// Eröffnung und tunen konsistent der Hafen
//***************************************************

int open_port(char* devname) {


int i,dflag=1;
char devstr[200]={0};


if (strlen(devname) != 0) strcpy(pdev,devname);   // behalten Name der Hafen  
else strcpy(devname,"/dev/ttyUSB0");  // wenn Name der Hafen nicht war gegeben

// Stattdessen voll Name Geräte ist erlaubt übertragen nur Nummer ttyUSB-der Hafen

// Überprüfung Name Geräte von Verfügbarkeit nicht digital Symbole
for(i=0;i<strlen(devname);i++) {
  if ((devname[i]<'0') || (devname[i]>'9')) dflag=0;
}
// Wenn die in der Linie - nur Ziffern, hinzufügen Präfix /dev/ttyUSB

if (dflag) strcpy(devstr,"/dev/ttyUSB");

// kopieren Name Geräte
strcat(devstr,devname);

siofd = open(devstr, O_RDWR | O_NOCTTY |O_SYNC);
if (siofd == -1) {
  printf("\n! - Seriell Hafen %s nicht öffnet sich\n", devname); 
  exit(0);
}
bzero(&sioparm, sizeof(sioparm)); // vorbereiten Block Attribute termios
sioparm.c_cflag = B115200 | CS8 | CLOCAL | CREAD ;
sioparm.c_iflag = 0;  // INPCK;
sioparm.c_oflag = 0;
sioparm.c_lflag = 0;
sioparm.c_cc[VTIME]=30; // timeout  
sioparm.c_cc[VMIN]=0;  
tcsetattr(siofd, TCSANOW, &sioparm);

tcflush(siofd,TCIOFLUSH);  // Clearing Ausgabe Puffer

return 1;
}


//*************************************
// Anpassen Zeit Erwartung der Hafen
//*************************************

void port_timeout(int timeout) {

bzero(&sioparm, sizeof(sioparm)); // vorbereiten Block Attribute termios
sioparm.c_cflag = B115200 | CS8 | CLOCAL | CREAD ;
sioparm.c_iflag = 0;  // INPCK;
sioparm.c_oflag = 0;
sioparm.c_lflag = 0;
sioparm.c_cc[VTIME]=timeout; // timeout  
sioparm.c_cc[VMIN]=0;  
tcsetattr(siofd, TCSANOW, &sioparm);
}

//*************************************************
//*  Suche Datei auf dem die Nummer in der die Verzeichnis
//*
//* num - # Datei
//* filename - Puffer für die voll Name Datei
//* id - variabel, in der welche wird sein aufgezeichnet Kennung Abschnitt
//*
//* return 0 - nicht gefunden
//*        1 - gefunden
//*************************************************
int find_file(int num, char* dirname, char* filename,unsigned int* id, unsigned int* size) {

DIR* fdir;
FILE* in;
unsigned int pt;
struct dirent* dentry;
char fpattern[5];

sprintf(fpattern,"%02i",num); // Muster für die Suchmaschine Datei auf dem 3 Zahlen Zahlen
fdir=opendir(dirname);
if (fdir == 0) {
  printf("\n Katalog %s nicht öffnet sich\n",dirname);
  exit(1);
}

// Prinzipal Zyklus - Suche das Recht uns Datei
while ((dentry=readdir(fdir)) != 0) {
  if (dentry->d_type != DT_REG) continue; // überspringen all das außer für regelmäßig Dateien
  if (strncmp(dentry->d_name,fpattern,2) == 0) break; // habe gefunden das Recht Datei. Genauer gesagt, Datei mit dem notwendig 3 in Zahlen in der der Anfang Name.
}

closedir(fdir);
// Form fertig Name Datei in der Puffer Ergebnis
if (dentry == 0) return 0; // nicht habe gefunden
strcpy(filename,dirname);
strcat(filename,"/");
// kopieren Name Datei in der Puffer Ergebnis
strcat(filename,dentry->d_name);  

// 00-00000200-M3Boot.bin
//überprüfen Name Datei von Verfügbarkeit Zeichen '-'
if ((dentry->d_name[2] != '-') || (dentry->d_name[11] != '-')) {
  printf("\n Falsch formatieren Name Datei - %s\n",dentry->d_name);
  exit(1);
}

// überprüfen digital Feld ID Abschnitt
if (strspn(dentry->d_name+3,"0123456789AaBbCcDdEeFf") != 8) {
  printf("\n Fehler in der Kennung Abschnitt - nicht numerisch Zeichen - %s\n",filename);
  exit(1);
}  
sscanf(dentry->d_name+3,"%8x",id);

// Überprüfung Zugänglichkeit und Lesbarkeit Datei
in=fopen(filename,"r");
if (in == 0) {
  printf("\n Fehler Entdeckungen Datei %s\n",filename);
  exit(1);
}
if (fread(&pt,1,4,in) != 4) {
  printf("\n Fehler Lesungen Datei %s\n",filename);
  exit(1);
}
  
// überprüfen, das Datei - ungekocht Bild, ohne Kopfzeile
if (pt == 0xa55aaa55) {
  printf("\n Datei %s hat a Beschriftung - für die Firmware nicht ist geeignet\n",filename);
  exit(1);
}


// Was? noch du kannst Auschecken? Tschüss nicht erfunden.  

//  Wir bekommen Größe Datei
fseek(in,0,SEEK_END);
*size=ftell(in);
fclose(in);

return 1;
}

//****************************************************
//*  Senden Modem AT-Teams
//*  
//* cmd - Puffer mit dem Mannschaft
//* rbuf - Puffer für die Aufzeichnungen antworte
//*
//* Rückkehr lang antworte
//****************************************************
int atcmd(char* cmd, char* rbuf) {

int res;
char cbuf[128];

strcpy(cbuf,"AT");
strcat(cbuf,cmd);
strcat(cbuf,"\r");

port_timeout(100);
// Wir reinigen Puffer Empfänger und Sender
tcflush(siofd,TCIOFLUSH);

// Versand Teams
write(siofd,cbuf,strlen(cbuf));
usleep(100000);

// lesen Ergebnis
res=read(siofd,rbuf,200);
return res;
}
  
