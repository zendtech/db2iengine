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


#include "db2i_collationSupport.h"
#include "db2i_errors.h"

struct CollationMapEntry {
  const char* collation_name;
  const char* db2_sort_sequence;
};

static CollationMapEntry collation_map[] = {
  { "ascii_general", "QALA101F4"},
  { "ascii", "QBLA101F4" },
  { "big5_chinese", "QACHT04B0" },
  { "big5", "QBCHT04B0" },
  { "cp1250_croatian", "QALA20481" },
  { "cp1250_general",  "QCLA20481" },
  { "cp1250_polish", "QDLA20481" },
  { "cp1250", "QELA20481" },
  { "cp1251_bulgarian", "QACYR0401" },
  { "cp1251_general", "QBCYR0401" },
  { "cp1251", "QCCYR0401" },
  { "cp1256_general", "QAARA01A4" },
  { "cp1256", "QBARA01A4" },
  { "cp850_general", "QCLA101F4" },
  { "cp850", "QDLA101F4" },
  { "cp852_general", "QALA20366" },
  { "cp852", "QBLA20366" },
  { "cp932_japanese", "QAJPN04B0" },
  { "cp932", "QBJPN04B0" },
  { "euckr_korean", "QAKOR04B0" },
  { "euckr", "QBKOR04B0" },
  { "gb2312_chinese", "QACHS04B0" },
  { "gb2312", "QBCHS04B0" },
  { "gbk_chinese", "QCCHS04B0" },
  { "gbk", "QDCHS04B0" },
  { "greek_general", "QAELL036B" },
  { "greek", "QBELL036B" },
  { "hebrew_general", "QAHEB01A8" },
  { "hebrew", "QBHEB01A8" },
  { "latin1_danish", "QALA1047C" },
  { "latin1_general", "QBLA1047C" },
  { "latin1_german1", "QCLA1047C" },
  { "latin1_spanish", "QDLA1047C" },
  { "latin1_swedish", "QELA1047C" },
  { "latin1", "QFLA1047C" },
  { "latin2_croatian", "QCLA20366" },
  { "latin2_general", "QELA20366" },
  { "latin2_hungarian", "QFLA20366" },
  { "latin2", "QGLA20366" },
  { "latin5_turkish", "QATRK0402" },
  { "latin5", "QBTRK0402" },
  { "macce_general", "QHLA20366" },
  { "macce", "QILA20366" },
  { "sjis_japanese", "QCJPN04B0" },
  { "sjis", "QDJPN04B0" },
  { "tis620_thai", "QATHA0346" },
  { "tis620", "QBTHA0346" },
  { "ucs2_czech", "ACS_CZ" },
  { "ucs2_danish", "ADA_DK" },
  { "ucs2_esperanto", "AEO" },
  { "ucs2_estonian", "AET" },
  { "ucs2_general", "QAUCS04B0" },
  { "ucs2_hungarian", "AHU" },
  { "ucs2_icelandic", "AIS" },
  { "ucs2_latvian", "ALV" },
  { "ucs2_lithuanian", "ALT" },
  { "ucs2_persian", "AFA" },
  { "ucs2_polish", "APL" },
  { "ucs2_romanian", "ARO" },
  { "ucs2_slovak", "ASK" },
  { "ucs2_slovenian", "ASL" },
  { "ucs2_spanish", "AES" },
  { "ucs2_swedish", "ASW" },
  { "ucs2_turkish",  "ATR" },
  { "ucs2_unicode", "AEN" },
  { "ucs2", "*HEX" },
  { "ujis_japanese", "QEJPN04B0" },
  { "ujis", "QFJPN04B0" },
  { "utf8_czech", "ACS_CZ" },
  { "utf8_danish", "ADA_DK" },
  { "utf8_esperanto", "AEO" },
  { "utf8_estonian", "AET" },
  { "utf8_general", "QAUCS04B0" },
  { "utf8_hungarian", "AHU" },
  { "utf8_icelandic", "AIS" },
  { "utf8_latvian", "ALV" },
  { "utf8_lithuanian", "ALT" },
  { "utf8_persian", "AFA" },
  { "utf8_polish", "APL" },
  { "utf8_romanian", "ARO" },
  { "utf8_slovak", "ASK" },
  { "utf8_slovenian", "ASL" },
  { "utf8_spanish", "AES" },
  { "utf8_swedish", "ASW" },
  { "utf8_turkish", "ATR" },
  { "utf8_unicode", "AEN" },
  { "utf8", "*HEX" },
  { "utf8mb4_czech", "ACS_CZ" },
  { "utf8mb4_danish", "ADA_DK" },
  { "utf8mb4_esperanto", "AEO" },
  { "utf8mb4_estonian", "AET" },
  { "utf8mb4_general", "QAUCS04B0" },
  { "utf8mb4_hungarian", "AHU" },
  { "utf8mb4_icelandic", "AIS" },
  { "utf8mb4_latvian", "ALV" },
  { "utf8mb4_lithuanian", "ALT" },
  { "utf8mb4_persian", "AFA" },
  { "utf8mb4_polish", "APL" },
  { "utf8mb4_romanian", "ARO" },
  { "utf8mb4_slovak", "ASK" },
  { "utf8mb4_slovenian", "ASL" },
  { "utf8mb4_spanish", "AES" },
  { "utf8mb4_swedish", "ASW" },
  { "utf8mb4_turkish", "ATR" },
  { "utf8mb4_unicode", "AEN" },
  { "utf8mb4", "*HEX" }
};


