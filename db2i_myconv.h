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

/**
  @file
  
  @brief  A direct map optimization of iconv and related functions
          This was show to significantly reduce character conversion cost
          for short strings when compared to calling iconv system code.
*/

#ifndef DB2I_MYCONV_H
#define DB2I_MYCONV_H

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <iconv.h>
// #include "/QOpenSys/usr/include/iconv.h"
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifdef __cplusplus
#define EXTERN  extern "C"
#else
#define EXTERN  extern
#endif


/* ANSI integer data types */
#if defined(__OS400_TGTVRM__)
/* for DTAMDL(*P128), datamodel(P128): int/long/pointer=4/4/16 */
/* LLP64:4/4/8 is used for teraspace ?? */
typedef short                   int16_t;
typedef unsigned short          uint16_t;
typedef int                     int32_t;
typedef unsigned int            uint32_t;
typedef long long               int64_t;
typedef unsigned long long      uint64_t;
#elif defined(PASE)
/* PASE uses IPL32: int/long/pointer=4/4/4 + long long */
#elif defined(__64BIT__)
/* AIX 64 bit uses LP64: int/long/pointer=4/8/8 */
#endif

#define CONVERTER_ICONV         1
#define CONVERTER_DMAP          2

#define DMAP_S2S                10
#define DMAP_S2U                20
#define DMAP_D2U                30
#define DMAP_E2U                40
#define DMAP_U2S                120
#define DMAP_T2S                125
#define DMAP_U2D                130
#define DMAP_T2D                135
#define DMAP_U2E                140
#define DMAP_T2E                145
#define DMAP_S28                220
#define DMAP_D28                230
#define DMAP_E28                240
#define DMAP_82S                310
#define DMAP_82D                320
#define DMAP_82E                330
#define DMAP_U28                410
#define DMAP_82U                420
#define DMAP_T28                425
#define DMAP_U2U                510


typedef struct __dmap_rec       *dmap_t;

struct __dmap_rec
{
  uint32_t              codingSchema;
  unsigned char *       dmapS2S;        /* SBCS -> SBCS                         */
  /* The following conversion needs be followed by conversion from UCS-2/UTF-16 to UTF-8   */
  UniChar *     dmapD12U;       /* DBCS(non-EUC) -> UCS-2/UTF-16        */
  UniChar *     dmapD22U;       /* DBCS(non-EUC) -> UCS-2/UTF-16        */
  UniChar *     dmapE02U;       /* EUC/SS0 -> UCS-2/UTF-16              */
  UniChar *     dmapE12U;       /* EUC/SS1 -> UCS-2/UTF-16              */
  UniChar *     dmapE22U;       /* EUC/0x8E + SS2 -> UCS-2/UTF-16       */
  UniChar *     dmapE32U;       /* EUC/0x8F + SS3 -> UCS-2/UTF-16       */
  uchar *       dmapU2D;        /* UCS-2 -> DBCS                        */
  uchar *       dmapU2S;        /* UCS-2 -> EUC SS0                     */
  uchar *       dmapU2M2;       /* UCS-2 -> EUC SS1                     */
  uchar *       dmapU2M3;       /* UCS-2 -> EUC SS2/SS3                 */
  /* All of these pointers/tables are not used at the same time.        
   * You may be able save some space if you consolidate them.
   */
  uchar *       dmapS28;        /* SBCS -> UTF-8                        */
  uchar *       dmapD28;        /* DBCS -> UTF-8                        */
};

typedef	struct __myconv_rec	*myconv_t;
struct __myconv_rec
{
  uint32_t      converterType;
  uint32_t      index;          /* for close */
  union {
    iconv_t     cnv_iconv;
    dmap_t      cnv_dmap;
  };
  int32_t       allocatedSize;
  int32_t       fromCcsid;
  int32_t       toCcsid;
  UniChar       subD;           /* DBCS substitution char                       */
  char          subS;           /* SBCS substitution char                       */
  UniChar       srcSubD;        /* DBCS substitution char of src codepage       */
  char          srcSubS;        /* SBCS substitution char of src codepage       */
  char          from    [41+1]; /* codepage name is up to 41 bytes              */
  char          to      [41+1]; /* codepage name is up to 41 bytes              */
#ifdef __64BIT__
  char          reserved[10];   /* align 128 */
#else
  char          reserved[14];   /* align 128 */
#endif
};




EXTERN	myconv_t        myconv_open(const char*, const char*, int32_t);
EXTERN	int             myconv_close(myconv_t);

EXTERN  size_t	        myconv_iconv(myconv_t   cd ,
                                     char**     inBuf,
                                     size_t*    inBytesLeft,
                                     char**     outBuf,
                                     size_t*    outBytesLeft,
                                     size_t*    numSub);

EXTERN  size_t	        myconv_dmap(myconv_t    cd,
                                    char**      inBuf,
                                    size_t*     inBytesLeft,
                                    char**      outBuf,
                                    size_t*     outBytesLeft,
                                    size_t*     numSub);


#define myconv(a,b,c,d,e,f) \
(((a)->converterType == CONVERTER_ICONV)? myconv_iconv((a),(b),(c),(d),(e),(f)): (((a)->converterType == CONVERTER_DMAP)? myconv_dmap((a),(b),(c),(d),(e),(f)): -1))


#define converterName(a) \
(((a) == CONVERTER_ICONV)? "iconv": ((a) == CONVERTER_DMAP)? "dmap": "?????")
#endif

EXTERN  void initMyconv();
EXTERN  void cleanupMyconv();

