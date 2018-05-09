// Verfahren der Arbeit mit dem Tabelle Abschnitte

#include <stdio.h>
#include <stdint.h>
#ifndef WIN32
#include <string.h>
#include <stdlib.h>
#else
#include <windows.h>
#include "printf.h"
#endif

#include <zlib.h>

#include "ptable.h"
#include "hdlcio.h"
#include "util.h"
#include "signver.h"

int32_t lzma_decode(uint8_t* inbuf,uint32_t fsize,uint8_t* outbuf);

//******************************************************
//*  Suche symbolisch Name Abschnitt auf dem seins Code
//******************************************************

void  find_pname(unsigned int id,unsigned char* pname) {

unsigned int j;
struct {
  char name[20];
  int code;
} pcodes[]={ 
  {"M3Boot",0x20000}, 
  {"M3Boot-ptable",0x10000}, 
  {"M3Boot_R11",0x200000}, 
  {"Ptable",0x10000},
  {"Ptable_ext_A",0x480000},
  {"Ptable_ext_B",0x490000},
  {"Fastboot",0x110000},
  {"Logo",0x130000},
  {"Kernel",0x30000},
  {"Kernel_R11",0x90000},
  {"DTS_R11",0x270000},
  {"VxWorks",0x40000},
  {"VxWorks_R11",0x220000},
  {"M3Image",0x50000},
  {"M3Image_R11",0x230000},
  {"DSP",0x60000},
  {"DSP_R11",0x240000},
  {"Nvdload",0x70000},
  {"Nvdload_R11",0x250000},
  {"Nvimg",0x80000},
  {"System",0x590000},
  {"System",0x100000},
  {"APP",0x570000}, 
  {"APP",0x5a0000}, 
  {"APP_EXT_A",0x450000}, 
  {"APP_EXT_B",0x460000},
  {"Oeminfo",0xa0000},
  {"CDROMISO",0xb0000},
  {"Oeminfo",0x550000},
  {"Oeminfo",0x510000},
  {"Oeminfo",0x1a0000},
  {"WEBUI",0x560000},
  {"WEBUI",0x5b0000},
  {"Wimaxcfg",0x170000},
  {"Wimaxcrf",0x180000},
  {"Userdata",0x190000},
  {"Online",0x1b0000},
  {"Online",0x5d0000},
  {"Online",0x5e0000},
  {"Ptable_R1",0x100},
  {"Bootloader_R1",0x101},
  {"Bootrom_R1",0x102},
  {"VxWorks_R1",0x550103},
  {"Fastboot_R1",0x104},
  {"Kernel_R1",0x105},
  {"System_R1",0x107},
  {"Nvimage_R1",0x66},
  {"WEBUI_R1",0x113},
  {"APP_R1",0x109},
  {"HIFI_R11",0x280000},
  {"Modem_fw",0x1e0000},
  {"Teeos",0x290000},
  {0,0}
};

for(j=0;pcodes[j].code != 0;j++) {
  if(pcodes[j].code == id) break;
}
if (pcodes[j].code != 0) strcpy(pname,pcodes[j].name); // Name gefunden - kopieren seins in der Struktur
else sprintf(pname,"U%08x",id); // Name nicht gefunden - wir ersetzen Pseudoname Uxxxxxxxx in der stumpf formatieren
}

//*******************************************************************
// Berechnung Größe Block Kontrolle Beträge Abschnitt
//*******************************************************************
uint32_t crcsize(int n) { 
  return ptable[n].hd.hdsize-sizeof(struct pheader); 
  
}

//*******************************************************************
// Empfangen Größe Bild Abschnitt
//*******************************************************************
uint32_t psize(int n) { 
  return ptable[n].hd.psize; 
  
}

//*******************************************************
//*  Berechnung blockieren Kontrolle Summen Kopfzeile
//*******************************************************
void calc_hd_crc16(int n) { 

ptable[n].hd.crc=0;   // klar das Alte CRC-Betrag
ptable[n].hd.crc=crc16((uint8_t*)&ptable[n].hd,sizeof(struct pheader));   
}


