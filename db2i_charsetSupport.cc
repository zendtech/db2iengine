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

// This table contains the mappings used to convert between MySQL/MariaDB
// charsets and Db2 CCSIDs. It tries to replicate the prior code which
// used the following mapping rules:
// - map single byte charsets to the matching EBCDIC CCSID
// - map the ucs2 charset to 13488
// - map the utf8 charset to 1208
// - map any other charset to 1200
//
// Instead of going through a bunch of code to try to map charsets to
// iconv names and then iconv names to names that QlgCvtTextDescToDesc
// understands to get the CCSID that we can pass to QTQGRDC to get the
// associated CCSID... we do that work upfront and store it in this table.
//
// NOTE: Any charset which is not in this list will cause the
// DB2I_ERR_UNSUPP_CHARSET error to be returned, however the Db2 CCSID does
// not have to match since existing tables may have been mapped differently.
static EncodingMapEntry encoding_map[] = {

  {"utf8", "UTF-8", "UTF-8", 1208},
  {"utf8mb4", "UTF-8", "UTF-8", 1208},
  {"utf8mb3", "UTF-8", "UTF-8", 1208},
  {"utf16", "UTF-16", "UTF-16", 1200},
  {"ucs2", "UCS-2", "UCS-2", 13488},
  {"ascii", "ISO8859-1", "IBM-1148", 1148},
  {"latin1", "ISO8859-1", "IBM-1148", 1148},
  {"latin2", "ISO8859-2", "IBM-1153", 1153},
  {"greek", "ISO8859-7", "IBM-4971", 4971},
  {"hebrew", "ISO8859-8", "IBM-424", 424},
  {"latin5", "ISO8859-9", "IBM-1155", 1155},
  {"tis620", "TIS-620", "IBM-838", 838},

  {"euckr", "EUC-KR", "UTF-16", 1200},
  {"gbk", "GBK", "UTF-16", 1200},
  {"gb2312", "GBK", "UTF-16", 1200},
  {"sjis", "IBM-943", "UTF-16", 1200},
  {"ujis", "EUC-JP", "UTF-16", 1200},
  {"big5", "big5", "UTF-16", 1200},
  {"cp1250", "IBM-1250", "IBM-1153", 1153},
  {"cp1251", "IBM-1251", "IBM-1154", 1154},
  {"cp850", "IBM-850", "IBM-1148", 1148},
  {"cp852", "IBM-852", "IBM-1153", 1153},
  {"cp1256", "IBM-1256", "IBM-420", 420},
  {"cp932", "IBM-943", "UTF-16", 1200},
  {"macce", "IBM-1282", "IBM-1153", 1153},

  // We cannot support these character sets as there is no matching iconv
  // conversion that is supported by PASE.
  // {"armscii8", "", "", },
  // {"cp866", "", "", },
  // {"cp1257", "", "", },
  // {"dec8", "", "", },
  // {"eucjpms", "", "",},
  // {"geostd8", "", "", },
  // {"hp8", "", "", },
  // {"koi8r", "", "", },
  // {"latin7", "", "", },
  // {"koi8u", "", "", },
  // {"keybcs2", "", "", },
  // {"macroman", "", "",},
  // {"swe7", "", "", },
  // {"binary", "", "", },
  // {"utf16le", "", "", },
  // {"utf32", "", "", },
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

  pthread_mutex_init(&iconvMapHashMutex,MY_MUTEX_INIT_FAST);
  my_hash_init(&iconvMapHash, &my_charset_bin, 10, offsetof(IconvMap, hashKey), sizeof(IconvMap::hashKey), 0, 0, HASH_UNIQUE);
 
  DBUG_PRINT("initCharsetSupport",  ("hashes initialized"));

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

  free_root(&iconvMapMemroot, 0);
  pthread_mutex_destroy(&iconvMapHashMutex);
  my_hash_free(&iconvMapHash);
}

int32 convertMyCharsetToDb2Ccsid(const CHARSET_INFO* charset_info) {
  for (auto& entry : encoding_map) {
    if(strcmp(entry.csname, charset_info->csname) == 0){
      return entry.db2_ccsid;
    }
  }
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
