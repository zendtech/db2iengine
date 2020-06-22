/*
Licensed Materials - Property of IBM
DB2 Storage Engine Enablement
Copyright IBM Corporation 2007,2008
All rights reserved

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met: 
 (a) Redistributions of source code must retain this list of conditions, the
     copyright notice in section {d} below, and the disclaimer following this
     list of conditions. 
 (b) Redistributions in binary form must reproduce this list of conditions, the
     copyright notice in section (d) below, and the disclaimer following this
     list of conditions, in the documentation and/or other materials provided
     with the distribution. 
 (c) The name of IBM may not be used to endorse or promote products derived from
     this software without specific prior written permission. 
 (d) The text of the required copyright notice is: 
       Licensed Materials - Property of IBM
       DB2 Storage Engine Enablement 
       Copyright IBM Corporation 2007,2008 
       All rights reserved

THIS SOFTWARE IS PROVIDED BY IBM CORPORATION "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL IBM CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
#include "db2i_charsetSupport.h"
#include <as400_types.h>
#include <as400_protos.h>
#include "db2i_ileBridge.h"
#include <qlgusr.h>
#include "db2i_errors.h"
#include "hash.h"

struct EncodingMapEntry {
  const char* csname;
  const char* iconv_mariadb;
  const char* iconv_db2;
  unsigned short db2_ccsid;
};

// single byte is ebcdic single byte
// multibyte is UTF-16
// Here is the list of supported mariadb charsets
// https://mariadb.com/kb/en/supported-character-sets-and-collations/
static EncodingMapEntry encoding_map[] = {

  {"utf8", "UTF-8", "UTF-8", 1208},
  {"utf8mb4", "UTF-8", "UTF-8", 1208},
  {"utf8mb3", "UTF-8", "UTF-8", 1208},
  {"utf16", "UTF-16", "UTF-16", 1200},
  {"ucs2", "UCS-2", "UCS-2", 13488},
  {"ascii", "ISO8859-1", "IBM-1148", 1148}, // (US ASCII) use ISO8859-1?
  {"latin1", "ISO8859-1", "IBM-1148", 1148}, // (cp1252 West European) AKA ISO8859-1
  {"latin2", "ISO8859-2", "IBM-1153", 1153}, // (ISO 8859-2 Central European)
  {"greek", "ISO8859-7", "IBM-4971", 4971}, // (ISO 8859-7 Greek) was 875
  {"hebrew", "ISO8859-8", "IBM-424", 424}, // (ISO 8859-8 Hebrew)
  {"latin5", "ISO8859-9", "IBM-1155", 1155}, // (ISO 8859-9 Turkish)
  {"tis620", "TIS-620", "IBM-838", 838}, // (TIS620 Thai)

  {"euckr", "EUC-KR", "UTF-16", 1200}, // (EUC-KR Korean)
  {"gbk", "GBK", "UTF-16", 1200}, // (Simplified Chinese) found GBK_ct__64 & GBK_ct but unsure what ct means
  {"gb2312", "GBK", "UTF-16", 1200}, // (Simplified Chinese) no converter found
  {"sjis", "IBM-943", "UTF-16", 1200}, // (Shift-JIS Japanese) use IBM-943 ?
  {"ujis", "EUC-JP", "UTF-16", 1200}, // (EUC-JP Japanese)
  {"big5", "big5", "UTF-16", 1200}, // (Big5 Traditional Chinese)
  {"cp1250", "IBM-1250", "IBM-1153", 1153}, // (Windows Central European)
  {"cp1251", "IBM-1251", "IBM-1154", 1154}, // (Windows Cyrillic)
  {"cp850", "IBM-850", "IBM-1148", 1148}, // (DOS West European)
  {"cp852", "IBM-852", "IBM-1153", 1153}, // (DOS Central European)
  {"cp1256", "IBM-1256", "IBM-420", 420}, // (Windows Arabic)
  {"cp932", "IBM-943", "UTF-16", 1200}, // (SJIS for Windows Japanese)
  {"macce", "IBM-1282", "IBM-1153", 1153}, // (Mac Central European)

  // There are no iconv converter tables for the following charsets if specified
  // DB2I_ERR_UNSUPP_CHARSET error is returned
  // {"armscii8", "", "", }, // (ARMSCII-8 Armenian)
  // {"cp866", "", "", }, // (DOS Russian)
  // {"cp1257", "", "", }, // (Windows Baltic)
  // {"dec8", "", "", }, // (DEC Western European)
  // {"eucjpms", "", "",}, // (UJIS for Windows Japanese)
  // {"geostd8", "", "", }, // (Georgian)
  // {"hp8", "", "", }, // (IBM-1050)
  // {"koi8r", "", "", }, // (8-bit Russian)
  // {"latin7", "", "", }, // (ISO 8859-13 Baltic)
  // {"koi8u", "", "", }, // (8 bit Ukrainian)
  // {"keybcs2", "", "", }, // (DOS Kamenicky Czech-Slovak)
  // {"macroman", "", "",}, // (Mac West European)
  // {"swe7", "", "", }, // (7bit Swedish)
  // {"binary", "", "", }, (Binary pseudo charset) not needed
  // {"utf16le", "", "", },
  // {"utf32", "", "", },
};

/*
  The following arrays define a mapping between IANA-style text descriptors and
  IBM i CCSID text descriptors. The mapping is a 1-to-1 correlation between
  corresponding array slots.
*/
#define MAX_IANASTRING 23
static const char ianaStringType[MAX_IANASTRING][10] = 
{
    {"ascii"},
    {"Big5"},    //big5
    {"cp1250"},
    {"cp1251"},
    {"cp1256"},
    {"cp850"},
    {"cp852"},
    {"cp866"},
    {"IBM943"},   //cp932
    {"EUC-KR"},   //euckr
    {"IBM1381"},  //gb2312
    {"IBM1386"},  //gbk
    {"greek"},
    {"hebrew"},
    {"latin1"},
    {"latin2"},
    {"latin5"},
    {"macce"},
    {"tis620"},
    {"Shift_JIS"}, //sjis
    {"ucs2"},
    {"EUC-JP"},    //ujis
    {"utf8"}
};
static const char ccsidType[MAX_IANASTRING][6] = 
{
    {"367"},  //ascii
    {"950"},  //big5
    {"1250"}, //cp1250
    {"1251"}, //cp1251
    {"1256"}, //cp1256
    {"850"},  //cp850
    {"852"},  //cp852
    {"866"},  //cp866
    {"943"},  //cp932
    {"970"},  //euckr
    {"1381"}, //gb2312
    {"1386"}, //gbk
    {"813"},  //greek
    {"916"},  //hebrew
    {"923"},  //latin1
    {"912"},  //latin2
    {"920"},  //latin5
    {"1282"}, //macce
    {"874"},  //tis620
    {"943"},  //sjis
    {"13488"},//ucs2
    {"5050"}, //ujis
    {"1208"}  //utf8
};