//*******************************************************
//*  Berechnung blockieren Kontrolle Summen Abschnitt 
//*******************************************************
void calc_crc16(int n) {
  
uint32_t csize; // Größe Block Beträge in der 16-bisschen Wörter
uint16_t* csblock;  // Wegweiser von erstellt Block
uint32_t off,len;
uint32_t i;
uint32_t blocksize=ptable[n].hd.blocksize; // Größe Block, von der Betrag

// bestimmen Größe und erstellen Block
csize=psize(n)/blocksize;
if (psize(n)%blocksize != 0) csize++; // Das wenn Größe Bild nicht Charge blocksize
csblock=(uint16_t*)malloc(csize*2);

// Zyklus berechnen Beträge
for (i=0;i<csize;i++) {
 off=i*blocksize; // Vorurteil zu aktuell Block 
 len=blocksize;
 if ((ptable[n].hd.psize-off)<blocksize) len=ptable[n].hd.psize-off; // für die das letzte unvollständig Block 
 csblock[i]=crc16(ptable[n].pimage+off,len);
} 
// eingeben Parameter in der Beschriftung
if (ptable[n].csumblock != 0) free(ptable[n].csumblock); // zerstören das Alte Block, wenn er war
ptable[n].csumblock=csblock;
ptable[n].hd.hdsize=csize*2+sizeof(struct pheader);
// neu berechnen CRC Kopfzeile
calc_hd_crc16(n);
  
}


