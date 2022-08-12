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
/*
code changes history
$$$ Owner     mm/dd/yy    Comments
$A1 LiJian    11/29/10    Make object name uppercase on system i. 

*/

#include "db2i_global.h"
#include "db2i_misc.h"
#include "string.h"
// was inline

/**
  Undelimit quote-delimited DB2 names in-place
*/
EXTERN void stripExtraQuotes(char* name, uint maxLen)
{
  char* oldName = (char*)strdup(name);
  uint i = 0;
  uint j = 0;
  do
  {
    name[j] = oldName[i];
    if (oldName[i] == '"' && oldName[i+1] == '"')
      ++i;
  } while (++j < maxLen && oldName[++i]);
  
  if (j == maxLen)
    --j;
  name[j] = 0;
  free(oldName);
}

/**
  Convert a MySQL identifier name into a DB2 compatible format
  
  @parm input  The MySQL name
  @parm output  The DB2 name
  @parm outlen  The amount of space allocated for output
  @parm delimit  Should delimiting quotes be placed around the converted name?
  @parm delimitQuotes  Should quotes in the MySQL be delimited with additional quotes?
  
  @return FALSE if output was too small and name was truncated; TRUE otherwise
*/
EXTERN bool convertMySQLNameToDB2Name(const char* input, 
                               char* output, 
                               size_t outlen,
                               bool delimit,
                               bool delimitQuotes)
{
  uint o = 0;
  if (delimit)
    output[o++] = '"';    

  uint i = 0;
  do
  {
    /* $A1 ADC - change to force UPPER (Jian) */ 
    if (lower_case_table_names == 1 || lower_case_table_names == 2) 
		output[o] = toupper(input[i]); /* force UPPER */
    else 
		output[o] = input[i]; /* original line */
    if (delimitQuotes && input[i] == '"')
      output[++o] = '"';
  } while (++o < outlen-2 && input[++i]);
  
  if (delimit)
    output[o++] = '"';
  // o = uint
  // outlen = size_t
  output[std::min( static_cast<size_t>(o), outlen-1)] = 0; // This isn't the most user-friendly way to handle overflows,
                                  // but at least its safe.
  return (o <= outlen-1);
}

EXTERN bool isOrdinaryIdentifier(const char* s)
{
  while (*s)
  {
    if (my_isupper(system_charset_info, *s) ||
        my_isdigit(system_charset_info, *s) ||
        (*s == '_') ||
        (*s == '@') ||
        (*s == '$') ||
        (*s == '#') ||
        (*s == '"'))
      ++s;
    else
      return false;
  }
  return true;
}

/**
  Fill memory with a 16-bit word.
  
  @param p  Pointer to space to fill.
  @param v  Value to fill
  @param l  Length of space (in 16-bit words)
*/
EXTERN void memset16(void* p, uint16 v, size_t l)
{
  uint16* p2=(uint16*)p;
  while (l--)
  {
    *(p2++) = v;
  }
}

// end was inline