// static _ILEpointer *QlgCvtTextDescToDesc_sym;

/* We keep a cache of the mapping for text descriptions obtained via
   QlgTextDescToDesc. The following structures implement this cache. */
static HASH textDescMapHash;
static MEM_ROOT textDescMapMemroot;
static pthread_mutex_t textDescMapHashMutex;
struct TextDescMap
{
  struct HashKey
  {
    int32 inType;
    int32 outType;
    char inDesc[Qlg_MaxDescSize];
  } hashKey;  
  char outDesc[Qlg_MaxDescSize];
};

/* We keep a cache of the mapping for open iconv descriptors. The following 
   structures implement this cache. */
static HASH iconvMapHash;
static MEM_ROOT iconvMapMemroot;
static pthread_mutex_t iconvMapHashMutex;
struct IconvMap
{
  struct HashKey
  {
    uint32 direction; // These are uint32s to avoid garbage data in the key from compiler padding
    uint32 db2CCSID;
    const CHARSET_INFO* myCharset;
  } hashKey;
  iconv_t iconvDesc;
};


/**
  Initialize the static structures used by this module.
  
  This must only be called once per plugin instantiation.
  
  @return  0 if successful. Failure otherwise
*/

int32 initCharsetSupport()
{
  DBUG_ENTER("initCharsetSupport");

  pthread_mutex_init(&textDescMapHashMutex,MY_MUTEX_INIT_FAST);
  my_hash_init(&textDescMapHash, &my_charset_bin, 10, offsetof(TextDescMap, hashKey), sizeof(TextDescMap::hashKey), 0, 0, HASH_UNIQUE);

  pthread_mutex_init(&iconvMapHashMutex,MY_MUTEX_INIT_FAST);
  my_hash_init(&iconvMapHash, &my_charset_bin, 10, offsetof(IconvMap, hashKey), sizeof(IconvMap::hashKey), 0, 0, HASH_UNIQUE);
 
  DBUG_PRINT("initCharsetSupport",  ("hashes initialized"));

  init_alloc_root(&textDescMapMemroot, "ibmdb2i", 2048 + ALLOC_ROOT_MIN_BLOCK_SIZE, 0, MYF(0));
  init_alloc_root(&iconvMapMemroot, "ibmdb2i", 256 + ALLOC_ROOT_MIN_BLOCK_SIZE, 0, MYF(0));

  initMyconv();
  
  DBUG_RETURN(0);
}


