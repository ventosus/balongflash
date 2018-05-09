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
#include "signver.h"
#include "zlib.h"

// Flagge Fehler Strukturen Datei
unsigned int errflag=0;

// Flagge digital Signaturen
int gflag=0;
// Flagge eingeben Firmware
int dflag=0;

// Geben Sie ein Firmware von der Kopfzeile Datei
int dload_id=-1;

//***********************************************
//* Tabelle Abschnitte
//***********************************************
struct ptb_t ptable[120];
int npart=0; // Anzahl von Abschnitte in der Tabelle


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

int main(int argc, char* argv[]) {

unsigned int opt;
int res;
FILE* in;
char devname[50] = "";
unsigned int  mflag=0,eflag=0,rflag=0,sflag=0,nflag=0,kflag=0,fflag=0;
unsigned char fdir[40];   // Katalog für die Multi-Datei Firmware

// Analyse der Befehl Linien
while ((opt = getopt(argc, argv, "d:hp:mersng:kf")) != -1) {
  switch (opt) {
   case 'h': 
     
printf("\n Dienstprogramm ist beabsichtigt für die Firmware Modems von Chipsätze Balong V7\n\n\
%s [Schlüssel] <Name Datei für die Downloads oder Name Katalog mit dem Dateien>\n\n\
 Akzeptabel das Folgende Schlüssel:\n\n"
#ifndef WIN32
"-p <tty> - aufeinanderfolgend Hafen für die Kommunikation mit dem Bootloader (auf dem Standard /dev/ttyUSB0)\n"
#else
"-p # - Nummer konsistent der Hafen für die Kommunikation mit dem Bootloader (zum Beispiel, -p8)\n"
"  wenn Hinweis -p nicht angegeben, produziert автоопределение der Hafen\n"
#endif
"-n       - Modus Multi-Datei Firmware von der die Katalog\n\
-g#      - Installation Regime digital Signaturen\n\
  -gl - die Beschreibung Parameter\n\
  -gd - Verbot автоопределения Signaturen\n\
-m       - zurückziehen Karte Datei Firmware und fertig die Arbeit\n\
-e       - auseinander nehmen Datei Firmware von Abschnitte ohne Überschriften\n\
-s       - auseinander nehmen Datei Firmware von Abschnitte mit dem Kopfzeilen\n\
-k       - nicht neu laden Modem auf dem endend Firmware\n\
-r       - gewaltsam neu laden Modem ohne Firmware Abschnitte\n\
-f       - Blitz sogar an Verfügbarkeit Fehler CRC in der das Original Datei\n\
-d#      - Installation eingeben Firmware (DLOAD_ID, 0..7), -dl - Liste Arten von\n\
\n",argv[0]);
    return 0;

   case 'p':
    strcpy(devname,optarg);
    break;

   case 'm':
     mflag=1;
     break;
     
   case 'n':
     nflag=1;
     break;
     
   case 'f':
     fflag=1;
     break;
     
   case 'r':
     rflag=1;
     break;
     
   case 'k':
     kflag=1;
     break;
     
   case 'e':
     eflag=1;
     break;

   case 's':
     sflag=1;
     break;

   case 'g':
     gparm(optarg);
     break;
     
   case 'd':
     dparm(optarg);
     break;
     
   case '?':
   case ':':  
     return -1;
  }
}  
printf("\n Das Programm für die Firmware Geräte von Balong-Chipsätze, V3.0.%i, (c) forth32, 2015, GNU GPLv3",BUILDNO);
#ifdef WIN32
printf("\n Hafen für die Windows 32bit  (c) rust3028, 2016");
#endif
printf("\n--------------------------------------------------------------------------------------------------\n");

if (eflag&sflag) {
  printf("\n Schlüssel -s und -e unvereinbar\n");
  return -1;
}  

if (kflag&rflag) {
  printf("\n Schlüssel -k und -r unvereinbar\n");
  return -1;
}  

if (nflag&(eflag|sflag|mflag)) {
  printf("\n Schlüssel -n unvereinbar mit dem Schlüssel -s, -m und -e\n");
  return -1;
}  
  

// ------  neu starten ohne Hinweise Datei
//--------------------------------------------
if ((optind>=argc)&rflag) goto sio; 


// Eröffnung Eingang Datei
//--------------------------------------------
if (optind>=argc) {
  if (nflag)
    printf("\n - Nicht angegeben Katalog mit dem Dateien\n");
  else 
    printf("\n - Nicht angezeigt Name Datei für die Downloads, verwenden Hinweis -h für die Tipps\n");
  return -1;
}  

if (nflag) 
  // für die -n - einfach kopieren Präfix
  strncpy(fdir,argv[optind],39);
else {
  // für die einzelne Datei Operationen
in=fopen(argv[optind],"rb");
if (in == 0) {
  printf("\n Fehler Entdeckungen %s",argv[optind]);
  return -1;
}
}


// Suche Abschnitte innerhalb Datei
if (!nflag) {
  findparts(in);
  show_fw_info();
}  

// Suche Dateien Firmware in der die Verzeichnis
else findfiles(fdir);
  
//------ Modus Ausgabe Karten Datei Firmware
if (mflag) show_file_map();

// Steckdose auf dem Fehler CRC
if (!fflag && errflag) {
    printf("\n\n! Eingabe Datei enthält Fehler - schließt ab die Arbeit\n");
    return -1; 
}

//------- Modus schneiden Datei Firmware
if (eflag|sflag) {
  fwsplit(sflag);
  printf("\n");
  return 0;
}

sio:
//--------- Die Hauptsache Modus - Eintrag Firmware
//--------------------------------------------

// Anpassen SIO
open_port(devname);

// Definieren Modus der Hafen und Version dload-Protokoll

res=dloadversion();
if (res == -1) return -2;
if (res == 0) {
  printf("\n Modem schon befindet sich in der HDLC-Modus");
  goto hdlc;
}

// Wenn die brauche, senden Befehl digital Signaturen
if (gflag != -1) send_signver();

// Wir treten ein in der HDLC-Modus

usleep(100000);
enter_hdlc();

// Komm rein in der HDLC
//------------------------------
hdlc:

// wir bekommen Version Protokoll und Kennung Geräte
protocol_version();
dev_ident();


printf("\n----------------------------------------------------\n");

if ((optind>=argc)&rflag) {
  // neu starten ohne Hinweise Datei
  restart_modem();
  exit(0);
}  

// Schreibe auf das Ganze Flash-Laufwerk
flash_all();
printf("\n");

port_timeout(1);

// Wir gehen von der Regime HDLC und neu starten
if (rflag || !kflag) restart_modem();
// Steckdose von der HDLC ohne startet neu
else leave_hdlc();
} 