//*******************************************************************
//* Extraktion Abschnitt von der Datei und Zusatz seins in der Diagramm Abschnitte
//*
//  in - Eingabe Datei Firmware
//  Position in der Datei entspricht oben Kopfzeile Abschnitt
//*******************************************************************
void extract(FILE* in)  {

uint16_t hcrc,crc;
uint16_t* crcblock;
uint32_t crcblocksize;
uint8_t* zbuf;
long int zlen;
int res;

ptable[npart].zflag=0; 
// lesen Beschriftung in der Struktur
ptable[npart].offset=ftell(in);
fread(&ptable[npart].hd,1,sizeof(struct pheader),in); // Beschriftung
//  Auf der Suche nach symbolisch Name Abschnitt auf dem Tabelle 
find_pname(ptable[npart].hd.code,ptable[npart].pname);

// herunterladen Block Kontrolle Beträge
ptable[npart].csumblock=0;  // Tschüss Block nicht erstellt
crcblock=(uint16_t*)malloc(crcsize(npart)); // hervorheben vorübergehend Speicher unter dem herunterladbar Block
crcblocksize=crcsize(npart);
fread(crcblock,1,crcblocksize,in);

// herunterladen Bild Abschnitt
ptable[npart].pimage=(uint8_t*)malloc(psize(npart));
fread(ptable[npart].pimage,1,psize(npart),in);

// überprüfen CRC Kopfzeile
hcrc=ptable[npart].hd.crc;
ptable[npart].hd.crc=0;  // alt CRC in der рассчете nicht wird berücksichtigt
crc=crc16((uint8_t*)&ptable[npart].hd,sizeof(struct pheader));
if (crc != hcrc) {
    printf("\n! Abschnitt %s (%02x) - Fehler Kontrolle Summen Kopfzeile",ptable[npart].pname,ptable[npart].hd.code>>16);
    errflag=1;
}  
ptable[npart].hd.crc=crc;  // wiederherstellen CRC

// berechnen und überprüfen CRC Abschnitt
calc_crc16(npart);
if (crcblocksize != crcsize(npart)) {
    printf("\n! Abschnitt %s (%02x) - falsch Größe Block Kontrolle Beträge",ptable[npart].pname,ptable[npart].hd.code>>16);
    errflag=1;
}    
  
else if (memcmp(crcblock,ptable[npart].csumblock,crcblocksize) != 0) {
    printf("\n! Abschnitt %s (%02x) - falsch blockieren Kontrolle Betrag",ptable[npart].pname,ptable[npart].hd.code>>16);
    errflag=1;
}  
  
free(crcblock);


ptable[npart].ztype=' ';
// Definition zlib-Komprimierung


if ((*(uint16_t*)ptable[npart].pimage) == 0xda78) {
  ptable[npart].zflag=ptable[npart].hd.psize;  // behalten verdichtet Größe 
  zlen=52428800;
  zbuf=malloc(zlen);  // Puffer in der 50M
  // auspacken Bild Abschnitt
  res=uncompress (zbuf, &zlen, ptable[npart].pimage, ptable[npart].hd.psize);
  if (res != Z_OK) {
    printf("\n! Fehler Auspacken Abschnitt %s (%02x)\n",ptable[npart].pname,ptable[npart].hd.code>>16);
    errflag=1;
  }
  // erstellen neu Puffer Bild Abschnitt und kopieren in der ihm raubgierig Daten
  free(ptable[npart].pimage);
  ptable[npart].pimage=malloc(zlen);
  memcpy(ptable[npart].pimage,zbuf,zlen);
  ptable[npart].hd.psize=zlen;
  free(zbuf);
  // wir ordnen um Kontrolle Summen
  calc_crc16(npart);
  ptable[npart].hd.crc=crc16((uint8_t*)&ptable[npart].hd,sizeof(struct pheader));
  ptable[npart].ztype='Z';
}

// Definition lzma-Komprimierung

if ((ptable[npart].pimage[0] == 0x5d) && (*(uint64_t*)(ptable[npart].pimage+5) == 0xffffffffffffffff)) {
  ptable[npart].zflag=ptable[npart].hd.psize;  // behalten verdichtet Größe 
  zlen=52428800;
  zbuf=malloc(zlen);  // Puffer in der 50M
  // auspacken Bild Abschnitt
  zlen=lzma_decode(ptable[npart].pimage, ptable[npart].hd.psize, zbuf);
  if (zlen>52428800) {
    printf("\n Übertroffen Größe Puffer\n");
    exit(1);
  }  
  if (res == -1) {
    printf("\n! Fehler Auspacken Abschnitt %s (%02x)\n",ptable[npart].pname,ptable[npart].hd.code>>16);
    errflag=1;
  }
  // erstellen neu Puffer Bild Abschnitt und kopieren in der ihm raubgierig Daten
  free(ptable[npart].pimage);
  ptable[npart].pimage=malloc(zlen);
  memcpy(ptable[npart].pimage,zbuf,zlen);
  ptable[npart].hd.psize=zlen;
  free(zbuf);
  // wir ordnen um Kontrolle Summen
  calc_crc16(npart);
  ptable[npart].hd.crc=crc16((uint8_t*)&ptable[npart].hd,sizeof(struct pheader));
  ptable[npart].ztype='L';
}
  
  
// fördern Zähler Abschnitte
npart++;

// wir fahren weg, wenn brauche, vorwärts von Grenze die Worte
res=ftell(in);
if ((res&3) != 0) fseek(in,(res+4)&(~3),SEEK_SET);
}