/**
  Cleanup the static structures used by this module.
  
  This must only be called once per plugin instantiation and only if 
  initCharsetSupport() was successful.
*/
void doneCharsetSupport()
{
  cleanupMyconv();
    
  free_root(&textDescMapMemroot, 0);
  free_root(&iconvMapMemroot, 0);
  
  pthread_mutex_destroy(&textDescMapHashMutex);
  my_hash_free(&textDescMapHash);
  pthread_mutex_destroy(&iconvMapHashMutex);
  my_hash_free(&iconvMapHash);
}

int32 convertMyCharsetToDb2Ccsid(const CHARSET_INFO* charset_info) {
  for (auto& entry : encoding_map) {
    if(strcmp(entry.csname, charset_info->csname) == 0){
      return entry.db2_ccsid;
    }
  }
  fprintf(stderr, "ibmdb2 error charset was not found in convertMyCharsetToDb2Ccsid\n");
  getErrTxt(DB2I_ERR_UNSUPP_CHARSET, charset_info->csname);
  return -DB2I_ERR_UNSUPP_CHARSET;
}


/**
  Obtain the encoding scheme of a CCSID.
    
  @param inCcsid  An IBM i CCSID
  @param[out] outEncodingScheme  The associated encoding scheme
    
  @return  0 if successful. Failure otherwise
*/
int32 getEncodingScheme(const uint16 inCcsid, int32& outEncodingScheme)
{
  DBUG_ENTER("db2i_charsetSupport::getEncodingScheme");
  
  static bool ptrInited = FALSE;
  static char ptrSpace[sizeof(ILEpointer) + 15];
  static ILEpointer* ptrToPtr = (ILEpointer*)roundToQuadWordBdy(ptrSpace);
  int rc;
    
  if (!ptrInited)
  {  
    rc = _RSLOBJ2(ptrToPtr, RSLOBJ_TS_PGM, "QTQGESP", "QSYS");

    if (rc)
    {
      getErrTxt(DB2I_ERR_RESOLVE_OBJ,"QTQGESP","QSYS","*PGM",errno);
      DBUG_RETURN(DB2I_ERR_RESOLVE_OBJ);
    }
    ptrInited = TRUE;
  }
  
  DBUG_ASSERT(inCcsid != 0);
  
  int GESPCCSID = inCcsid;
  int GESPLen = 32;
  int GESPNbrVal = 0;
  int32 GESPES;
  int GESPCSCPL[32];
  int GESPFB[3];
  void* ILEArgv[7];
  ILEArgv[0] = &GESPCCSID;
  ILEArgv[1] = &GESPLen;
  ILEArgv[2] = &GESPNbrVal;
  ILEArgv[3] = &GESPES;
  ILEArgv[4] = &GESPCSCPL;
  ILEArgv[5] = &GESPFB;
  ILEArgv[6] = NULL;
  
  rc = _PGMCALL(ptrToPtr, (void**)&ILEArgv, 0);
  
  if (rc)
  {
     getErrTxt(DB2I_ERR_PGMCALL,"QTQGESP","QSYS",rc);
     DBUG_RETURN(DB2I_ERR_PGMCALL);
  }
  if (GESPFB[0] != 0 ||
      GESPFB[1] != 0 ||
      GESPFB[2] != 0) 
  {
    getErrTxt(DB2I_ERR_QTQGESP,GESPFB[0],GESPFB[1],GESPFB[2]);
    DBUG_RETURN(DB2I_ERR_QTQGESP);
  }
  outEncodingScheme = GESPES;
  
  DBUG_RETURN(0);
}