/**
  Get the IBM i sort sequence that corresponds to the given MySQL collation.
  
  @param fieldCharSet  The collated character set
  @param[out] rtnSortSequence  The corresponding sort sequence
    
  @return  0 if successful. Failure otherwise
*/
static int32 getAssociatedSortSequence(const CHARSET_INFO *fieldCharSet, const char** rtnSortSequence)
{
  DBUG_ENTER("ha_ibmdb2i::getAssociatedSortSequence");

  if (strcmp(fieldCharSet->csname,"binary") != 0)
  {
    size_t collationSearchLen = strlen(fieldCharSet->name);
    if (fieldCharSet->state & MY_CS_BINSORT)
      collationSearchLen -= strlen("_bin");
    else
      collationSearchLen -= strlen("_ci");

    for (auto& entry : collation_map)
    {
      if ((strlen(entry.collation_name) == collationSearchLen) && 
          (memcmp(entry.collation_name, fieldCharSet->name, collationSearchLen) == 0)) {
          *rtnSortSequence = entry.db2_sort_sequence;
          DBUG_RETURN(0);
      }
    }
    getErrTxt(DB2I_ERR_SRTSEQ);
    DBUG_RETURN(DB2I_ERR_SRTSEQ);
  }
}


/**
  Update sort sequence information for a key.
  
  This function accumulates information about a key as it is called for each
  field composing the key. The caller should invoke the function for each field
  and (with the exception of the charset parm) preserve the values for the 
  parms across invocations, until a particular key has been evaluated. Once
  the last field in the key has been evaluated, the fileSortSequence and 
  fileSortSequenceLibrary parms will contain the correct information for 
  creating the corresponding DB2 key.
  
  @param charset  The character set under consideration
  @param[in, out] fileSortSequenceType  The type of the current key's sort seq
  @param[in, out] fileSortSequence  The IBM i identifier for the DB2 sort sequence
                                    that corresponds 
    
  @return  0 if successful. Failure otherwise
*/
int32 updateAssociatedSortSequence(const CHARSET_INFO* charset, 
                                   char* fileSortSequenceType, 
                                   char* fileSortSequence, 
                                   char* fileSortSequenceLibrary)
{
  DBUG_ENTER("ha_ibmdb2i::updateAssociatedSortSequence");
  DBUG_ASSERT(charset);
  if (strcmp(charset->csname,"binary") != 0)
  {
    char newSortSequence[11] = "";
    char newSortSequenceType = ' ';
    const char* foundSortSequence;
    int rc = getAssociatedSortSequence(charset, &foundSortSequence);
    if (rc) DBUG_RETURN (rc);
    switch(foundSortSequence[0])
    {
      case '*': // Binary
        strcat(newSortSequence,foundSortSequence);
        newSortSequenceType = 'B';
        break;
      case 'Q': // Non-ICU sort sequence
        strcat(newSortSequence,foundSortSequence);
        if ((charset->state & MY_CS_BINSORT) != 0)
        {
          strcat(newSortSequence,"U"); 
        }
        else if ((charset->state & MY_CS_CSSORT) != 0)
        {
          strcat(newSortSequence,"U"); 
        }
        else
        {
          strcat(newSortSequence,"S"); 
        }
        newSortSequenceType = 'N';
        break;
      default: // ICU sort sequence
	{
          if ((charset->state & MY_CS_CSSORT) == 0)
          {
            if (osVersion.v >= 6)
              strcat(newSortSequence,"I34"); // ICU 3.4
            else 
              strcat(newSortSequence,"I26"); // ICU 2.6.1
          }
          strcat(newSortSequence,foundSortSequence);
          newSortSequenceType = 'I';
        }
        break;
    }
    if (*fileSortSequenceType == ' ') // If no sort sequence has been set yet
    {
      // Set associated sort sequence
      strcpy(fileSortSequence,newSortSequence);
      strcpy(fileSortSequenceLibrary,"QSYS");
      *fileSortSequenceType = newSortSequenceType;
    }
    else if (strcmp(fileSortSequence,newSortSequence) != 0)
    {
      // Only one sort sequence/collation is supported for each DB2 index.
      getErrTxt(DB2I_ERR_MIXED_COLLATIONS);
      DBUG_RETURN(DB2I_ERR_MIXED_COLLATIONS);
    }
  }

  DBUG_RETURN(0);
}