//*******************************************************
//*  Suche Abschnitte in der Datei Firmware
//* 
//* kehrt zurück Anzahl von gefunden Abschnitte
//*******************************************************
int findparts(FILE* in) {

// Puffer das Präfix BIN-Datei
uint8_t prefix[0x5c];
int32_t signsize;
int32_t hd_dload_id;

// Markierung start Kopfzeile Abschnitt	      
const unsigned int dpattern=0xa55aaa55;
unsigned int i;


// Suche start Gespräche Abschnitte in der Datei
while (fread(&i,1,4,in) == 4) {
  if (i == dpattern) break;
}
if (feof(in)) {
  printf("\n In der Datei nicht gefunden Abschnitte - Datei nicht enthält Bild Firmware\n");
  exit(0);
}  

// aktuell Position in der Datei sollte zu sein nicht näher 0x60 aus start - Größe Kopfzeile insgesamt Datei
if (ftell(in)<0x60) {
    printf("\n Kopfzeile Datei hat a falsch Größe\n");
    exit(0);
}    
fseek(in,-0x60,SEEK_CUR); // wir fahren weg von Zuhause BIN-Datei

// herausnehmen Präfix
fread(prefix,0x5c,1,in);
hd_dload_id=*((uint32_t*)&prefix[0]);
// wenn gewaltsam dload_id nicht installiert - wählen Sie seins von der Kopfzeile
if (dload_id == -1) dload_id=hd_dload_id;
if (dload_id > 0xf) {
  printf("\n Ungültig Code eingeben Firmware (dload_id) in der Titel - %x",dload_id);
  exit(0);
}  
printf("\n Code Datei Firmware: %x (%s)\n",hd_dload_id,fw_description(hd_dload_id));

// Suche übrig Abschnitte

do {
  printf("\r Suche Abschnitt # %i",npart); fflush(stdout);	
  if (fread(&i,1,4,in) != 4) break; // das Ende Datei
  if (i != dpattern) break;         // Muster nicht gefunden - das Ende Gespräche Abschnitte
  fseek(in,-4,SEEK_CUR);            // wir fahren weg zurück, von Zuhause Kopfzeile
  extract(in);                      // extrahieren Abschnitt
} while(1);
printf("\r                                 \r");

// auf der Suche nach digital Unterschrift
signsize=serach_sign();
if (signsize == -1) printf("\n Digital Unterschrift: nicht gefunden");
else {
  printf("\n Digital Unterschrift: %i Bytes",signsize);
  printf("\n Ja öffnen Schlüssel: %s",signver_hash);
}
if (((signsize == -1) && (dload_id>7)) ||
    ((signsize != -1) && (dload_id<8))) 
    printf("\n ! ACHTUNG: Verfügbarkeit digital Signaturen nicht entspricht Code eingeben Firmware: %02x",dload_id);


return npart;
}


//*******************************************************
//* Suche Abschnitte in der Multi-Datei Modus
//*******************************************************
void findfiles (char* fdir) {

char filename[200];  
FILE* in;
  
printf("\n Suche Dateien-Bilder Abschnitte...\n\n ##   Größe      ID        Name          Datei\n-----------------------------------------------------------------\n");

for (npart=0;npart<30;npart++) {
    if (find_file(npart, fdir, filename, &ptable[npart].hd.code, &ptable[npart].hd.psize) == 0) break; // das Ende Suchmaschine - Abschnitt mit dem so ein ID nicht habe gefunden
    // wir bekommen symbolisch Name Abschnitt
    find_pname(ptable[npart].hd.code,ptable[npart].pname);
    printf("\n %02i  %8i  %08x  %-14.14s  %s",npart,ptable[npart].hd.psize,ptable[npart].hd.code,ptable[npart].pname,filename);fflush(stdout);
    
    // verteilen Speicher unter dem Bild Abschnitt
    ptable[npart].pimage=malloc(ptable[npart].hd.psize);
    if (ptable[npart].pimage == 0) {
      printf("\n! Fehler Zuteilung von Speicher, Abschnitt #%i, Größe = %i Bytes\n",npart,ptable[npart].hd.psize);
      exit(0);
    }
    
    // lesen Bild in der Puffer
    in=fopen(filename,"r");
    if (in == 0) {
      printf("\n Fehler Entdeckungen Datei %s",filename);
      return;
    } 
    fread(ptable[npart].pimage,ptable[npart].hd.psize,1,in);
    fclose(in);
      
}
if (npart == 0) {
 printf("\n! Nicht gefunden uns eins Datei mit dem Art und Weise Abschnitt in der Verzeichnis %s",fdir);
 exit(0);
} 
}