/**
  Get the best fit equivalent CCSID. (Wrapper for QTQGRDC API)
    
  @param inCcsid  An IBM i CCSID
  @param inEncodingScheme  The encoding scheme
  @param[out] outCcsid  The equivalent CCSID
    
  @return  0 if successful. Failure otherwise
*/
int32 getAssociatedCCSID(const uint16 inCcsid, const int inEncodingScheme, uint16* outCcsid)
{
  DBUG_ENTER("db2i_charsetSupport::getAssociatedCCSID");
  static bool ptrInited = FALSE;
  static char ptrSpace[sizeof(ILEpointer) + 15];
  static ILEpointer* ptrToPtr = (ILEpointer*)roundToQuadWordBdy(ptrSpace);
  int rc;
  
  // Override non-standard charsets
  if ((inCcsid == 923) && (inEncodingScheme == 0x1100))
  {
    *outCcsid = 1148;
    DBUG_RETURN(0);
  }
  else if ((inCcsid == 1250) && (inEncodingScheme == 0x1100))
  {
    *outCcsid = 1153;
    DBUG_RETURN(0);
  }

  if (!ptrInited)
  {  
    rc = _RSLOBJ2(ptrToPtr, RSLOBJ_TS_PGM, "QTQGRDC", "QSYS");

    if (rc)
    {
       getErrTxt(DB2I_ERR_RESOLVE_OBJ,"QTQGRDC","QSYS","*PGM",errno);
       DBUG_RETURN(DB2I_ERR_RESOLVE_OBJ);
    }
    ptrInited = TRUE;
  }

  int GRDCCCSID = inCcsid;
  int GRDCES = inEncodingScheme;
  int GRDCSel = 0;
  int GRDCAssCCSID;
  int GRDCFB[3];
  void* ILEArgv[7];
  ILEArgv[0] = &GRDCCCSID;
  ILEArgv[1] = &GRDCES;
  ILEArgv[2] = &GRDCSel;
  ILEArgv[3] = &GRDCAssCCSID;
  ILEArgv[4] = &GRDCFB;
  ILEArgv[5] = NULL;
  
  rc = _PGMCALL(ptrToPtr, (void**)&ILEArgv, 0);

  if (rc)  
  {
     getErrTxt(DB2I_ERR_PGMCALL,"QTQGRDC","QSYS",rc);
     DBUG_RETURN(DB2I_ERR_PGMCALL);
  }
  if (GRDCFB[0] != 0 ||
      GRDCFB[1] != 0 ||
      GRDCFB[2] != 0)
  {
    getErrTxt(DB2I_ERR_QTQGRDC,GRDCFB[0],GRDCFB[1],GRDCFB[2]);
    DBUG_RETURN(DB2I_ERR_QTQGRDC);
  }
 
  *outCcsid = GRDCAssCCSID;

  DBUG_RETURN(0);
}

