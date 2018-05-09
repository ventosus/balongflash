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

#define true 1
#define false 0


//***************************************************
//* Lagerung der Code Fehler
//***************************************************
int errcode;


//***************************************************
//* Fazit der Code Fehler Teams
//***************************************************
void printerr() {
  
if (errcode == -1) printf(" - Zeitüberschreitung Teams\n");
else printf(" - Code Fehler %02x\n",errcode);
}

//***************************************************
// Senden Teams start Abschnitt
// 
//  code - 32-bisschen Code Abschnitt
//  size - fertig Größe beschreibbar Abschnitt
// 
//*  Wirkung:
//  false - Fehler
//  true - Befehl angenommen Modem
//***************************************************
int dload_start(uint32_t code,uint32_t size) {

uint32_t iolen;  
uint8_t replybuf[4096];
  
#ifndef WIN32
static struct __attribute__ ((__packed__))  {
#else
#pragma pack(push,1)
static struct {
#endif
  uint8_t cmd;
  uint32_t code;
  uint32_t size;
  uint8_t pool[3];
} cmd_dload_init =  {0x41,0,0,{0,0,0}};
#ifdef WIN32
#pragma pack(pop)
#endif


cmd_dload_init.code=htonl(code);
cmd_dload_init.size=htonl(size);
iolen=send_cmd((uint8_t*)&cmd_dload_init,sizeof(cmd_dload_init),replybuf);
errcode=replybuf[3];
if ((iolen == 0) || (replybuf[1] != 2)) {
  if (iolen == 0) errcode=-1;
  return false;
}  
else return true;
}  

//***************************************************
// Senden Block Abschnitt
// 
//  blk - # Block
//  pimage - Adresse start Bild Abschnitt in der Speicher
// 
//*  Wirkung:
//  false - Fehler
//  true - Befehl angenommen Modem
//***************************************************
int dload_block(uint32_t part, uint32_t blk, uint8_t* pimage) {

uint32_t res,blksize,iolen;
uint8_t replybuf[4096];

#ifndef WIN32
static struct __attribute__ ((__packed__)) {
#else
#pragma pack(push,1)
static struct {
#endif
  uint8_t cmd;
  uint32_t blk;
  uint16_t bsize;
  uint8_t data[fblock];
} cmd_dload_block;  
#ifdef WIN32
#pragma pack(pop)
#endif

blksize=fblock; // primär Bedeutung Größe Block
res=ptable[part].hd.psize-blk*fblock;  // Größe übrig Stück zu das Ende Datei
if (res<fblock) blksize=res;  // richtig Größe das letzte Block

// Code Teams
cmd_dload_block.cmd=0x42;
// Nummer Block
cmd_dload_block.blk=htonl(blk+1);
// Größe Block
cmd_dload_block.bsize=htons(blksize);
// Teil von Daten von der Bild Abschnitt
memcpy(cmd_dload_block.data,pimage+blk*fblock,blksize);
// senden Block in der Modem
iolen=send_cmd((uint8_t*)&cmd_dload_block,sizeof(cmd_dload_block)-fblock+blksize,replybuf); // senden Befehl

errcode=replybuf[3];
if ((iolen == 0) || (replybuf[1] != 2))  {
  if (iolen == 0) errcode=-1;
  return false;
}
return true;
}

  
//***************************************************
// Fertigstellung Aufzeichnungen Abschnitt
// 
//  code - Code Abschnitt
//  size - Größe Abschnitt
// 
//*  Wirkung:
//  false - Fehler
//  true - Befehl angenommen Modem
//***************************************************
int dload_end(uint32_t code, uint32_t size) {

uint32_t iolen;
uint8_t replybuf[4096];

#ifndef WIN32
static struct __attribute__ ((__packed__)) {
#else
#pragma pack(push,1)
static struct {
#endif
  uint8_t cmd;
  uint32_t size;
  uint8_t garbage[3];
  uint32_t code;
  uint8_t garbage1[11];
} cmd_dload_end;
#ifdef WIN32
#pragma pack(pop)
#endif


cmd_dload_end.cmd=0x43;
cmd_dload_end.code=htonl(code);
cmd_dload_end.size=htonl(size);
iolen=send_cmd((uint8_t*)&cmd_dload_end,sizeof(cmd_dload_end),replybuf);
errcode=replybuf[3];
if ((iolen == 0) || (replybuf[1] != 2)) {
  if (iolen == 0) errcode=-1;
  return false;
}  
return true;
}  



//***************************************************
//* Aufnahme in der Modem von allen Abschnitte von der Tabellen
//***************************************************
void flash_all() {

int32_t part;
uint32_t blk,maxblock;

printf("\n##  ---- Name Abschnitt ---- aufgezeichnet");
// Haupt Zyklus Aufzeichnungen Abschnitte
for(part=0;part<npart;part++) {
printf("\n");  
//  printf("\n02i %s)",part,ptable[part].pname);
 // Befehl start Abschnitt
 if (!dload_start(ptable[part].hd.code,ptable[part].hd.psize)) {
   printf("\r! Abgelehnt Beschriftung Abschnitt %i (%s)",part,ptable[part].pname);
   printerr();
   exit(-2);
 }  
    
 maxblock=(ptable[part].hd.psize+(fblock-1))/fblock; // Anzahl von Blöcke in der Abschnitt
 // Blockieren Zyklus Übertragung von Bild Abschnitt
 for(blk=0;blk<maxblock;blk++) {
  // Fazit Prozent aufgezeichnet
  printf("\r%02i  %-20s  %i%%",part,ptable[part].pname,(blk+1)*100/maxblock); 

    // Wir senden regelmäßig Block
  if (!dload_block(part,blk,ptable[part].pimage)) {
   printf("\n! Abgelehnt Block %i Abschnitt %i (%s)",blk,part,ptable[part].pname);
   printerr();
   exit(-2);
  }  
 }    

// schließen Abschnitt
 if (!dload_end(ptable[part].hd.code,ptable[part].hd.psize)) {
   printf("\n! Fehler Schließen Abschnitt %i (%s)",part,ptable[part].pname);
   printerr();
   exit(-2);
 }  
} // das Ende Zyklus auf dem Abschnitte
}
