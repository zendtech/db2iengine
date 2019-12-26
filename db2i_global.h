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


#ifndef DB2I_GLOBAL_H
#define DB2I_GLOBAL_H

#define MYSQL_SERVER 1
#include <my_global.h>
#if defined( __GNUC__ )
    long double align __attribute__((aligned(16))); /* force gcc align quadword */
#else
    long double		align;	/* Force xlc quadword alignment
				   (with -qldbl128 -qalign=natural) */
#endif
// #include <memory.h>
// #include <pthread/bits/stdc++.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
// #include <macros.h>
#include <my_sys.h>
#include "sql_priv.h"
#include "sql_show.h"
#include "sql_table.h"
#include "sql_class.h"

#ifdef __cplusplus
#define EXTERN  extern "C"
#else
#define EXTERN  extern
#endif

/*
 * gcc anonymous min, max appears to link incorrect.
 * Explicit macro will be used.
 */

#define min(a,b)         ((long long)(a)>(long long)(b) ? (b) : (a))
#define max(a,b)         ((long long)(a)<(long long)(b) ? (b) : (a))

#define db2i_beint16_from_ptr2leint16(V,M) \
  do { int16 def_temp;\
       ((uchar*) &def_temp)[0]=(M)[1];\
       ((uchar*) &def_temp)[1]=(M)[0];\
       (V)=def_temp; } while(0)

#define db2i_beint32_from_ptr2leuint16(V,U) \
  do { int32 def_temp; \
       int32 I = (int32)(*(uint16*)(U));\
       char * M = (char *)&I;\
       ((uchar*) &def_temp)[0]=(M)[3];\
       ((uchar*) &def_temp)[1]=(M)[2];\
       ((uchar*) &def_temp)[2]=(M)[1];\
       ((uchar*) &def_temp)[3]=(M)[0];\
       (V)=def_temp; } while(0)

#define db2i_beint32_from_ptr2leint32(V,M) \
  do { int32 def_temp;\
       ((uchar*) &def_temp)[0]=(M)[3];\
       ((uchar*) &def_temp)[1]=(M)[2];\
       ((uchar*) &def_temp)[2]=(M)[1];\
       ((uchar*) &def_temp)[3]=(M)[0];\
       (V)=def_temp; } while(0)

#define db2i_beint32_from_ptr2leuint24(V,M) \
  do { int32 def_temp; \
       ((uchar*) &def_temp)[0]=(char)0x00;\
       ((uchar*) &def_temp)[1]=(M)[2];\
       ((uchar*) &def_temp)[2]=(M)[1];\
       ((uchar*) &def_temp)[3]=(M)[0];\
       (V)=def_temp; } while(0)

#define db2i_beint64_from_ptr2leuint32(V,U) \
  do { int64 def_temp; \
       int64 I = (int64)(*(uint32*)(U));\
       char * M = (char *)&I;\
       ((uchar*) &def_temp)[0]=(M)[7];\
       ((uchar*) &def_temp)[1]=(M)[6];\
       ((uchar*) &def_temp)[2]=(M)[5];\
       ((uchar*) &def_temp)[3]=(M)[4];\
       ((uchar*) &def_temp)[4]=(M)[3];\
       ((uchar*) &def_temp)[5]=(M)[2];\
       ((uchar*) &def_temp)[6]=(M)[1];\
       ((uchar*) &def_temp)[7]=(M)[0];\
       (V) = def_temp; } while(0)

#define db2i_beint64_from_ptr2leint64(V,M) \
  do { int64 def_temp;\
       ((uchar*) &def_temp)[0]=(M)[7];\
       ((uchar*) &def_temp)[1]=(M)[6];\
       ((uchar*) &def_temp)[2]=(M)[5];\
       ((uchar*) &def_temp)[3]=(M)[4];\
       ((uchar*) &def_temp)[4]=(M)[3];\
       ((uchar*) &def_temp)[5]=(M)[2];\
       ((uchar*) &def_temp)[6]=(M)[1];\
       ((uchar*) &def_temp)[7]=(M)[0];\
       (V) = def_temp; } while(0)

#define db2i_befloat4_from_ptr2lefloat4(V,M) \
  do { float def_temp;\
       ((uchar*) &def_temp)[0]=(M)[3];\
       ((uchar*) &def_temp)[1]=(M)[2];\
       ((uchar*) &def_temp)[2]=(M)[1];\
       ((uchar*) &def_temp)[3]=(M)[0];\
       (V)=def_temp; } while(0)

#define db2i_befloat8_from_ptr2lefloat8(V,M) \
  do { double def_temp;\
       ((uchar*) &def_temp)[0]=(M)[7];\
       ((uchar*) &def_temp)[1]=(M)[6];\
       ((uchar*) &def_temp)[2]=(M)[5];\
       ((uchar*) &def_temp)[3]=(M)[4];\
       ((uchar*) &def_temp)[4]=(M)[3];\
       ((uchar*) &def_temp)[5]=(M)[2];\
       ((uchar*) &def_temp)[6]=(M)[1];\
       ((uchar*) &def_temp)[7]=(M)[0];\
       (V) = def_temp; } while(0)

const uint MAX_DB2_KEY_PARTS=120;
const int MAX_DB2_V5R4_LIBNAME_LENGTH = 10;
const int MAX_DB2_V6R1_LIBNAME_LENGTH = 30;
const int MAX_DB2_SCHEMANAME_LENGTH=258; 
const int MAX_DB2_FILENAME_LENGTH=258; 
const int MAX_DB2_COLNAME_LENGTH=128;
const int MAX_DB2_SAVEPOINTNAME_LENGTH=128;
const int MAX_DB2_QUALIFIEDNAME_LENGTH=MAX_DB2_V6R1_LIBNAME_LENGTH + 1 + MAX_DB2_FILENAME_LENGTH;
const uint32 MAX_CHAR_LENGTH = 32765;
const uint32 MAX_VARCHAR_LENGTH = 32739;
const uint32 MAX_DEC_PRECISION = 63;
const uint32 MAX_BLOB_LENGTH = 2147483646;
const uint32 MAX_BINARY_LENGTH = MAX_CHAR_LENGTH;
const uint32 MAX_VARBINARY_LENGTH = MAX_VARCHAR_LENGTH;
const uint32 MAX_FULL_ALLOCATE_BLOB_LENGTH = 65536;
const uint32 MAX_FOREIGN_LEN = 64000;
EXTERN const char* DB2I_TEMP_TABLE_SCHEMA;
const char DB2I_ADDL_INDEX_NAME_DELIMITER[5] = {'_','_','_','_','_'};
const char DB2I_DEFAULT_INDEX_NAME_DELIMITER[3] = {'_','_','_'};
const int DB2I_INDEX_NAME_LENGTH_TO_PRESERVE = 110;

enum enum_DB2I_INDEX_TYPE
{
  typeNone = 0,
  typeDefault = 'D',
  typeHex = 'H',
  typeAscii = 'A'
};

typedef uint64_t ILEMemHandle;

struct OSVersion
{
  uint8 v;
  uint8 r;
};
extern OSVersion osVersion;

EXTERN void* roundToQuadWordBdy(void* ptr);

EXTERN void* malloc_aligned(size_t size);

EXTERN void free_aligned(void* p);

#endif
