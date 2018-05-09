

// Struktur Beschreibung Kopfzeile Abschnitt
#ifndef WIN32
struct __attribute__ ((__packed__)) pheader {
#else
#pragma pack(push,1)
struct pheader {
#endif
 int32_t magic;    //   0xa55aaa55
 uint32_t hdsize;   // Größe Kopfzeile
 uint32_t hdversion;
 uint8_t unlock[8];
 uint32_t code;     // Geben Sie ein Abschnitt
 uint32_t psize;    // Raz Ränder von Daten
 uint8_t date[16];
 uint8_t time[16];  // Datum-die Zeit Montage Firmware
 uint8_t version[32];   // Version Prhoivki
 uint16_t crc;   // CRC Kopfzeile
 uint32_t blocksize;  // Größe Block CRC Bild Firmware
}; 
#ifdef WIN32
#pragma pack(pop)
#endif


// Struktur Beschreibung Tabellen Abschnitte

struct ptb_t{
  unsigned char pname[20];    // alphabetisch Name Abschnitt
  struct pheader hd;  // Bild Kopfzeile
  uint16_t* csumblock; // Block Kontrolle Beträge
  uint8_t* pimage;   // Bild Abschnitt
  uint32_t offset;   // Vorurteil in der Datei zu start Abschnitt
  uint32_t zflag;     // Attribut prägnant Abschnitt  
  uint8_t ztype;    // Geben Sie ein Komprimierung
};

//******************************************************
//*  Extern Arrays für die Lagerung Tabellen Abschnitte
//******************************************************
extern struct ptb_t ptable[];
extern int npart; // Anzahl von Abschnitte in der Tabelle

extern uint32_t errflag;

int findparts(FILE* in);
void  find_pname(unsigned int id,unsigned char* pname);
void findfiles (char* fdir);
uint32_t psize(int n);

extern int dload_id;
