//  Low-Level Verfahren der Arbeit mit dem konsistent Hafen und HDLC

#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include <io.h>
#include "printf.h"

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
static HANDLE hSerial;

static int read(int siofd, void* buf, unsigned int len)
{
    DWORD bytes_read = 0;

    ReadFile(hSerial, buf, len, &bytes_read, NULL);
 
    return bytes_read;
}

static int write(int siofd, void* buf, unsigned int len)
{
    DWORD bytes_written = 0;

    WriteFile(hSerial, buf, len, &bytes_written, NULL);

    return bytes_written;
}

//*************************************************
//*    Referenz Puffer in der Modem
//*************************************************
unsigned int send_unframed_buf(char* outcmdbuf, unsigned int outlen) {


PurgeComm(hSerial, PURGE_RXCLEAR);

write(siofd,"\x7e",1);  // senden Präfix

if (write(siofd,outcmdbuf,outlen) == 0) {   printf("\n Fehler Aufzeichnungen Teams");return 0;  }
FlushFileBuffers(hSerial);

return 1;
}

//******************************************************************************************
//* Empfang Puffer mit dem antworte von der Modem
//*
//*  masslen - Anzahl von Bytes, von der vereinheitlicht blockieren ohne Analyse Funktion das Ende 7F
//******************************************************************************************

unsigned int receive_reply(char* iobuf, int masslen) {
  
int i,iolen,escflag,bcnt,incount;
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

int i,iolen,escflag,bcnt,incount;
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

DEFINE_GUID(GUID_DEVCLASS_PORTS, 0x4D36E978, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);

static int find_port(int* port_no, char* port_name)
{
  HDEVINFO device_info_set;
  DWORD member_index = 0;
  SP_DEVINFO_DATA device_info_data;
  DWORD reg_data_type;
  char property_buffer[256];
  DWORD required_size;
  char* p;
  int result = 1;

  device_info_set = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, 0, DIGCF_PRESENT);

  if (device_info_set == INVALID_HANDLE_VALUE)
    return result;

  while (TRUE)
  {
    ZeroMemory(&device_info_data, sizeof(SP_DEVINFO_DATA));
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiEnumDeviceInfo(device_info_set, member_index, &device_info_data))
      break;

    member_index++;

    if (!SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_HARDWAREID,
             &reg_data_type, (PBYTE)property_buffer, sizeof(property_buffer), &required_size))
      continue;

    if (
        (
         strstr(_strupr(property_buffer), "VID_12D1&PID_1C05") != NULL &&
         strstr(_strupr(property_buffer), "&MI_02") != NULL
        ) ||
        (
         strstr(_strupr(property_buffer), "VID_12D1&PID_1442") != NULL &&
         strstr(_strupr(property_buffer), "&MI_00") != NULL
        )
       )
    {
      if (SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME,
              &reg_data_type, (PBYTE)property_buffer, sizeof(property_buffer), &required_size))
      {
        p = strstr(property_buffer, " (COM");
        if (p != NULL)
        {
          *port_no = atoi(p + 5);
          strcpy(port_name, property_buffer);
          result = 0;
        }
      }
      break;
    }
  }

  SetupDiDestroyDeviceInfoList(device_info_set);

  return result;
}

//***************************************************
// Eröffnung und tunen konsistent der Hafen
//***************************************************

int open_port(char* devname) {

DCB dcbSerialParams = {0};
COMMTIMEOUTS CommTimeouts;

char device[20] = "\\\\.\\COM";
int port_no;
char port_name[256];

if (*devname == '\0')
{
  printf("\n\nSuche Nähen der Hafen...\n");
  
  if (find_port(&port_no, port_name) == 0)
  {
    sprintf(devname, "%d", port_no);
    printf("Hafen: \"%s\"\n", port_name);
  }
  else
  {
    printf("Hafen nicht erkannt!\n");
    exit(0); 
  }
    //printf("\n! - Seriell Hafen nicht gegeben\n"); 
    //exit(0); 
}

strcat(device, devname);

hSerial = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
if (hSerial == INVALID_HANDLE_VALUE)
{
   printf("\n! - Seriell Hafen COM%s nicht öffnet sich\n", devname); 
   exit(0); 
}

ZeroMemory(&dcbSerialParams, sizeof(dcbSerialParams));
dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
dcbSerialParams.BaudRate = CBR_115200;
dcbSerialParams.ByteSize = 8;
dcbSerialParams.StopBits = ONESTOPBIT;
dcbSerialParams.Parity = NOPARITY;
dcbSerialParams.fBinary = TRUE;
dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
if (!SetCommState(hSerial, &dcbSerialParams))
{
    CloseHandle(hSerial);
    printf("\n! - Fehler an Initialisierung COM-der Hafen\n"); 
    exit(0); 
    //return -1;
}

CommTimeouts.ReadIntervalTimeout = 5;
CommTimeouts.ReadTotalTimeoutConstant = 30000;
CommTimeouts.ReadTotalTimeoutMultiplier = 0;
CommTimeouts.WriteTotalTimeoutConstant = 0;
CommTimeouts.WriteTotalTimeoutMultiplier = 0;
if (!SetCommTimeouts(hSerial, &CommTimeouts))
{
    CloseHandle(hSerial);
    printf("\n! - Fehler an Initialisierung COM-der Hafen\n"); 
    exit(0); 
}

PurgeComm(hSerial, PURGE_RXCLEAR);

return 1;
}


//*************************************
// Anpassen Zeit Erwartung der Hafen
//*************************************

void port_timeout(int timeout) {
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


char fpattern[80];
char fname[_MAX_PATH];
struct _finddata_t fileinfo;
intptr_t res;
FILE* in;
unsigned int pt;

sprintf(fpattern,"%s\\%02d*", dirname, num);
res = _findfirst(fpattern, &fileinfo);
_findclose(res);
if (res == -1)
    return 0;
if ((fileinfo.attrib & _A_SUBDIR) != 0)
    return 0;
strcpy(fname, fileinfo.name);
strcpy(filename, dirname);
strcat(filename, "\\");
strcat(filename, fname);  

// 00-00000200-M3Boot.bin
//überprüfen Name Datei von Verfügbarkeit Zeichen '-'
if (fname[2] != '-' || fname[11] != '-') {
  printf("\n Falsch formatieren Name Datei - %s\n",fname);
  exit(1);
}

// überprüfen digital Feld ID Abschnitt
if (strspn(fname+3,"0123456789AaBbCcDdEeFf") != 8) {
  printf("\n Fehler in der Kennung Abschnitt - nicht numerisch Zeichen - %s\n",filename);
  exit(1);
}  
sscanf(fname+3,"%8x",id);

// Überprüfung Zugänglichkeit und Lesbarkeit Datei
in=fopen(filename,"rb");
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

fclose(in);

*size = fileinfo.size;


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
PurgeComm(hSerial, PURGE_RXCLEAR);

// Versand Teams
write(siofd,cbuf,strlen(cbuf));
Sleep(100);

// lesen Ergebnis
res=read(siofd,rbuf,200);
return res;
}
  