/**
  Open an iconv conversion between a MySQL charset and the respective IBM i CCSID
    
  @param direction  The direction of the conversion
  @param cs  The MySQL character set
  @param db2CCSID  The IBM i CCSID
  @param[out] newConversion  The iconv descriptor
    
  @return  0 if successful. Failure otherwise
*/
int32 getConversion(enum_conversionDirection direction, const CHARSET_INFO* cs, uint16 db2CCSID, iconv_t& conversion)
{
  DBUG_ENTER("db2i_charsetSupport::getConversion");
  
  int32 rc;

  /* Build the hash key */  
  IconvMap::HashKey hashKey;
  hashKey.direction= direction;
  hashKey.myCharset= cs;
  hashKey.db2CCSID= db2CCSID;

  /* Look for the conversion in the cache and add it if it is not there. */
  IconvMap *mapping;
  if (!(mapping= (IconvMap *)
  	my_hash_search(&iconvMapHash,
                (const uchar*)&hashKey,
                sizeof(hashKey))))
  {
    DBUG_PRINT("getConversion", ("Hash miss for direction=%d, cs=%s, ccsid=%d", direction, cs->name, db2CCSID));

    EncodingMapEntry *entry = nullptr;

    for (auto& e : encoding_map) {
      if (strcmp(e.csname, cs->csname) == 0) {
        entry = &e;
        break;
      }
    }

    if (entry == nullptr) {
      fprintf(stderr, "ibmdb2 error entry was null charset was not found\n");
      getErrTxt(DB2I_ERR_UNSUPP_CHARSET, cs->csname);
      DBUG_RETURN(DB2I_ERR_UNSUPP_CHARSET);
    }

    const char *iconv_db2 = entry->iconv_db2;
    // IBM-12345 is the max length of temp buffer
    char temp[10];
    if (db2CCSID != entry->db2_ccsid) {
      switch(db2CCSID) {
        case 1208:
        iconv_db2 = "UTF-8";
        break;
        case 1200:
        iconv_db2 = "UTF-16";
        break;
        case 13488:
        iconv_db2 = "UCS-2";
        break;
        default:
        sprintf(temp, "IBM-%hu", db2CCSID);
        iconv_db2 = temp;
      }
    }

    iconv_t newConversion;
    /* Call iconv to open the conversion. */
    if (direction == toDB2)
    {
      newConversion = iconv_open(iconv_db2, entry->iconv_mariadb);
    }
    else
    {
      newConversion = iconv_open(entry->iconv_mariadb, iconv_db2);
    }

    if (unlikely(newConversion == (iconv_t) -1))
    {
      fprintf(stderr, "ibmdb2i error opening iconv conversion: iconv_mariadb: %s, iconv_db2: %s toDB2: %d\n",
       entry->iconv_mariadb, iconv_db2, (int)toDB2);
      getErrTxt(DB2I_ERR_UNSUPP_CHARSET, cs->csname);
      DBUG_RETURN(DB2I_ERR_UNSUPP_CHARSET);
    }

    /* Insert the new conversion into the cache. */
    mapping = (IconvMap*)alloc_root(&iconvMapMemroot, sizeof(IconvMap));
    if (!mapping)
    {
      my_error(ER_OUTOFMEMORY, MYF(0), sizeof(IconvMap));
      DBUG_RETURN( HA_ERR_OUT_OF_MEM);
    }
    memcpy(&(mapping->hashKey), &hashKey, sizeof(mapping->hashKey));
    mapping->iconvDesc = newConversion;
    pthread_mutex_lock(&iconvMapHashMutex);
    my_hash_insert(&iconvMapHash, (const uchar*)mapping);
    pthread_mutex_unlock(&iconvMapHashMutex);
  }

  conversion= mapping->iconvDesc;

  DBUG_RETURN(0);
}

/**
  Fast-path conversion from ASCII to EBCDIC for use in converting
  identifiers to be sent to the QMY APIs.
  
  @param input  ASCII data
  @param[out] ouput  EBCDIC data
  @param ilen  Size of input buffer and output buffer
*/
int convToEbcdic(const char* input, char* output, size_t ilen)
{
  static bool inited = FALSE;
  static iconv_t ic;
  
  if (ilen == 0)
    return 0;
  
  if (!inited)
  {
    ic = iconv_open( "IBM-037", "ISO8859-1" );
    inited = TRUE;
  }
  size_t substitutedChars;
  size_t olen = ilen;
  if (iconv( ic, (char**)&input, &ilen, &output, &olen, &substitutedChars ) == (size_t) -1)
    return errno;
  
  return 0;
}


/**
  Fast-path conversion from EBCDIC to ASCII for use in converting
  data received from the QMY APIs.
  
  @param input  EBCDIC data
  @param[out] ouput  ASCII data
  @param ilen  Size of input buffer and output buffer
*/
int convFromEbcdic(const char* input, char* output, size_t ilen)
{
  static bool inited = FALSE;
  static iconv_t ic;
  
  if (ilen == 0)
    return 0;

  if (!inited)
  {
    ic = iconv_open("ISO8859-1", "IBM-037");
    inited = TRUE;
  }
  
  size_t substitutedChars;
  size_t olen = ilen;
  if (iconv( ic, (char**)&input, &ilen, &output, &olen, &substitutedChars) == (size_t) -1)
    return errno;
  
  return 0;
}
