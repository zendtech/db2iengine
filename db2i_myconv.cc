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

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <errno.h>
#include <iconv.h>
// #include "/QOpenSys/usr/include/iconv.h"
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <as400_protos.h>
#include "db2i_myconv.h"
#include "db2i_global.h"

int32_t myconvDebug = 0;


EXTERN  int     myconvGetES(CCSID);
EXTERN  int     myconvIsEBCDIC(const char *);
EXTERN  int     myconvIsASCII(const char *);
EXTERN  int     myconvIsUnicode(const char *);   /* UTF-8, UTF-16, or UCS-2 */
EXTERN  int     myconvIsUnicode2(const char *);  /* 2 byte Unicode */
EXTERN  int     myconvIsUCS2(const char *);
EXTERN  int     myconvIsUTF16(const char *);
EXTERN  int     myconvIsUTF8(const char *);
EXTERN  int     myconvIsEUC(const char *);
EXTERN  int     myconvIsISO(const char *);
EXTERN  int     myconvIsSBCS(const char *);
EXTERN  int     myconvIsDBCS(const char *);
EXTERN  char    myconvGetSubS(const char *);
EXTERN  UniChar myconvGetSubD(const char *);

/*
static	char 	szGetTimeString[20];
static	char *	GetTimeString(time_t	now)
{
  struct tm *	tm;

  now = time(&now);
  tm = (struct tm *) localtime(&now);
  sprintf(szGetTimeString, "%04d/%02d/%02d %02d:%02d:%02d",
          tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
          tm->tm_hour,        tm->tm_min,   tm->tm_sec);

  return szGetTimeString;
}
*/

static MEM_ROOT dmapMemRoot;

EXTERN void initMyconv()
{
  init_alloc_root(&dmapMemRoot, "ibmdb2i", 0x200 + ALLOC_ROOT_MIN_BLOCK_SIZE, 0, MYF(0));
}

EXTERN void cleanupMyconv()
{
  free_root(&dmapMemRoot,0);
}


#ifdef DEBUG
/* type:	*/
#define STDOUT_WITH_TIME        -1      /*  to stdout with time	*/
#define STDERR_WITH_TIME	-2      /*  to stderr with time	*/
#define STDOUT_WO_TIME  	1       /* : to stdout		*/
#define STDERR_WO_TIME          2       /* : to stderr		*/


static void MyPrintf(long	type,
		     char *	fmt, ...)
{
  char 		StdoutFN[256];
  va_list	ap;
  char *	p;
  time_t	now;
  FILE *	fd=stderr;

  if (type < 0)
  {
    now = time(&now);
    fprintf(fd, "%s ", GetTimeString(now));
  }
  va_start(ap, fmt);
  vfprintf(fd, fmt, ap);
  va_end(ap);
}
#endif




#define MAX_CONVERTER           128

int32_t mycstoccsid(const char* pname)
{
  if (strcmp(pname, "UTF-16")==0)
    return 1200;
  else if (strcmp(pname, "big5")==0)
    return 950;
  else
    return cstoccsid(pname);
}
#define cstoccsid mycstoccsid

static  struct __myconv_rec     myconv_rec      [MAX_CONVERTER];
static  struct __dmap_rec       dmap_rec        [MAX_CONVERTER];

static int      dmap_open(const char *  to,
                          const char *  from,
                          const int32_t idx)
{
  if (myconvIsSBCS(from) && myconvIsSBCS(to)) {
    dmap_rec[idx].codingSchema = DMAP_S2S;
    if ((dmap_rec[idx].dmapS2S = (uchar *) alloc_root(&dmapMemRoot, 0x100)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                   to, from, idx, DMAP_S2S, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapS2S, 0x00, 0x100);
    myconv_rec[idx].allocatedSize=0x100;

    {
      char      dmapSrc[0x100];
      iconv_t   cd;
      int32_t   i;
      size_t    inBytesLeft=0x100;
      size_t    outBytesLeft=0x100;
      size_t    len;
      char *    inBuf=dmapSrc;
      char *    outBuf=(char *) dmap_rec[idx].dmapS2S;

      if ((cd = iconv_open(to, from)) == (iconv_t) -1) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d) failed with iconv_open(), errno = %d in %s at %d\n",
                   to, from, idx, errno, __FILE__,__LINE__);
#endif
        return -1;
      }

      inBytesLeft = 0x100;
      for (i = 0; i < (int) inBytesLeft; ++i)
        dmapSrc[i]=i;

      do {
        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
#ifdef DEBUG
          if (myconvDebug) {
            MyPrintf(STDERR_WITH_TIME,
                     "dmap_open(%s,%s,%d), CS=%d: iconv() returns %d, errno = %d in %s at %d\n",
                     to, from, idx, DMAP_S2S, len, errno, __FILE__,__LINE__);
            MyPrintf(STDERR_WITH_TIME,
                     "inBytesLeft = %d, inBuf - dmapSrc = %d\n", inBytesLeft, inBuf-dmapSrc);
            MyPrintf(STDERR_WITH_TIME,
                     "outBytesLeft = %d, outBuf - dmapS2S = %d\n", outBytesLeft, outBuf-(char *) dmap_rec[idx].dmapS2S);
          }
          if ((inBytesLeft == 86 || inBytesLeft == 64 || inBytesLeft == 1) &&
              memcmp(from, "IBM-1256", 9) == 0 &&
              memcmp(to, "IBM-420", 8) == 0) {
            /* Known problem for IBM-1256_IBM-420 */
            --inBytesLeft;
            ++inBuf;
            *outBuf=0x00;
            ++outBuf;
            --outBytesLeft;
            continue;
          } else if ((inBytesLeft == 173 || inBytesLeft == 172 ||
                      inBytesLeft == 74  || inBytesLeft == 73  ||
                      inBytesLeft == 52  || inBytesLeft == 50  ||
                      inBytesLeft == 31  || inBytesLeft == 20  ||
                      inBytesLeft == 6) &&
                     memcmp(to, "IBM-1256", 9) == 0 &&
                     memcmp(from, "IBM-420", 8) == 0) {
            /* Known problem for IBM-420_IBM-1256 */
            --inBytesLeft;
            ++inBuf;
            *outBuf=0x00;
            ++outBuf;
            --outBytesLeft;
            continue;
          } else if ((128 >= inBytesLeft) &&
                     memcmp(to, "IBM-037", 8) == 0 &&
                     memcmp(from, "IBM-367", 8) == 0) {
            /* Known problem for IBM-367_IBM-037 */
            --inBytesLeft;
            ++inBuf;
            *outBuf=0x00;
            ++outBuf;
            --outBytesLeft;
            continue;
          } else if (((1 <= inBytesLeft && inBytesLeft <= 4) || (97 <= inBytesLeft && inBytesLeft <= 128)) &&
                     memcmp(to, "IBM-838", 8) == 0 &&
                     memcmp(from, "TIS-620", 8) == 0) {
            /* Known problem for TIS-620_IBM-838 */
            --inBytesLeft;
            ++inBuf;
            *outBuf=0x00;
            ++outBuf;
            --outBytesLeft;
            continue;
          }
          iconv_close(cd);
          return -1;
#else
          /* Tolerant to undefined conversions for any converter */
          --inBytesLeft;
          ++inBuf;
          *outBuf=0x00;
          ++outBuf;
          --outBytesLeft;
          continue;
#endif
        }
      } while (inBytesLeft > 0);

      if (myconvIsISO(to))
        myconv_rec[idx].subS=0x1A;
      else if (myconvIsASCII(to))
        myconv_rec[idx].subS=0x7F;
      else if (myconvIsEBCDIC(to))
        myconv_rec[idx].subS=0x3F;

      if (myconvIsISO(from))
        myconv_rec[idx].srcSubS=0x1A;
      else if (myconvIsASCII(from))
        myconv_rec[idx].srcSubS=0x7F;
      else if (myconvIsEBCDIC(from))
        myconv_rec[idx].srcSubS=0x3F;

      iconv_close(cd);
    }
  } else if (((myconvIsSBCS(from) && myconvIsUnicode2(to)) && (dmap_rec[idx].codingSchema = DMAP_S2U)) ||
             ((myconvIsSBCS(from) && myconvIsUTF8(to)) && (dmap_rec[idx].codingSchema = DMAP_S28))) {

    /* single byte mapping */
    if ((dmap_rec[idx].dmapD12U = (UniChar *) alloc_root(&dmapMemRoot, 0x100 * 2)) == NULL) {
#ifdef DEBUG
      MyPrintf(STDERR_WITH_TIME,
               "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
               to, from, idx, DMAP_S2U, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapD12U, 0x00, 0x100 * 2);
    myconv_rec[idx].allocatedSize=0x100 * 2;

    
    {
      char      dmapSrc[2];
      iconv_t   cd;
      int32_t   i;
      size_t    inBytesLeft;
      size_t    outBytesLeft;
      size_t    len;
      char *    inBuf;
      char *    outBuf;
#ifdef support_surrogate
      if ((cd = iconv_open("UTF-16", from)) == (iconv_t) -1) {
#else
      if ((cd = iconv_open("UCS-2", from)) == (iconv_t) -1) {
#endif
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d) failed with iconv_open(), errno = %d in %s at %d\n",
                 to, from, idx, errno, __FILE__,__LINE__);
#endif
        return -1;
      }

      for (i = 0; i < 0x100; ++i) {
        dmapSrc[0]=i;
        inBuf=dmapSrc;
        inBytesLeft=1;
        outBuf=(char *) &(dmap_rec[idx].dmapD12U[i]);
        outBytesLeft=2;
        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
          if ((errno == EILSEQ || errno == EINVAL) &&
              inBytesLeft == 1 &&
              outBytesLeft == 2) {
            continue;
          } else {
#ifdef DEBUG
            if (myconvDebug) {
              MyPrintf(STDERR_WITH_TIME,
                       "dmap_open(%s,%s,%d) failed to initialize with iconv(cd,%02x,%d,%02x%02x,%d), errno = %d in %s at %d\n",
                       to, from, idx, dmapSrc[0], 1,
                       (&dmap_rec[idx].dmapD12U[i])[0],(&dmap_rec[idx].dmapD12U[i])[1], 2,
                       errno, __FILE__,__LINE__);
              MyPrintf(STDERR_WITH_TIME,
                       "inBytesLeft=%d, outBytesLeft=%d, %02x%02x\n",
                       inBytesLeft, outBytesLeft,
                       (&dmap_rec[idx].dmapD12U[i])[0],(&dmap_rec[idx].dmapD12U[i])[1]);
            }
#endif
            iconv_close(cd);
            return -1;
          }            
          dmap_rec[idx].dmapD12U[i]=0x0000;
        }
        if (dmap_rec[idx].dmapE02U[i] == 0x001A &&      /* pick the first one */
            myconv_rec[idx].srcSubS == 0x00) {
          myconv_rec[idx].srcSubS=i;
        }
      }
      iconv_close(cd);
    }
    myconv_rec[idx].subS=0x1A;
    myconv_rec[idx].subD=0xFFFD;
    

  } else if (((myconvIsUCS2(from)  && myconvIsSBCS(to)) && (dmap_rec[idx].codingSchema = DMAP_U2S)) ||
             ((myconvIsUTF16(from) && myconvIsSBCS(to)) && (dmap_rec[idx].codingSchema = DMAP_T2S)) ||
             ((myconvIsUTF8(from)  && myconvIsSBCS(to)) && (dmap_rec[idx].codingSchema = DMAP_82S))) {
    /* UTF-16 -> SBCS, the direct map a bit of waste of space,
     * binary search may be reasonable alternative
     */
    if ((dmap_rec[idx].dmapU2S = (uchar *) alloc_root(&dmapMemRoot, 0x10000 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_U2S, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapU2S, 0x00, 0x10000);
    myconv_rec[idx].allocatedSize=(0x10000 * 2);

    {
      iconv_t   cd;
      int32_t   i;

#ifdef support_surrogate
      if ((cd = iconv_open(to, "UTF-16")) == (iconv_t) -1) {
#else
      if ((cd = iconv_open(to, "UCS-2")) == (iconv_t) -1) {
#endif
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d) failed with iconv_open(), errno = %d in %s at %d\n",
                 to, from, idx, errno, __FILE__,__LINE__);
#endif
        return -1;
      }

      for (i = 0; i < 0x100; ++i) {
        UniChar         dmapSrc[0x100];
        int32_t         j;
        for (j = 0; j < 0x100; ++j) {
          dmapSrc[j]=i * 0x100 + j;
        }
        char *  inBuf=(char *) dmapSrc;
        char *  outBuf=(char *) &(dmap_rec[idx].dmapU2S[i*0x100]);
        size_t  inBytesLeft=sizeof(dmapSrc);
        size_t  outBytesLeft=0x100;
        size_t  len;

        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
          if (inBytesLeft == 0 && outBytesLeft == 0) {    /* a number of substitution returns */
            continue;
          }
#ifdef DEBUG
          if (myconvDebug) {
            MyPrintf(STDERR_WITH_TIME,
                     "dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                     from, to, idx, errno, __FILE__,__LINE__);
            MyPrintf(STDERR_WITH_TIME,
                     "iconv() retuns %d, errno=%d, InBytesLeft=%d, OutBytesLeft=%d\n",
                     len, errno, inBytesLeft, outBytesLeft, __FILE__,__LINE__);
          }
#endif
          iconv_close(cd);
          return -1;
        }
      }
      iconv_close(cd);

      myconv_rec[idx].subS = dmap_rec[idx].dmapU2S[0x1A];
      myconv_rec[idx].subD = dmap_rec[idx].dmapU2S[0xFFFD];
      myconv_rec[idx].srcSubS = 0x1A;
      myconv_rec[idx].srcSubD = 0xFFFD;
    }



  } else if (((myconvIsDBCS(from) && myconvIsUnicode2(to)) && (dmap_rec[idx].codingSchema = DMAP_D2U)) ||
             ((myconvIsDBCS(from) && myconvIsUTF8(to)) && (dmap_rec[idx].codingSchema = DMAP_D28))) {

    /* single byte mapping */
    if ((dmap_rec[idx].dmapD12U = (UniChar *) alloc_root(&dmapMemRoot, 0x100 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_D2U, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapD12U, 0x00, 0x100 * 2);

    /* double byte mapping, assume 7 bit ASCII is not use as the first byte of DBCS. */
    if ((dmap_rec[idx].dmapD22U = (UniChar *) alloc_root(&dmapMemRoot, 0x8000 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_D2U, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapD22U, 0x00, 0x8000 * 2);

    myconv_rec[idx].allocatedSize=(0x100 + 0x8000) * 2;

    
    {
      char      dmapSrc[2];
      iconv_t   cd;
      int32_t   i;
      size_t    inBytesLeft;
      size_t    outBytesLeft;
      size_t    len;
      char *    inBuf;
      char *    outBuf;

#ifdef support_surrogate
      if ((cd = iconv_open("UTF-16", from)) == (iconv_t) -1) {
#else
      if ((cd = iconv_open("UCS-2", from)) == (iconv_t) -1) {
#endif
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d) failed with iconv_open(), errno = %d in %s at %d\n",
                   to, from, idx, errno, __FILE__,__LINE__);
#endif
        return -1;
      }

      for (i = 0; i < 0x100; ++i) {
        dmapSrc[0]=i;
        inBuf=dmapSrc;
        inBytesLeft=1;
        outBuf=(char *) (&dmap_rec[idx].dmapD12U[i]);
        outBytesLeft=2;
        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
          if ((errno == EILSEQ || errno == EINVAL) &&
              inBytesLeft == 1 &&
              outBytesLeft == 2) {
            continue;
          } else {
#ifdef DEBUG
            if (myconvDebug) {
              MyPrintf(STDERR_WITH_TIME,
                       "dmap_open(%s,%s,%d) failed to initialize with iconv(cd,%02x,%d,%02x%02x,%d), errno = %d in %s at %d\n",
                       to, from, idx, dmapSrc[0], 1,
                       (&dmap_rec[idx].dmapD12U[i])[0],(&dmap_rec[idx].dmapD12U[i])[1], 2,
                       errno, __FILE__,__LINE__);
              MyPrintf(STDERR_WITH_TIME,
                       "inBytesLeft=%d, outBytesLeft=%d, %02x%02x\n",
                       inBytesLeft, outBytesLeft,
                       (&dmap_rec[idx].dmapD12U[i])[0],(&dmap_rec[idx].dmapD12U[i])[1]);
            }
#endif
            iconv_close(cd);
            return -1;
          }            
          dmap_rec[idx].dmapD12U[i]=0x0000;
        }
        if (dmap_rec[idx].dmapD12U[i] == 0x001A &&      /* pick the first one */
            myconv_rec[idx].srcSubS == 0x00) {
          myconv_rec[idx].srcSubS=i;
        }
      }

      
      for (i = 0x80; i < 0x100; ++i) {
        int j;
        if (dmap_rec[idx].dmapD12U[i] != 0x0000)
          continue;
        for (j = 0x01; j < 0x100; ++j) {
          dmapSrc[0]=i;
          dmapSrc[1]=j;
          int offset = i-0x80;
          offset<<=8;
          offset+=j;

          inBuf=dmapSrc;
          inBytesLeft=2;
          outBuf=(char *) &(dmap_rec[idx].dmapD22U[offset]);
          outBytesLeft=2;
          if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
            if (inBytesLeft == 2 && outBytesLeft == 2 && (errno == EILSEQ || errno == EINVAL)) {
              ;  /* invalid DBCS character, dmapDD2U[offset] remains 0x0000 */
            } else {
#ifdef DEBUG
              if (myconvDebug) {
                MyPrintf(STDERR_WITH_TIME,
                         "dmap_open(%s,%s,%d) failed to initialize with iconv(cd,%p,2,%p,2), errno = %d in %s at %d\n",
                         to, from, idx,
                         dmapSrc, &(dmap_rec[idx].dmapD22U[offset]),
                         errno, __FILE__,__LINE__);
                MyPrintf(STDERR_WO_TIME, 
                         "iconv(cd,0x%02x%02x,2,0x%04x,2) returns %d, inBytesLeft=%d, outBytesLeft=%d\n",
                         dmapSrc[0], dmapSrc[1],
                         dmap_rec[idx].dmapD22U[offset],
                         len, inBytesLeft, outBytesLeft);
              }
#endif
              iconv_close(cd);
              return -1;
            }
          } else {
#ifdef TRACE_DMAP
            if (myconvDebug) {
              MyPrintf(STDERR_WITH_TIME,
                       "dmap_open(%s,%s,%d) failed to initialize with iconv(), rc=%d, errno=%d in %s at %d\n",
                       to, from, idx, len, errno, __FILE__,__LINE__);
              MyPrintf(STDERR_WITH_TIME,
                       "%04X: src=%04X%04X, inBuf=0x%02X%02X, inBytesLeft=%d, outBuf=%02X%02X%02X, outBytesLeft=%d\n",
                       i, dmapSrc[0], dmapSrc[1], inBuf[0], inBuf[1],
                       inBytesLeft, outBuf[-2], outBuf[-1], outBuf[0], outBytesLeft);
              MyPrintf(STDERR_WITH_TIME,
                       "&dmapSrc=%p, inBuf=%p, %p, outBuf=%p\n",
                       dmapSrc, inBuf, dmap_rec[idx].dmapU2M3 + (i - 0x80) * 2, outBuf);
            }
#endif
          }
        }
        if (dmap_rec[idx].dmapD12U[i] == 0xFFFD) {      /* pick the last one */
          myconv_rec[idx].srcSubD=i* 0x100 + j;
        }
      }
      iconv_close(cd);
    }

    myconv_rec[idx].subS=0x1A;
    myconv_rec[idx].subD=0xFFFD;
    myconv_rec[idx].srcSubD=0xFCFC;
    

  } else if (((myconvIsUCS2(from)  && myconvIsDBCS(to)) && (dmap_rec[idx].codingSchema = DMAP_U2D)) ||
             ((myconvIsUTF16(from) && myconvIsDBCS(to)) && (dmap_rec[idx].codingSchema = DMAP_T2D)) ||
             ((myconvIsUTF8(from)  && myconvIsDBCS(to)) && (dmap_rec[idx].codingSchema = DMAP_82D))) {
    /* UTF-16 -> DBCS single/double byte */
    /* A single table will cover all characters, assuming no second byte is 0x00. */
    if ((dmap_rec[idx].dmapU2D = (uchar *) alloc_root(&dmapMemRoot, 0x10000 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_U2D, errno, __FILE__,__LINE__);
#endif
      return -1;
    }

    memset(dmap_rec[idx].dmapU2D, 0x00, 0x10000 * 2);
    myconv_rec[idx].allocatedSize=(0x10000 * 2);

    {
      UniChar   dmapSrc[1];
      iconv_t   cd;
      int32_t   i;
      size_t    inBytesLeft;
      size_t    outBytesLeft;
      size_t    len;
      char *    inBuf;
      char *    outBuf;

#ifdef support_surrogate
      if ((cd = iconv_open(to, "UTF-16")) == (iconv_t) -1) {
#else
      if ((cd = iconv_open(to, "UCS-2")) == (iconv_t) -1) {
#endif
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d) failed with iconv_open(), errno = %d in %s at %d\n",
                 to, from, idx, errno, __FILE__,__LINE__);
#endif
        return -1;
      }

      /* easy implementation, convert 1 Unicode character at one time. */
      /* If the open performance is an issue, convert a chunk such as 128 chracters.  */
      /* if the converted length is not the same as the original, convert one by one. */
      (dmap_rec[idx].dmapU2D)[0x0000]=0x00;
      for (i = 1; i < 0x10000; ++i) {
        dmapSrc[0]=i;
        inBuf=(char *) dmapSrc;
        inBytesLeft=2;
        outBuf=(char *) &((dmap_rec[idx].dmapU2D)[2*i]);
        outBytesLeft=2;
        do {
          if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
            if (len == 1 && inBytesLeft == 0 && outBytesLeft == 1 && (dmap_rec[idx].dmapU2D)[2*i] == 0x1A) {
              /* UCS-2_TIS-620:0x0080 => 0x1A, converted to SBCS replacement character */
              (dmap_rec[idx].dmapU2D)[2*i+1]=0x00;
              break;
            } else if (len == 1 && inBytesLeft == 0 && outBytesLeft == 0) {
              break;
            }
            if (errno == EILSEQ || errno == EINVAL) {
#ifdef DEBUG
              if (myconvDebug) {
                MyPrintf(STDERR_WITH_TIME,
                         "dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                         to, from, idx, errno, __FILE__,__LINE__);
                MyPrintf(STDERR_WO_TIME,
                         "iconv(cd,%04x,2,%02x%02x,2) returns inBytesLeft=%d, outBytesLeft=%d\n",
                         dmapSrc[0],
                         (dmap_rec[idx].dmapU2D)[2*i], (dmap_rec[idx].dmapU2D)[2*i+1],
                         inBytesLeft, outBytesLeft);
                if (outBuf - (char *) dmap_rec[idx].dmapU2M2 > 1)
                  MyPrintf(STDERR_WO_TIME, "outBuf[-2..2]=%02X%02X%02X%02X%02X\n", outBuf[-2],outBuf[-1],outBuf[0],outBuf[1],outBuf[2]);
                else
                  MyPrintf(STDERR_WO_TIME, "outBuf[0..2]=%02X%02X%02X\n", outBuf[0],outBuf[1],outBuf[2]);
              }
#endif
              inBuf+=2;
              inBytesLeft-=2;
              memcpy(outBuf, (char *) &(myconv_rec[idx].subD), 2);
              outBuf+=2;
              outBytesLeft-=2;
            } else {
#ifdef DEBUG
              MyPrintf(STDERR_WITH_TIME,
                       "[%d] dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                       i, to, from, idx, errno, __FILE__,__LINE__);
              MyPrintf(STDERR_WO_TIME, 
                       "iconv(cd,%04x,2,%02x%02x,2) returns %d inBytesLeft=%d, outBytesLeft=%d\n",
                       dmapSrc[0],
                       (dmap_rec[idx].dmapU2D)[2*i],
                       (dmap_rec[idx].dmapU2D)[2*i+1],
                       len, inBytesLeft,outBytesLeft);
              if (i == 1) {
                MyPrintf(STDERR_WO_TIME, 
                         " inBuf [-1..2]=%02x%02x%02x%02x\n",
                         inBuf[-1],inBuf[0],inBuf[1],inBuf[2]);
                MyPrintf(STDERR_WO_TIME,
                         " outBuf [-1..2]=%02x%02x%02x%02x\n",
                         outBuf[-1],outBuf[0],outBuf[1],outBuf[2]);
              } else {
                MyPrintf(STDERR_WO_TIME, 
                         " inBuf [-2..2]=%02x%02x%02x%02x%02x\n",
                         inBuf[-2],inBuf[-1],inBuf[0],inBuf[1],inBuf[2]);
                MyPrintf(STDERR_WO_TIME,
                         " outBuf [-2..2]=%02x%02x%02x%02x%02x\n",
                         outBuf[-2],outBuf[-1],outBuf[0],outBuf[1],outBuf[2]);
              }
#endif
              iconv_close(cd);
              return -1;
            }
            if (len == 0 && inBytesLeft == 0 && outBytesLeft == 1) { /* converted to SBCS */
              (dmap_rec[idx].dmapU2D)[2*i+1]=0x00;
              break;
            }
          }
        } while (inBytesLeft > 0);
      }
      iconv_close(cd);
      myconv_rec[idx].subS = dmap_rec[idx].dmapU2D[2*0x1A];
      myconv_rec[idx].subD = dmap_rec[idx].dmapU2D[2*0xFFFD] * 0x100 
        + dmap_rec[idx].dmapU2D[2*0xFFFD+1];
      myconv_rec[idx].srcSubS = 0x1A;
      myconv_rec[idx].srcSubD = 0xFFFD;
    }


  } else if (((myconvIsEUC(from) && myconvIsUnicode2(to)) && (dmap_rec[idx].codingSchema = DMAP_E2U)) ||
             ((myconvIsEUC(from) && myconvIsUTF8(to)) && (dmap_rec[idx].codingSchema = DMAP_E28))) {
    /* S0: 0x00 - 0x7F */
    if ((dmap_rec[idx].dmapE02U = (UniChar *) alloc_root(&dmapMemRoot, 0x100 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_E2U, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapE02U, 0x00, 0x100 * 2);

    /* S1: 0xA0 - 0xFF, 0xA0 - 0xFF */
    if ((dmap_rec[idx].dmapE12U = (UniChar *) alloc_root(&dmapMemRoot, 0x60 * 0x60 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_E2U, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapE12U, 0x00, 0x60 * 0x60 * 2);

    /* SS2: 0x8E + 0xA0 - 0xFF, 0xA0 - 0xFF */
    if ((dmap_rec[idx].dmapE22U = (UniChar *) alloc_root(&dmapMemRoot, 0x60 * 0x61 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_E2U, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapE22U, 0x00, 0x60 * 0x61 * 2);

    /* SS3: 0x8F + 0xA0 - 0xFF, 0xA0 - 0xFF */
    if ((dmap_rec[idx].dmapE32U = (UniChar *) alloc_root(&dmapMemRoot, 0x60 * 0x61 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_E2U, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapE32U, 0x00, 0x60 * 0x61 * 2);

    myconv_rec[idx].allocatedSize=(0x100 + 0x60 * 0x60  + 0x60 * 0x61* 2) * 2;

    
    {
      char      dmapSrc[0x60 * 0x60 * 3];
      iconv_t   cd;
      int32_t   i;
      size_t    inBytesLeft;
      size_t    outBytesLeft;
      size_t    len;
      char *    inBuf;
      char *    outBuf;
      char      SS=0x8E;

#ifdef support_surrogate
      if ((cd = iconv_open("UTF-16", from)) == (iconv_t) -1) {
#else
      if ((cd = iconv_open("UCS-2", from)) == (iconv_t) -1) {
#endif
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d) failed with iconv_open(), errno = %d in %s at %d\n",
                   to, from, idx, errno, __FILE__,__LINE__);
#endif
        return -1;
      }

      for (i = 0; i < 0x100; ++i) {
        dmapSrc[0]=i;
        inBuf=dmapSrc;
        inBytesLeft=1;
        outBuf=(char *) (&dmap_rec[idx].dmapE02U[i]);
        outBytesLeft=2;
        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
#ifdef DEBUG
          if (myconvDebug) {
            MyPrintf(STDERR_WITH_TIME,
                     "dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                     to, from, idx, errno, __FILE__,__LINE__);
          }
#endif
          dmap_rec[idx].dmapE02U[i]=0x0000;
        }
        if (dmap_rec[idx].dmapE02U[i] == 0x001A &&      /* pick the first one */
            myconv_rec[idx].srcSubS == 0x00) {
          myconv_rec[idx].srcSubS=i;
        }
      }

      
      inBuf=dmapSrc;
      for (i = 0; i < 0x60; ++i) {
        int j;
        for (j = 0; j < 0x60; ++j) {
          *inBuf=i+0xA0;
          ++inBuf;
          *inBuf=j+0xA0;
          ++inBuf;
        }
      }
      inBuf=dmapSrc;
      inBytesLeft=0x60 * 0x60 * 2;
      outBuf=(char *) dmap_rec[idx].dmapE12U;
      outBytesLeft=0x60 * 0x60 * 2;
      do {
        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
          if (errno == EILSEQ) {
#ifdef DEBUG
            if (myconvDebug) {
              MyPrintf(STDERR_WITH_TIME,
                       "dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                       to, from, idx, errno, __FILE__,__LINE__);
              MyPrintf(STDERR_WO_TIME, "inBytesLeft=%d, outBytesLeft=%d\n", inBytesLeft, outBytesLeft);
              if (inBuf - dmapSrc > 1 && inBuf - dmapSrc <= sizeof(dmapSrc) - 2)
                MyPrintf(STDERR_WO_TIME, "inBuf[-2..2]=%02X%02X%02X%02X%02X\n", inBuf[-2],inBuf[-1],inBuf[0],inBuf[1],inBuf[2]);
              else
                MyPrintf(STDERR_WO_TIME, "inBuf[0..2]=%02X%02X%02X\n", inBuf[0],inBuf[1],inBuf[2]);
              if (outBuf - (char *) dmap_rec[idx].dmapE12U > 1)
                MyPrintf(STDERR_WO_TIME, "outBuf[-2..2]=%02X%02X%02X%02X%02X\n", outBuf[-2],outBuf[-1],outBuf[0],outBuf[1],outBuf[2]);
              else
                MyPrintf(STDERR_WO_TIME, "outBuf[0..2]=%02X%02X%02X\n", outBuf[0],outBuf[1],outBuf[2]);
            }
#endif
            inBuf+=2;
            inBytesLeft-=2;
            outBuf[0]=0x00;
            outBuf[1]=0x00;
            outBuf+=2;
            outBytesLeft-=2;
          } else {
#ifdef DEBUG
            MyPrintf(STDERR_WITH_TIME,
                     "dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                     to, from, idx, errno, __FILE__,__LINE__);
#endif
            iconv_close(cd);
            return -1;
          }
        }
      } while (inBytesLeft > 0);

      /* SS2: 0x8E + 1 or 2 bytes */
      /* SS3: 0x8E + 1 or 2 bytes */
      while (SS != 0x00) {
        int32_t numSuccess=0;
        for (i = 0; i < 0x60; ++i) {
          inBuf=dmapSrc;
          inBuf[0]=SS;
          inBuf[1]=i+0xA0;
          inBytesLeft=2;
          if (SS == 0x8E)
            outBuf=(char *) &(dmap_rec[idx].dmapE22U[i]);
          else
            outBuf=(char *) &(dmap_rec[idx].dmapE32U[i]);
          outBytesLeft=2;
          if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
            if (SS == 0x8E)
              dmap_rec[idx].dmapE22U[i]=0x0000;
            else
              dmap_rec[idx].dmapE32U[i]=0x0000;
          } else {
            ++numSuccess;
          }
        }
        if (numSuccess == 0) { /* SS2 is 2 bytes */
          inBuf=dmapSrc;
          for (i = 0; i < 0x60; ++i) {
            int j;
            for (j = 0; j < 0x60; ++j) {
              *inBuf=SS;
              ++inBuf;
              *inBuf=i+0xA0;
              ++inBuf;
              *inBuf=j+0xA0;
              ++inBuf;
            }
          }
          inBuf=dmapSrc;
          inBytesLeft=0x60 * 0x60 * 3;
          if (SS == 0x8E)
            outBuf=(char *) &(dmap_rec[idx].dmapE22U[0x60]);
          else
            outBuf=(char *) &(dmap_rec[idx].dmapE32U[0x60]);
          outBytesLeft=0x60 * 0x60 * 2;
          do {
            if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
#ifdef DEBUG
              if (myconvDebug) {
                MyPrintf(STDERR_WITH_TIME,
                         "%02X:dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                         SS, to, from, idx, errno, __FILE__,__LINE__);
                MyPrintf(STDERR_WO_TIME, "inBytesLeft=%d, outBytesLeft=%d\n", inBytesLeft, outBytesLeft);
                if (inBuf - dmapSrc > 1 && inBuf - dmapSrc <= sizeof(dmapSrc) - 2)
                  MyPrintf(STDERR_WO_TIME, "inBuf[-2..2]=%02X%02X%02X%02X%02X\n", inBuf[-2],inBuf[-1],inBuf[0],inBuf[1],inBuf[2]);
                else
                  MyPrintf(STDERR_WO_TIME, "inBuf[0..2]=%02X%02X%02X\n", inBuf[0],inBuf[1],inBuf[2]);
              }
#endif
              if (errno == EILSEQ || errno == EINVAL) {
                inBuf+=3;
                inBytesLeft-=3;
                outBuf[0]=0x00;
                outBuf[1]=0x00;
                outBuf+=2;
                outBytesLeft-=2;
              } else {
#ifdef DEBUG
                MyPrintf(STDERR_WITH_TIME,
                         "%02X:dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                         SS, to, from, idx, errno, __FILE__,__LINE__);
#endif
                iconv_close(cd);
                return -1;
              }
            }
          } while (inBytesLeft > 0);
        }
        if (SS == 0x8E)
          SS=0x8F;
        else 
          SS = 0x00;
      }
      iconv_close(cd);

      myconv_rec[idx].subS=0x1A;
      myconv_rec[idx].subD=0xFFFD;
      for (i = 0; i < 0x80; ++i) {
        if (dmap_rec[idx].dmapE02U[i] == 0x001A) {
          myconv_rec[idx].srcSubS=i;    /* pick the first one */
          break;
        }
      }

      for (i = 0; i < 0x60 * 0x60; ++i) {
        if (dmap_rec[idx].dmapE12U[i] == 0xFFFD) {
          uchar byte1=i / 0x60;
          uchar byte2=i % 0x60;
          myconv_rec[idx].srcSubD=(byte1 + 0xA0) * 0x100 + (byte2 + 0xA0);    /* pick the last one */
        }
      }

    }

  } else if (((myconvIsUCS2(from)  && myconvIsEUC(to)) && (dmap_rec[idx].codingSchema = DMAP_U2E)) ||
             ((myconvIsUTF16(from) && myconvIsEUC(to)) && (dmap_rec[idx].codingSchema = DMAP_T2E)) ||
             ((myconvIsUTF8(from)  && myconvIsEUC(to)) && (dmap_rec[idx].codingSchema = DMAP_82E))) {
    /* S0: 0x00 - 0xFF */
    if ((dmap_rec[idx].dmapU2S = (uchar *) alloc_root(&dmapMemRoot, 0x100)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_U2E, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapU2S, 0x00, 0x100);

    /* U0080 - UFFFF -> S1: 0xA0 - 0xFF, 0xA0 - 0xFF */
    if ((dmap_rec[idx].dmapU2M2 = (uchar *) alloc_root(&dmapMemRoot, 0xFF80 * 2)) == NULL) {
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
                 to, from, idx, DMAP_U2E, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapU2M2, 0x00, 0xFF80 * 2);

    /* U0080 - UFFFF -> SS2: 0x8E + 0xA0 - 0xFF, 0xA0 - 0xFF
     *                  SS3: 0x8F + 0xA0 - 0xFF, 0xA0 - 0xFF */
    if ((dmap_rec[idx].dmapU2M3 = (uchar *) alloc_root(&dmapMemRoot, 0xFF80 * 3)) == NULL) {
#ifdef DEBUG
      MyPrintf(STDERR_WITH_TIME,
               "dmap_open(%s,%s,%d), CS=%d failed with malloc(), errno = %d in %s at %d\n",
               to, from, idx, DMAP_U2E, errno, __FILE__,__LINE__);
#endif
      return -1;
    }
    memset(dmap_rec[idx].dmapU2M3, 0x00, 0xFF80 * 3);
    myconv_rec[idx].allocatedSize=(0x100 + 0xFF80 * 2 + 0xFF80 * 3);

    {
      UniChar   dmapSrc[0x80];
      iconv_t   cd;
      int32_t   i;
      size_t    inBytesLeft;
      size_t    outBytesLeft;
      size_t    len;
      char *    inBuf;
      char *    outBuf;

#ifdef support_surrogate
      if ((cd = iconv_open(to, "UTF-16")) == (iconv_t) -1) {
#else
      if ((cd = iconv_open(to, "UCS-2")) == (iconv_t) -1) {
#endif
#ifdef DEBUG
        MyPrintf(STDERR_WITH_TIME,
                 "dmap_open(%s,%s,%d) failed with iconv_open(), errno = %d in %s at %d\n",
                   to, from, idx, errno, __FILE__,__LINE__);
#endif
        return -1;
      }

      for (i = 0; i < 0x80; ++i)
        dmapSrc[i]=i;
      inBuf=(char *) dmapSrc;
      inBytesLeft=0x80 * 2;
      outBuf=(char *) dmap_rec[idx].dmapU2S;
      outBytesLeft=0x80;
      do {
        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
#ifdef DEBUG
          MyPrintf(STDERR_WITH_TIME,
                   "dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                   to, from, idx, errno, __FILE__,__LINE__);
#endif
          iconv_close(cd);
          return -1;
        }
      } while (inBytesLeft > 0);

      myconv_rec[idx].srcSubS = 0x1A;
      myconv_rec[idx].srcSubD = 0xFFFD;
      myconv_rec[idx].subS = dmap_rec[idx].dmapU2S[0x1A];
      
      outBuf=(char *) &(myconv_rec[idx].subD);
      dmapSrc[0]=0xFFFD;
      inBuf=(char *) dmapSrc;
      inBytesLeft=2;
      outBytesLeft=2;
      if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
#ifdef DEBUG
        if (myconvDebug) {
          MyPrintf(STDERR_WITH_TIME,
                   "dmap_open(%s,%s,%d) failed to initialize with iconv(), rc=%d, errno=%d in %s at %d\n",
                   to, from, idx, len, errno, __FILE__,__LINE__);
          MyPrintf(STDERR_WO_TIME, "iconv(0x1A,1,%p,1) returns outBuf=%p, outBytesLeft=%d\n",
                   dmapSrc, outBuf, outBytesLeft);
        }
#endif
        if (outBytesLeft == 0) {
          /* UCS-2_IBM-eucKR returns error.
             myconv(iconv) rc=1, error=0, InBytesLeft=0, OutBytesLeft=18
             myconv(iconvRev) rc=-1, error=116, InBytesLeft=2, OutBytesLeft=20
             iconv: 0xFFFD => 0xAFFE => 0x    rc=1,-1  sub=0,0
          */
          ;
        } else {
          iconv_close(cd);
          return -1;
        }
      }
      
      for (i = 0x80; i < 0xFFFF; ++i) {
        uchar   eucBuf[3];
        dmapSrc[0]=i;
        inBuf=(char *) dmapSrc;
        inBytesLeft=2;
        outBuf=(char *) eucBuf;
        outBytesLeft=sizeof(eucBuf);
        errno=0;
        if ((len = iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft)) != (size_t) 0) {
          if (len == 1 && errno == 0 && inBytesLeft == 0 && outBytesLeft == 1) {  /* substitution occurred. */            continue;
          }

          if (errno == EILSEQ) {
#ifdef DEBUG
            if (myconvDebug) {
              MyPrintf(STDERR_WITH_TIME,
                       "dmap_open(%s,%s,%d) failed to initialize with iconv(), errno = %d in %s at %d\n",
                       to, from, idx, errno, __FILE__,__LINE__);
              MyPrintf(STDERR_WO_TIME, "inBytesLeft=%d, outBytesLeft=%d\n", inBytesLeft, outBytesLeft);
              if (inBuf - (char *) dmapSrc > 1 && inBuf - (char *) dmapSrc <= sizeof(dmapSrc) - 2)
                MyPrintf(STDERR_WO_TIME, "inBuf[-2..2]=%02X%02X%02X%02X%02X\n", inBuf[-2],inBuf[-1],inBuf[0],inBuf[1],inBuf[2]);
              else
                MyPrintf(STDERR_WO_TIME, "inBuf[0..2]=%02X%02X%02X\n", inBuf[0],inBuf[1],inBuf[2]);
              if (outBuf - (char *) dmap_rec[idx].dmapU2M2 > 1)
                MyPrintf(STDERR_WO_TIME, "outBuf[-2..2]=%02X%02X%02X%02X%02X\n", outBuf[-2],outBuf[-1],outBuf[0],outBuf[1],outBuf[2]);
              else
                MyPrintf(STDERR_WO_TIME, "outBuf[0..2]=%02X%02X%02X\n", outBuf[0],outBuf[1],outBuf[2]);
            }
#endif
            inBuf+=2;
            inBytesLeft-=2;
            memcpy(outBuf, (char *) &(myconv_rec[idx].subD), 2);
            outBuf+=2;
            outBytesLeft-=2;
          } else {
#ifdef DEBUG
            if (myconvDebug) {
              MyPrintf(STDERR_WITH_TIME,
                       "dmap_open(%s,%s,%d) failed to initialize with iconv(), rc = %d, errno = %d in %s at %d\n",
                       to, from, idx, len, errno, __FILE__,__LINE__);
              MyPrintf(STDERR_WITH_TIME,
                       "%04X: src=%04X%04X, inBuf=0x%02X%02X, inBytesLeft=%d, outBuf[-2..0]=%02X%02X%02X, outBytesLeft=%d\n",
                       i, dmapSrc[0], dmapSrc[1], inBuf[0], inBuf[1],
                       inBytesLeft, outBuf[-2], outBuf[-1], outBuf[0], outBytesLeft);
              MyPrintf(STDERR_WITH_TIME,
                       "&dmapSrc=%p, inBuf=%p, dmapU2M2 + %d = %p, outBuf=%p\n",
                       dmapSrc, inBuf, (i - 0x80) * 2, dmap_rec[idx].dmapU2M2 + (i - 0x80) * 2, outBuf);
            }
#endif
            iconv_close(cd);
            return -1;
          }
        }
        if (sizeof(eucBuf) - outBytesLeft == 1) {
          if (i < 0x100) {
            (dmap_rec[idx].dmapU2S)[i]=eucBuf[0];
          } else {
            dmap_rec[idx].dmapU2M2[(i - 0x80) * 2]        = eucBuf[0];
            dmap_rec[idx].dmapU2M2[(i - 0x80) * 2 + 1]    = 0x00;
          }
        } else if (sizeof(eucBuf) - outBytesLeft == 2) {  /* 2 bytes */
          dmap_rec[idx].dmapU2M2[(i - 0x80) * 2]        = eucBuf[0];
          dmap_rec[idx].dmapU2M2[(i - 0x80) * 2 + 1]    = eucBuf[1];
        } else if (sizeof(eucBuf) - outBytesLeft == 3) { /* 3 byte SS2/SS3 */
          dmap_rec[idx].dmapU2M3[(i - 0x80) * 3]        = eucBuf[0];
          dmap_rec[idx].dmapU2M3[(i - 0x80) * 3 + 1]    = eucBuf[1];
          dmap_rec[idx].dmapU2M3[(i - 0x80) * 3 + 2]    = eucBuf[2];
        } else {
#ifdef DEBUG
          if (myconvDebug) {
            MyPrintf(STDERR_WITH_TIME,
                     "dmap_open(%s,%s,%d) failed to initialize with iconv(), rc=%d, errno=%d in %s at %d\n",
                     to, from, idx, len, errno, __FILE__,__LINE__);
            MyPrintf(STDERR_WITH_TIME,
                     "%04X: src=%04X%04X, inBuf=0x%02X%02X, inBytesLeft=%d, outBuf=%02X%02X%02X, outBytesLeft=%d\n",
                     i, dmapSrc[0], dmapSrc[1], inBuf[0], inBuf[1],
                     inBytesLeft, outBuf[-2], outBuf[-1], outBuf[0], outBytesLeft);
            MyPrintf(STDERR_WITH_TIME,
                     "&dmapSrc=%p, inBuf=%p, %p, outBuf=%p\n",
                     dmapSrc, inBuf, dmap_rec[idx].dmapU2M3 + (i - 0x80) * 2, outBuf);
          }
#endif
          return -1;
        }
          
      }
      iconv_close(cd);
    }

  } else if (myconvIsUTF16(from) && myconvIsUTF8(to)) {
    dmap_rec[idx].codingSchema = DMAP_T28;

  } else if (myconvIsUCS2(from) && myconvIsUTF8(to)) {
    dmap_rec[idx].codingSchema = DMAP_U28;

  } else if (myconvIsUTF8(from) && myconvIsUnicode2(to)) {
    dmap_rec[idx].codingSchema = DMAP_82U;

  } else if (myconvIsUnicode2(from) && myconvIsUnicode2(to)) {
    dmap_rec[idx].codingSchema = DMAP_U2U;

  } else {
    
    return -1;
  }
  myconv_rec[idx].cnv_dmap=&(dmap_rec[idx]);
  return 0;
}


/*
static int      bins_open(const char *  to,
                          const char *  from,
                          const int32_t idx)
{
  return -1;
}
*/



static int32_t  dmap_close(const int32_t        idx)
{
  if (dmap_rec[idx].codingSchema == DMAP_S2S) {
    if (dmap_rec[idx].dmapS2S != NULL) {
      dmap_rec[idx].dmapS2S=NULL;
    }
  } else if (dmap_rec[idx].codingSchema == DMAP_E2U) {
    if (dmap_rec[idx].dmapE02U != NULL) {
      dmap_rec[idx].dmapE02U=NULL;
    }
    if (dmap_rec[idx].dmapE12U != NULL) {
      dmap_rec[idx].dmapE12U=NULL;
    }
    if (dmap_rec[idx].dmapE22U != NULL) {
      dmap_rec[idx].dmapE22U=NULL;
    }
    if (dmap_rec[idx].dmapE32U != NULL) {
      dmap_rec[idx].dmapE32U=NULL;
    }
  }

  return 0;
}

/*
static int32_t  bins_close(const int32_t        idx)
{
  return 0;
}
*/


myconv_t myconv_open(const char *       toCode,
                     const char *       fromCode,
                     int32_t            converter)
{
  int32 i;
  for (i = 0; i < MAX_CONVERTER; ++i) {
    if (myconv_rec[i].converterType == 0)
      break;
  }
  if (i >= MAX_CONVERTER)
    return ((myconv_t) -1);

  myconv_rec[i].converterType = converter;
  myconv_rec[i].index=i;
  myconv_rec[i].fromCcsid=cstoccsid(fromCode);
  if (myconv_rec[i].fromCcsid == 0 && memcmp(fromCode, "big5",5) == 0)
    myconv_rec[i].fromCcsid=950;
  myconv_rec[i].toCcsid=cstoccsid(toCode);
  if (myconv_rec[i].toCcsid == 0 && memcmp(toCode, "big5",5) == 0)
    myconv_rec[i].toCcsid=950;
  strncpy(myconv_rec[i].from,   fromCode,       sizeof(myconv_rec[i].from)-1);
  strncpy(myconv_rec[i].to,     toCode,         sizeof(myconv_rec[i].to)-1);

  if (converter == CONVERTER_ICONV) {
    if ((myconv_rec[i].cnv_iconv=iconv_open(toCode, fromCode)) == (iconv_t) -1) {
      return ((myconv_t) -1);
    }
    myconv_rec[i].allocatedSize = -1;
    myconv_rec[i].srcSubS=myconvGetSubS(fromCode);
    myconv_rec[i].srcSubD=myconvGetSubD(fromCode);
    myconv_rec[i].subS=myconvGetSubS(toCode);
    myconv_rec[i].subD=myconvGetSubD(toCode);
    return &(myconv_rec[i]);
  } else if (converter == CONVERTER_DMAP &&
             dmap_open(toCode, fromCode, i) != -1) {
    return &(myconv_rec[i]);
  }
  return ((myconv_t) -1);
}



int32_t myconv_close(myconv_t cd)
{
  int32_t   ret=0;

  if (cd->converterType == CONVERTER_ICONV) {
    ret=iconv_close(cd->cnv_iconv);
  } else if (cd->converterType == CONVERTER_DMAP) {
    ret=dmap_close(cd->index);
  }
  memset(&(myconv_rec[cd->index]), 0x00, sizeof(myconv_rec[cd->index]));
  return ret;
}




/* reference: http://www-306.ibm.com/software/globalization/other/es.jsp */
/* systemCL would be expensive, and myconvIsXXXXX is called frequently.
   need to cache entries */
#define MAX_CCSID       256
static int      ccsidList      [MAX_CCSID];
static int      esList         [MAX_CCSID];
int32 getEncodingScheme(const uint16 inCcsid, int32& outEncodingScheme);
EXTERN int      myconvGetES(CCSID     ccsid) 
{
  /* call QtqValidateCCSID in ILE to get encoding schema */
  /*  return QtqValidateCCSID(ccsid); */
  int   i;
  for (i = 0; i < MAX_CCSID; ++i) {
    if (ccsidList[i] == (int) ccsid)
      return esList[i];
    if (ccsidList[i] == 0x00)
      break;
  }

  if (i >= MAX_CCSID) {
    i=MAX_CCSID-1;
  }

  { 
    ccsidList[i]=ccsid;
    getEncodingScheme(ccsid, esList[i]);
#ifdef DEBUG_PASE
    if (myconvDebug) {
      fprintf(stderr, "CCSID=%d, ES=0x%04X\n", ccsid, esList[i]);
    }
#endif
    return esList[i];
  }
  return 0;
}


EXTERN  int     myconvIsEBCDIC(const char *     pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x1100 ||
      es == 0x1200 ||
      es == 0x6100 ||
      es == 0x6200 ||
      es == 0x1301 ) {
    return TRUE;
  }
  return FALSE;
}


EXTERN int      myconvIsISO(const char *    pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x4100 ||
      es == 0x4105 ||
      es == 0x4155 ||
      es == 0x5100 ||
      es == 0x5150 ||
      es == 0x5200 ||
      es == 0x5404 ||
      es == 0x5409 ||
      es == 0x540A ||
      es == 0x5700) {
    return TRUE;
  }
  return FALSE;
}


EXTERN  int     myconvIsASCII(const char *      pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x2100 ||
      es == 0x3100 ||
      es == 0x8100 ||
      es == 0x2200 ||
      es == 0x3200 ||
      es == 0x9200 ||
      es == 0x2300 ||
      es == 0x2305 ||
      es == 0x3300 ||
      es == 0x2900 ||
      es == 0x2A00) {
    return TRUE;
  } else if (memcmp(pName, "big5", 5) == 0) {
    return TRUE;
  }
  return FALSE;
}



EXTERN  int     myconvIsUCS2(const char *       pName)
{
  if (cstoccsid(pName) == 13488) {
    return TRUE;
  }
  return FALSE;
}


EXTERN  int     myconvIsUTF16(const char *       pName)
{
  if (cstoccsid(pName) == 1200) {
    return TRUE;
  }
  return FALSE;
}


EXTERN  int     myconvIsUnicode2(const char *       pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x7200 ||
      es == 0x720B ||
      es == 0x720F) {
    return TRUE;
  }
  return FALSE;
}


EXTERN  int     myconvIsUTF8(const char *       pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x7807) {
    return TRUE;
  }
  return FALSE;
}


EXTERN  int     myconvIsUnicode(const char *       pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x7200 ||
      es == 0x720B ||
      es == 0x720F ||
      es == 0x7807) {
    return TRUE;
  }
  return FALSE;
}


EXTERN  int     myconvIsEUC(const char *        pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x4403) {
    return TRUE;
  }
  return FALSE;
}


EXTERN int      myconvIsDBCS(const char *   pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x1200 ||
      es == 0x2200 ||
      es == 0x2300 ||
      es == 0x2305 ||
      es == 0x2A00 ||
      es == 0x3200 ||
      es == 0x3300 ||
      es == 0x5200 ||
      es == 0x6200 ||
      es == 0x9200) {
    return TRUE;
  } else if (memcmp(pName, "big5", 5) == 0) {
    return TRUE;
  }
  return FALSE;
}


EXTERN int      myconvIsSBCS(const char *   pName)
{
  int   es = myconvGetES(cstoccsid(pName));
  if (es == 0x1100 ||
      es == 0x2100 ||
      es == 0x3100 ||
      es == 0x4100 ||
      es == 0x4105 ||
      es == 0x5100 ||
      es == 0x5150 ||
      es == 0x6100 ||
      es == 0x8100) {
    return TRUE;
  }
  return FALSE;
}



EXTERN char myconvGetSubS(const char *     code) 
{
  if (myconvIsEBCDIC(code)) {
    return 0x3F;
  } else if (myconvIsASCII(code)) {
    return 0x1A;
  } else if (myconvIsISO(code)) {
    return 0x1A;
  } else if (myconvIsEUC(code)) {
    return 0x1A;
  } else if (myconvIsUCS2(code)) {
    return 0x00;
  } else if (myconvIsUTF8(code)) {
    return 0x1A;
  }
  return 0x00;
}


EXTERN UniChar myconvGetSubD(const char *     code) 
{
  if (myconvIsEBCDIC(code)) {
    return 0xFDFD;
  } else if (myconvIsASCII(code)) {
    return 0xFCFC;
  } else if (myconvIsISO(code)) {
    return 0x00;
  } else if (myconvIsEUC(code)) {
    return 0x00;
  } else if (myconvIsUCS2(code)) {
    return 0xFFFD;
  } else if (myconvIsUTF8(code)) {
    return 0x00;
  }
  return 0x00;
}

// was inline
EXTERN  size_t	        myconv_iconv(myconv_t   cd ,
                                     char**     inBuf,
                                     size_t*    inBytesLeft,
                                     char**     outBuf,
                                     size_t*    outBytesLeft,
                                     size_t*    numSub)
{
  return iconv(cd->cnv_iconv, inBuf, inBytesLeft, outBuf, outBytesLeft);
}

EXTERN  size_t	        myconv_dmap(myconv_t    cd,
                                    char**      inBuf,
                                    size_t*     inBytesLeft,
                                    char**      outBuf,
                                    size_t*     outBytesLeft,
                                    size_t*     numSub)
{
  if (cd->cnv_dmap->codingSchema == DMAP_S2S) {
    register unsigned char *   dmapS2S=cd->cnv_dmap->dmapS2S;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register size_t     numS=0;
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
      } else {
        *pOut=dmapS2S[(uchar)*pIn];
        if (*pOut == 0x00) {
          errno=EILSEQ;  /* 116 */
          *outBytesLeft-=(*inBytesLeft-inLen);
          *inBytesLeft=inLen;
          *outBuf=pOut;
          *inBuf=pIn;
          *numSub+=numS;
          return -1;
        }
        if (*pOut == subS) {
          if ((*pOut=dmapS2S[(uchar)*pIn]) == subS) {
            if (*pIn != cd->srcSubS)
              ++numS;
          }
        }
      }
      ++pIn;
      --inLen;
      ++pOut;
    }
    *outBytesLeft-=(*inBytesLeft-inLen);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_E2U) {
    /* use uchar * instead of UniChar to avoid memcpy */
    register uchar *    dmapE02U=(uchar *) (cd->cnv_dmap->dmapE02U);
    register uchar *    dmapE12U=(uchar *) (cd->cnv_dmap->dmapE12U);
    register uchar *    dmapE22U=(uchar *) (cd->cnv_dmap->dmapE22U);
    register uchar *    dmapE32U=(uchar *) (cd->cnv_dmap->dmapE32U);
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register int        offset;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register size_t     numS=0;
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
        ++pOut;
        *pOut=0x00;
        ++pOut;
        ++pIn;
        --inLen;
      } else {
        if (*pIn == 0x8E) {      /* SS2 */
          if (inLen < 2) {
            if (cd->fromCcsid == 33722 ||       /* IBM-eucJP */
                cd->fromCcsid == 964)           /* IBM-eucTW */
              errno=EINVAL;  /* 22 */
            else
              errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            return -1;
          }
          ++pIn;
          if (*pIn < 0xA0) {
            if (cd->fromCcsid == 964)           /* IBM-eucTW */
              errno=EINVAL;  /* 22 */
            else
              errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            return -1;
          }
          offset=(*pIn - 0xA0);
          offset<<=1;
          if (dmapE22U[offset]   == 0x00 &&
              dmapE22U[offset+1] == 0x00) {     /* 2 bytes */
            if (inLen < 3) {
              if (cd->fromCcsid == 964)           /* IBM-eucTW */
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            offset=(*pIn - 0xA0) * 0x60 + 0x60;
            ++pIn;
            if (*pIn < 0xA0) {
              if (cd->fromCcsid == 964)           /* IBM-eucTW */
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            offset+=(*pIn - 0xA0);
            offset<<=1;
            if (dmapE22U[offset] == 0x00 &&
                dmapE22U[offset+1] == 0x00) {
              if (cd->fromCcsid == 964)           /* IBM-eucTW */
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            *pOut=dmapE22U[offset];
            ++pOut;
            *pOut=dmapE22U[offset+1];
            ++pOut;
            if (dmapE22U[offset] == 0xFF &&
                dmapE22U[offset+1] == 0xFD) {
              if (pIn[-2] * 0x100 + pIn[-1] != cd->srcSubD)
                ++numS;
            }
            ++pIn;
            inLen-=3;
          } else {      /* 1 bytes */
            *pOut=dmapE22U[offset];
            ++pOut;
            *pOut=dmapE22U[offset+1];
            ++pOut;
            ++pIn;
            inLen-=2;
          }
        } else if (*pIn == 0x8F) {     /* SS3 */
          if (inLen < 2) {
            if (cd->fromCcsid == 33722)   /* IBM-eucJP */
              errno=EINVAL;  /* 22 */
            else
              errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            return -1;
          }
          ++pIn;
          if (*pIn < 0xA0) {
            if (cd->fromCcsid == 970 ||         /* IBM-eucKR */
                cd->fromCcsid == 964 ||         /* IBM-eucTW */
                cd->fromCcsid == 1383 ||        /* IBM-eucCN */
                (cd->fromCcsid == 33722 && 3 <= inLen)) /* IBM-eucJP */
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            return -1;
          }
          offset=(*pIn - 0xA0);
          offset<<=1;
          if (dmapE32U[offset]   == 0x00 &&
              dmapE32U[offset+1] == 0x00) { /* 0x8F + 2 bytes */
            if (inLen < 3) {
              if (cd->fromCcsid == 33722)
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            offset=(*pIn - 0xA0) * 0x60 + 0x60;
            ++pIn;
            if (*pIn < 0xA0) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            offset+=(*pIn - 0xA0);
            offset<<=1;
            if (dmapE32U[offset] == 0x00 &&
                dmapE32U[offset+1] == 0x00) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            *pOut=dmapE32U[offset];
            ++pOut;
            *pOut=dmapE32U[offset+1];
            ++pOut;
            if (dmapE32U[offset] == 0xFF &&
                dmapE32U[offset+1] == 0xFD) {
              if (pIn[-2] * 0x100 + pIn[-1] != cd->srcSubD)
                ++numS;
            }
            ++pIn;
            inLen-=3;
          } else {      /* 0x8F + 1 bytes */
            *pOut=dmapE32U[offset];
            ++pOut;
            *pOut=dmapE32U[offset+1];
            ++pOut;
            ++pIn;
            inLen-=2;
          }

        } else {
          offset=*pIn;
          offset<<=1;
          if (dmapE02U[offset]   == 0x00 &&
              dmapE02U[offset+1] == 0x00) {         /* SS1 */
            if (inLen < 2) {
              if ((cd->fromCcsid == 33722 && (*pIn == 0xA0 || (0xA9 <= *pIn && *pIn <= 0xAF) || *pIn == 0xFF)) ||
                  (cd->fromCcsid == 970 && (*pIn == 0xA0 || *pIn == 0xAD || *pIn == 0xAE || *pIn == 0xAF || *pIn == 0xFF)) ||
                  (cd->fromCcsid == 964 && (*pIn == 0xA0 || (0xAA <= *pIn && *pIn <= 0xC1) || *pIn == 0xC3 || *pIn == 0xFE || *pIn == 0xFF)) ||
                  (cd->fromCcsid == 1383 && (*pIn == 0xA0 || *pIn == 0xFF)))
                errno=EILSEQ;  /* 116 */
              else
                errno=EINVAL;  /* 22 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn;
              return -1;
            }
            if (*pIn < 0xA0) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn;
              return -1;
            }
            offset=(*pIn - 0xA0) * 0x60;
            ++pIn;
            if (*pIn < 0xA0) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            offset+=(*pIn - 0xA0);
            offset<<=1;
            if (dmapE12U[offset] == 0x00 &&
                dmapE12U[offset+1] == 0x00) {   /* undefined mapping */
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            *pOut=dmapE12U[offset];
            ++pOut;
            *pOut=dmapE12U[offset+1];
            ++pOut;
            if (dmapE12U[offset] == 0xFF &&
                dmapE12U[offset+1] == 0xFD) {
              if (pIn[-1] * 0x100 + pIn[0] != cd->srcSubD)
                ++numS;
            }
            ++pIn;
            inLen-=2;
          } else {
            *pOut=dmapE02U[offset];
            ++pOut;
            *pOut=dmapE02U[offset+1];
            ++pOut;
            if (dmapE02U[offset] == 0x00 &&
                dmapE02U[offset+1] == 0x1A) {
              if (*pIn != cd->srcSubS)
                ++numS;
            }
            ++pIn;
            --inLen;
          }
        }
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;


  } else if (cd->cnv_dmap->codingSchema == DMAP_E28) {
    /* use uchar * instead of UniChar to avoid memcpy */
    register uchar *    dmapE02U=(uchar *) (cd->cnv_dmap->dmapE02U);
    register uchar *    dmapE12U=(uchar *) (cd->cnv_dmap->dmapE12U);
    register uchar *    dmapE22U=(uchar *) (cd->cnv_dmap->dmapE22U);
    register uchar *    dmapE32U=(uchar *) (cd->cnv_dmap->dmapE32U);
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register int        offset;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register size_t     numS=0;
    register UniChar    in;     /* copy part of U28 */
    register UniChar    ucs2;
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
        ++pOut;
        ++pIn;
        --inLen;
      } else {
        if (*pIn == 0x8E) {      /* SS2 */
          if (inLen < 2) {
            if (cd->fromCcsid == 33722 ||       /* IBM-eucJP */
                cd->fromCcsid == 964)           /* IBM-eucTW */
              errno=EINVAL;  /* 22 */
            else
              errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            return -1;
          }
          ++pIn;
          if (*pIn < 0xA0) {
            if (cd->fromCcsid == 964)           /* IBM-eucTW */
              errno=EINVAL;  /* 22 */
            else
              errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            return -1;
          }
          offset=(*pIn - 0xA0);
          offset<<=1;
          if (dmapE22U[offset]   == 0x00 &&
              dmapE22U[offset+1] == 0x00) {     /* 2 bytes */
            if (inLen < 3) {
              if (cd->fromCcsid == 964)           /* IBM-eucTW */
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            offset=(*pIn - 0xA0) * 0x60 + 0x60;
            ++pIn;
            if (*pIn < 0xA0) {
              if (cd->fromCcsid == 964)           /* IBM-eucTW */
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            offset+=(*pIn - 0xA0);
            offset<<=1;
            if (dmapE22U[offset] == 0x00 &&
                dmapE22U[offset+1] == 0x00) {
              if (cd->fromCcsid == 964)           /* IBM-eucTW */
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            in=dmapE22U[offset];
            in<<=8;
            in+=dmapE22U[offset+1];
            if (dmapE22U[offset] == 0xFF &&
                dmapE22U[offset+1] == 0xFD) {
              if (pIn[-2] * 0x100 + pIn[-1] != cd->srcSubD)
                ++numS;
            }
            ++pIn;
            inLen-=3;
          } else {      /* 1 bytes */
            in=dmapE22U[offset];
            in<<=8;
            in+=dmapE22U[offset+1];
            ++pIn;
            inLen-=2;
          }
        } else if (*pIn == 0x8F) {     /* SS3 */
          if (inLen < 2) {
            if (cd->fromCcsid == 33722)   /* IBM-eucJP */
              errno=EINVAL;  /* 22 */
            else
              errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            return -1;
          }
          ++pIn;
          if (*pIn < 0xA0) {
            if (cd->fromCcsid == 970 ||         /* IBM-eucKR */
                cd->fromCcsid == 964 ||         /* IBM-eucTW */
                cd->fromCcsid == 1383 ||        /* IBM-eucCN */
                (cd->fromCcsid == 33722 && 3 <= inLen)) /* IBM-eucJP */
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            return -1;
          }
          offset=(*pIn - 0xA0);
          offset<<=1;
          if (dmapE32U[offset]   == 0x00 &&
              dmapE32U[offset+1] == 0x00) { /* 0x8F + 2 bytes */
            if (inLen < 3) {
              if (cd->fromCcsid == 33722)
                errno=EINVAL;  /* 22 */
              else
                errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            offset=(*pIn - 0xA0) * 0x60 + 0x60;
            ++pIn;
            if (*pIn < 0xA0) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            offset+=(*pIn - 0xA0);
            offset<<=1;
            if (dmapE32U[offset] == 0x00 &&
                dmapE32U[offset+1] == 0x00) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-2;
              return -1;
            }
            in=dmapE32U[offset];
            in<<=8;
            in+=dmapE32U[offset+1];
            if (dmapE32U[offset] == 0xFF &&
                dmapE32U[offset+1] == 0xFD) {
              if (pIn[-2] * 0x100 + pIn[-1] != cd->srcSubD)
                ++numS;
            }
            ++pIn;
            inLen-=3;
          } else {      /* 0x8F + 1 bytes */
            in=dmapE32U[offset];
            in<<=8;
            in+=dmapE32U[offset+1];
            ++pIn;
            inLen-=2;
          }

        } else {
          offset=*pIn;
          offset<<=1;
          if (dmapE02U[offset]   == 0x00 &&
              dmapE02U[offset+1] == 0x00) {         /* SS1 */
            if (inLen < 2) {
              if ((cd->fromCcsid == 33722 && (*pIn == 0xA0 || (0xA9 <= *pIn && *pIn <= 0xAF) || *pIn == 0xFF)) ||
                  (cd->fromCcsid == 970 && (*pIn == 0xA0 || *pIn == 0xAD || *pIn == 0xAE || *pIn == 0xAF || *pIn == 0xFF)) ||
                  (cd->fromCcsid == 964 && (*pIn == 0xA0 || (0xAA <= *pIn && *pIn <= 0xC1) || *pIn == 0xC3 || *pIn == 0xFE || *pIn == 0xFF)) ||
                  (cd->fromCcsid == 1383 && (*pIn == 0xA0 || *pIn == 0xFF)))
                errno=EILSEQ;  /* 116 */
              else
                errno=EINVAL;  /* 22 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn;
              return -1;
            }
            if (*pIn < 0xA0) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn;
              return -1;
            }
            offset=(*pIn - 0xA0) * 0x60;
            ++pIn;
            if (*pIn < 0xA0) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            offset+=(*pIn - 0xA0);
            offset<<=1;
            if (dmapE12U[offset] == 0x00 &&
                dmapE12U[offset+1] == 0x00) {   /* undefined mapping */
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-1;
              return -1;
            }
            in=dmapE12U[offset];
            in<<=8;
            in+=dmapE12U[offset+1];
            if (dmapE12U[offset] == 0xFF &&
                dmapE12U[offset+1] == 0xFD) {
              if (pIn[-1] * 0x100 + pIn[0] != cd->srcSubD)
                ++numS;
            }
            ++pIn;
            inLen-=2;
          } else {
            in=dmapE02U[offset];
            in<<=8;
            in+=dmapE02U[offset+1];
            if (dmapE02U[offset] == 0x00 &&
                dmapE02U[offset+1] == 0x1A) {
              if (*pIn != cd->srcSubS)
                ++numS;
            }
            ++pIn;
            --inLen;
          }
        }
        ucs2=in;
        if ((in & 0xFF80) == 0x0000) {     /* U28: in & 0b1111111110000000 == 0x0000 */
          *pOut=in;
          ++pOut;
        } else if ((in & 0xF800) == 0x0000) {     /* in & 0b1111100000000000 == 0x0000 */
          register uchar        byte;
          in>>=6;
          in&=0x001F;   /* 0b0000000000011111 */
          in|=0x00C0;   /* 0b0000000011000000 */
          *pOut=in;
          ++pOut;
          byte=ucs2;    /* dmapD12U[offset+1]; */
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;
        } else if ((in & 0xFC00) == 0xD800) {
          *pOut=0xEF;
          ++pOut;
          *pOut=0xBF;
          ++pOut;
          *pOut=0xBD;
          ++pOut;
        } else {
          register uchar        byte;
          register uchar        work;
          byte=(ucs2>>8);    /* dmapD12U[offset]; */
          byte>>=4;
          byte|=0xE0;   /* 0b11100000; */
          *pOut=byte;
          ++pOut;

          byte=(ucs2>>8);    /* dmapD12U[offset]; */
          byte<<=2;
          work=ucs2;    /* dmapD12U[offset+1]; */
          work>>=6;
          byte|=work;
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;

          byte=ucs2;    /* dmapD12U[offset+1]; */
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;
        }
        /* end of U28 */
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_U2E) {
    register uchar *    dmapU2S=cd->cnv_dmap->dmapU2S;
    register uchar *    dmapU2M2=cd->cnv_dmap->dmapU2M2 - 0x80 * 2;
    register uchar *    dmapU2M3=cd->cnv_dmap->dmapU2M3 - 0x80 * 3;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register char *     pSubD=(char *) &(cd->subD);
    register size_t     numS=0;
    register size_t     rc=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;  /* 22 */
        *outBytesLeft-=(pOut-*outBuf);
        *inBytesLeft=inLen;
        *outBuf=pOut;
        *inBuf=pIn;
        return -1;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else if (in < 0x100 && dmapU2S[in] != 0x0000) {
        if ((*pOut=dmapU2S[in]) == subS) {
          if (in != cd->srcSubS)
            ++numS;
        }
        ++pOut;
      } else {
        in<<=1;
        if (dmapU2M2[in] == 0x00) {     /* not found in dmapU2M2 */
          in*=1.5;
          if (dmapU2M3[in] == 0x00) {   /* not found in dmapU2M3*/
            *pOut=pSubD[0];
            ++pOut;
            *pOut=pSubD[1];
            ++pOut;
            ++numS;
            ++rc;
          } else {
            *pOut=dmapU2M3[in];
            ++pOut;
            *pOut=dmapU2M3[1+in];
            ++pOut;
            *pOut=dmapU2M3[2+in];
            ++pOut;
          }
        } else {
          *pOut=dmapU2M2[in];
          ++pOut;
          if (dmapU2M2[1+in] == 0x00) {
            if (*pOut == subS) {
              in>>=1;
              if (in != cd->srcSubS)
                ++numS;
            }
          } else {
            *pOut=dmapU2M2[1+in];
            ++pOut;
            if (memcmp(pOut-2, pSubD, 2) == 0) {
              in>>=1;
              if (in != cd->srcSubD) {
                ++numS;
                ++rc;
              }
            }
          }
        }
      }
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return rc;        /* compatibility to iconv() */

  } else if (cd->cnv_dmap->codingSchema == DMAP_T2E) {
    register uchar *    dmapU2S=cd->cnv_dmap->dmapU2S;
    register uchar *    dmapU2M2=cd->cnv_dmap->dmapU2M2 - 0x80 * 2;
    register uchar *    dmapU2M3=cd->cnv_dmap->dmapU2M3 - 0x80 * 3;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register char *     pSubD=(char *) &(cd->subD);
    register size_t     numS=0;
    register size_t     rc=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;  /* 22 */
        *outBytesLeft-=(pOut-*outBuf);
        *inBytesLeft=inLen-1;
        *outBuf=pOut;
        *inBuf=pIn;
        ++numS;
        *numSub+=numS;
        return 0;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else if (0xD800 <= in && in <= 0xDBFF) { /* first byte of surrogate */
        errno=EINVAL;   /* 22 */
        *inBytesLeft=inLen-2;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn+2;
        ++numS;
        *numSub+=numS;
        return -1;
        
      } else if (0xDC00 <= in && in <= 0xDFFF) { /* second byte of surrogate */
        errno=EINVAL;   /* 22 */
        *inBytesLeft=inLen-1;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        ++numS;
        *numSub+=numS;
        return -1;
        
      } else if (in < 0x100 && dmapU2S[in] != 0x0000) {
        if ((*pOut=dmapU2S[in]) == subS) {
          if (in != cd->srcSubS)
            ++numS;
        }
        ++pOut;
      } else {
        in<<=1;
        if (dmapU2M2[in] == 0x00) {     /* not found in dmapU2M2 */
          in*=1.5;
          if (dmapU2M3[in] == 0x00) {   /* not found in dmapU2M3*/
            *pOut=pSubD[0];
            ++pOut;
            *pOut=pSubD[1];
            ++pOut;
            ++numS;
            ++rc;
          } else {
            *pOut=dmapU2M3[in];
            ++pOut;
            *pOut=dmapU2M3[1+in];
            ++pOut;
            *pOut=dmapU2M3[2+in];
            ++pOut;
          }
        } else {
          *pOut=dmapU2M2[in];
          ++pOut;
          if (dmapU2M2[1+in] == 0x00) {
            if (*pOut == subS) {
              in>>=1;
              if (in != cd->srcSubS)
                ++numS;
            }
          } else {
            *pOut=dmapU2M2[1+in];
            ++pOut;
            if (memcmp(pOut-2, pSubD, 2) == 0) {
              in>>=1;
              if (in != cd->srcSubD) {
                ++numS;
                ++rc;
              }
            }
          }
        }
      }
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_82E) {
    register uchar *    dmapU2S=cd->cnv_dmap->dmapU2S;
    register uchar *    dmapU2M2=cd->cnv_dmap->dmapU2M2 - 0x80 * 2;
    register uchar *    dmapU2M3=cd->cnv_dmap->dmapU2M3 - 0x80 * 3;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register char *     pSubD=(char *) &(cd->subD);
    register size_t     numS=0;
    register size_t     rc=0;
    while (0 < inLen) {
      register uint32_t in;
      uint32_t          in2;
      if (pLastOutBuf < pOut)
        break;
      /* convert from UTF-8 to UCS-2 */
      if (*pIn == 0x00) {
        in=0x0000;
        ++pIn;
        --inLen;
      } else {                          /* 82U: */
        register uchar byte1=*pIn;
        if ((byte1 & 0x80) == 0x00) {         /* if (byte1 & 0b10000000 == 0b00000000) { */
          /* 1 bytes sequence:  0xxxxxxx => 00000000 0xxxxxxx*/
          in=byte1;
          ++pIn;
          --inLen;
        } else if ((byte1 & 0xE0) == 0xC0) {  /* (byte1 & 0b11100000 == 0b11000000) { */
          if (inLen < 2) {
            errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          if (byte1 == 0xC0 || byte1 == 0xC1) {  /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          /* 2 bytes sequence: 
             110yyyyy 10xxxxxx => 00000yyy yyxxxxxx */
          register uchar byte2;
          ++pIn;
          byte2=*pIn;
          if ((byte2 & 0xC0) == 0x80) {       /* byte2 & 0b11000000 == 0b10000000) { */
            register uchar work=byte1;
            work<<=6;
            byte2&=0x3F;      /* 0b00111111; */
            byte2|=work;
              
            byte1&=0x1F;      /* 0b00011111; */
            byte1>>=2;
            in=byte1;
            in<<=8;
            in+=byte2;
            inLen-=2;
            ++pIn;
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            *numSub+=numS;
            return -1;
          }
        } else if ((byte1 & 0xF0) == 0xE0) {    /* byte1 & 0b11110000 == 0b11100000 */
          /* 3 bytes sequence: 
             1110zzzz 10yyyyyy 10xxxxxx => zzzzyyyy yyxxxxxx */
          register uchar byte2;
          register uchar byte3;
          if (inLen < 3) {
            if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          if ((byte2 & 0xC0) != 0x80 ||
              (byte3 & 0xC0) != 0x80 ||
              (byte1 == 0xE0 && byte2 < 0xA0)) { /* invalid sequence, only 0xA0-0xBF allowed after 0xE0 */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-2;
            *numSub+=numS;
            return -1;
          }
          {
            register uchar work=byte2;
            work<<=6;
            byte3&=0x3F;      /* 0b00111111; */
            byte3|=work;
              
            byte2&=0x3F;      /* 0b00111111; */
            byte2>>=2;

            byte1<<=4;
            in=byte1 | byte2;;
            in<<=8;
            in+=byte3;
            inLen-=3;
            ++pIn;
          }
        } else if ((0xF0 <= byte1 && byte1 <= 0xF4)) {    /* (bytes1 & 11111000) == 0x1110000 */
          /* 4 bytes sequence
             11110uuu 10uuzzzz 10yyyyyy 10xxxxxx => 110110ww wwzzzzyy 110111yy yyxxxxxx
             where uuuuu = wwww + 1 */
          register uchar byte2;
          register uchar byte3;
          register uchar byte4;
          if (inLen < 4) {
            if ((inLen >= 2 && (pIn[1]  & 0xC0) != 0x80) ||
                (inLen >= 3 && (pIn[2]  & 0xC0) != 0x80) ||
                (cd->toCcsid == 13488) )
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          ++pIn;
          byte4=*pIn;
          if ((byte2 & 0xC0) == 0x80 &&         /* byte2 & 0b11000000 == 0b10000000 */
              (byte3 & 0xC0) == 0x80 &&         /* byte3 & 0b11000000 == 0b10000000 */
              (byte4 & 0xC0) == 0x80) {         /* byte4 & 0b11000000 == 0b10000000 */
            register uchar work=byte2;
            if (byte1 == 0xF0 && byte2 < 0x90) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
              /* iconv() returns 0 for 0xF4908080 and convert to 0x00 
            } else if (byte1 == 0xF4 && byte2 > 0x8F) {
              errno=EINVAL;
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
              */
            }

            work&=0x30;       /* 0b00110000; */
            work>>=4;
            byte1&=0x07;      /* 0b00000111; */
            byte1<<=2;
            byte1+=work;      /* uuuuu */
            --byte1;          /* wwww  */

            work=byte1 & 0x0F;
            work>>=2;
            work+=0xD8;       /* 0b11011011; */
            in=work;
            in<<=8;

            byte1<<=6;
            byte2<<=2;
            byte2&=0x3C;      /* 0b00111100; */
            work=byte3;
            work>>=4;
            work&=0x03;       /* 0b00000011; */
            work|=byte1;
            work|=byte2;
            in+=work;

            work=byte3;
            work>>=2;
            work&=0x03;       /* 0b00000011; */
            work|=0xDC;       /* 0b110111xx; */
            in2=work;
            in2<<=8;

            byte3<<=6;
            byte4&=0x3F;      /* 0b00111111; */
            byte4|=byte3;
            in2+=byte4;
            inLen-=4;
            ++pIn;
#ifdef match_with_GBK
            if ((0xD800 == in && in2 < 0xDC80) ||
                (0xD840 == in && in2 < 0xDC80) ||
                (0xD880 == in && in2 < 0xDC80) ||
                (0xD8C0 == in && in2 < 0xDC80) ||
                (0xD900 == in && in2 < 0xDC80) ||
                (0xD940 == in && in2 < 0xDC80) ||
                (0xD980 == in && in2 < 0xDC80) ||
                (0xD9C0 == in && in2 < 0xDC80) ||
                (0xDA00 == in && in2 < 0xDC80) ||
                (0xDA40 == in && in2 < 0xDC80) ||
                (0xDA80 == in && in2 < 0xDC80) ||
                (0xDAC0 == in && in2 < 0xDC80) ||
                (0xDB00 == in && in2 < 0xDC80) ||
                (0xDB40 == in && in2 < 0xDC80) ||
                (0xDB80 == in && in2 < 0xDC80) ||
                (0xDBC0 == in && in2 < 0xDC80)) {
#else
              if ((0xD800 <= in  && in  <= 0xDBFF) &&
                  (0xDC00 <= in2 && in2 <= 0xDFFF)) {
#endif
              *pOut=subS;
              ++pOut;
              ++numS;
              continue;
            }
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-3;
            *numSub+=numS;
            return -1;
          }
        } else if (0xF5 <= byte1 /* && byte1 <= 0xFF */) { /* minic iconv() behavior */
          if (inLen < 4 ||
              (inLen >= 4 && byte1 == 0xF8 && pIn[1] < 0x90) ||
              pIn[1] < 0x80 || 0xBF < pIn[1] ||
              pIn[2] < 0x80 || 0xBF < pIn[2] ||
              pIn[3] < 0x80 || 0xBF < pIn[3] ) {
            if (inLen == 1)
              errno=EINVAL;  /* 22 */
            else if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else if (inLen == 3 && ((pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else if (inLen >= 4 && (byte1 == 0xF8 || (pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80 || (pIn[3] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */

            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          } else if ((pIn[1] == 0x80 || pIn[1] == 0x90 || pIn[1] == 0xA0 || pIn[1] == 0xB0) && 
                     pIn[2] < 0x82) {
            *pOut=subS;   /* Though returns replacement character, which iconv() does not return. */
            ++pOut;
            ++numS;
            pIn+=4;
            inLen-=4;
            continue;
          } else {
            *pOut=pSubD[0];   /* Though returns replacement character, which iconv() does not return. */
            ++pOut;
            *pOut=pSubD[1];
            ++pOut;
            ++numS;
            pIn+=4;
            inLen-=4;
            continue;
            /* iconv() returns 0 with strange 1 byte converted values */
          }

        } else { /* invalid sequence */
          errno=EILSEQ;  /* 116 */
          *outBytesLeft-=(pOut-*outBuf);
          *inBytesLeft=inLen;
          *outBuf=pOut;
          *inBuf=pIn;
          *numSub+=numS;
          return -1;
        }
      }
      /* end of UTF-8 to UCS-2 */
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else if (in < 0x100 && dmapU2S[in] != 0x0000) {
        if ((*pOut=dmapU2S[in]) == subS) {
          if (in != cd->srcSubS)
            ++numS;
        }
        ++pOut;
      } else {
        in<<=1;
        if (dmapU2M2[in] == 0x00) {     /* not found in dmapU2M2 */
          in*=1.5;
          if (dmapU2M3[in] == 0x00) {   /* not found in dmapU2M3*/
            *pOut=pSubD[0];
            ++pOut;
            *pOut=pSubD[1];
            ++pOut;
            ++numS;
            ++rc;
          } else {
            *pOut=dmapU2M3[in];
            ++pOut;
            *pOut=dmapU2M3[1+in];
            ++pOut;
            *pOut=dmapU2M3[2+in];
            ++pOut;
          }
        } else {
          *pOut=dmapU2M2[in];
          ++pOut;
          if (dmapU2M2[1+in] == 0x00) {
            if (*pOut == subS) {
              in>>=1;
              if (in != cd->srcSubS)
                ++numS;
            }
          } else {
            *pOut=dmapU2M2[1+in];
            ++pOut;
            if (memcmp(pOut-2, pSubD, 2) == 0) {
              in>>=1;
              if (in != cd->srcSubD) {
                ++numS;
                ++rc;
              }
            }
          }
        }
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_S2U) {
    /* use uchar * instead of UniChar to avoid memcpy */
    register uchar *    dmapD12U=(uchar *) (cd->cnv_dmap->dmapD12U);
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register int        offset;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register size_t     numS=0;
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
        ++pOut;
        *pOut=0x00;
        ++pOut;
        ++pIn;
        --inLen;
      } else {
        offset=*pIn;
        offset<<=1;
        *pOut=dmapD12U[offset];
        ++pOut;
        *pOut=dmapD12U[offset+1];
        ++pOut;
        if (dmapD12U[offset]   == 0x00) {
          if (dmapD12U[offset+1] == 0x1A) {
            if (*pIn != cd->srcSubS)
              ++numS;
          } else if (dmapD12U[offset+1] == 0x00) {
            pOut-=2;
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
        }
        ++pIn;
        --inLen;
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_S28) {
    /* use uchar * instead of UniChar to avoid memcpy */
    register uchar *    dmapD12U=(uchar *) (cd->cnv_dmap->dmapD12U);
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register int        offset;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register size_t     numS=0;
    register UniChar    in;     /* copy part of U28 */
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
        ++pOut;
        ++pIn;
        --inLen;
      } else {
        offset=*pIn;
        offset<<=1;
        in=dmapD12U[offset];
        in<<=8;
        in+=dmapD12U[offset+1];
        if ((in & 0xFF80) == 0x0000) {     /* U28: in & 0b1111111110000000 == 0x0000 */
          if (in == 0x000) {
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          *pOut=in;
          ++pOut;
        } else if ((in & 0xF800) == 0x0000) {     /* in & 0b1111100000000000 == 0x0000 */
          register uchar        byte;
          in>>=6;
          in&=0x001F;   /* 0b0000000000011111 */
          in|=0x00C0;   /* 0b0000000011000000 */
          *pOut=in;
          ++pOut;
          byte=dmapD12U[offset+1];
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;
        } else if ((in & 0xFC00) == 0xD800) {    /* There should not be no surrogate character in SBCS. */
          *pOut=0xEF;
          ++pOut;
          *pOut=0xBF;
          ++pOut;
          *pOut=0xBD;
          ++pOut;
        } else {
          register uchar        byte;
          register uchar        work;
          byte=dmapD12U[offset];
          byte>>=4;
          byte|=0xE0;   /* 0b11100000; */
          *pOut=byte;
          ++pOut;

          byte=dmapD12U[offset];
          byte<<=2;
          work=dmapD12U[offset+1];
          work>>=6;
          byte|=work;
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;

          byte=dmapD12U[offset+1];
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;
        }
        /* end of U28 */
        if (dmapD12U[offset]   == 0x00) {
          if (dmapD12U[offset+1] == 0x1A) {
            if (*pIn != cd->srcSubS)
              ++numS;
          }
        }
        ++pIn;
        --inLen;
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_U2S) {
    register uchar *    dmapU2S=cd->cnv_dmap->dmapU2S;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;   /* 22 */

        *inBytesLeft=inLen;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        return -1;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;
      } else {
        if ((*pOut=dmapU2S[in]) == 0x00) {
          *pOut=subS;
          ++numS;
          errno=EINVAL;  /* 22 */
        } else if (*pOut == subS) {
          if (in != cd->srcSubS)
            ++numS;
        }
      }
      ++pOut;
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return numS;

  } else if (cd->cnv_dmap->codingSchema == DMAP_T2S) {
    register uchar *    dmapU2S=cd->cnv_dmap->dmapU2S;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;   /* 22 */

        *inBytesLeft=inLen-1;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        ++numS;
        *numSub+=numS;
        return 0;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;

      } else if (0xD800 <= in && in <=  0xDFFF) {     /* 0xD800-0xDFFF, surrogate first and second values */
        if (0xDC00 <= in ) {
          errno=EINVAL;   /* 22 */
          *inBytesLeft=inLen-1;
          *outBytesLeft-=(pOut-*outBuf);
          *outBuf=pOut;
          *inBuf=pIn;
          return -1;
          
        } else if (inLen < 4) {
          errno=EINVAL;   /* 22 */
          *inBytesLeft=inLen-2;
          *outBytesLeft-=(pOut-*outBuf);
          *outBuf=pOut;
          *inBuf=pIn+2;
          return -1;
          
        } else {
          register uint32_t       in2;
          in2=pIn[2];
          in2<<=8;
          in2+=pIn[3];
          if (0xDC00 <= in2 && in2 <= 0xDFFF) {   /* second surrogate character =0xDC00 - 0xDFFF*/
            *pOut=subS;
            ++numS;
            pIn+=4;
          } else {
            errno=EINVAL;   /* 22 */
            *inBytesLeft=inLen-1;
            *outBytesLeft-=(pOut-*outBuf);
            *outBuf=pOut;
            *inBuf=pIn;
            return -1;
          }
        }
      } else {
        if ((*pOut=dmapU2S[in]) == 0x00) {
          *pOut=subS;
          ++numS;
          errno=EINVAL;  /* 22 */
        } else if (*pOut == subS) {
          if (in != cd->srcSubS)
            ++numS;
        }
      }
      ++pOut;
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_82S) {
    register uchar *    dmapU2S=cd->cnv_dmap->dmapU2S;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t         in;
      uint32_t                  in2;  /* The second surrogate value */
      if (pLastOutBuf < pOut)
        break;
      /* convert from UTF-8 to UCS-2 */
      if (*pIn == 0x00) {
        in=0x0000;
        ++pIn;
        --inLen;
      } else {                          /* 82U: */
        register uchar byte1=*pIn;
        if ((byte1 & 0x80) == 0x00) {         /* if (byte1 & 0b10000000 == 0b00000000) { */
          /* 1 bytes sequence:  0xxxxxxx => 00000000 0xxxxxxx*/
          in=byte1;
          ++pIn;
          --inLen;
        } else if ((byte1 & 0xE0) == 0xC0) {  /* (byte1 & 0b11100000 == 0b11000000) { */
          if (inLen < 2) {
            errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          if (byte1 == 0xC0 || byte1 == 0xC1) {  /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          /* 2 bytes sequence: 
             110yyyyy 10xxxxxx => 00000yyy yyxxxxxx */
          register uchar byte2;
          ++pIn;
          byte2=*pIn;
          if ((byte2 & 0xC0) == 0x80) {       /* byte2 & 0b11000000 == 0b10000000) { */
            register uchar work=byte1;
            work<<=6;
            byte2&=0x3F;      /* 0b00111111; */
            byte2|=work;
              
            byte1&=0x1F;      /* 0b00011111; */
            byte1>>=2;
            in=byte1;
            in<<=8;
            in+=byte2;
            inLen-=2;
            ++pIn;
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            *numSub+=numS;
            return -1;
          }
        } else if ((byte1 & 0xF0) == 0xE0) {    /* byte1 & 0b11110000 == 0b11100000 */
          /* 3 bytes sequence: 
             1110zzzz 10yyyyyy 10xxxxxx => zzzzyyyy yyxxxxxx */
          register uchar byte2;
          register uchar byte3;
          if (inLen < 3) {
            if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          if ((byte2 & 0xC0) != 0x80 ||
              (byte3 & 0xC0) != 0x80 ||
              (byte1 == 0xE0 && byte2 < 0xA0)) { /* invalid sequence, only 0xA0-0xBF allowed after 0xE0 */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-2;
            *numSub+=numS;
            return -1;
          }
          {
            register uchar work=byte2;
            work<<=6;
            byte3&=0x3F;      /* 0b00111111; */
            byte3|=work;
              
            byte2&=0x3F;      /* 0b00111111; */
            byte2>>=2;

            byte1<<=4;
            in=byte1 | byte2;;
            in<<=8;
            in+=byte3;
            inLen-=3;
            ++pIn;
          }
        } else if ((0xF0 <= byte1 && byte1 <= 0xF4) ||    /* (bytes1 & 11111000) == 0x1110000 */
                   ((byte1&=0xF7) && 0xF0 <= byte1 && byte1 <= 0xF4)) { /* minic iconv() behavior */
          /* 4 bytes sequence
             11110uuu 10uuzzzz 10yyyyyy 10xxxxxx => 110110ww wwzzzzyy 110111yy yyxxxxxx
             where uuuuu = wwww + 1 */
          register uchar byte2;
          register uchar byte3;
          register uchar byte4;
          if (inLen < 4) {
            if ((inLen >= 2 && (pIn[1]  & 0xC0) != 0x80) ||
                (inLen >= 3 && (pIn[2]  & 0xC0) != 0x80) ||
                (cd->toCcsid == 13488) )
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          ++pIn;
          byte4=*pIn;
          if ((byte2 & 0xC0) == 0x80 &&         /* byte2 & 0b11000000 == 0b10000000 */
              (byte3 & 0xC0) == 0x80 &&         /* byte3 & 0b11000000 == 0b10000000 */
              (byte4 & 0xC0) == 0x80) {         /* byte4 & 0b11000000 == 0b10000000 */
            register uchar work=byte2;
            if (byte1 == 0xF0 && byte2 < 0x90) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
              /* iconv() returns 0 for 0xF4908080 and convert to 0x00 
            } else if (byte1 == 0xF4 && byte2 > 0x8F) {
              errno=EINVAL;
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
              */
            }

            work&=0x30;       /* 0b00110000; */
            work>>=4;
            byte1&=0x07;      /* 0b00000111; */
            byte1<<=2;
            byte1+=work;      /* uuuuu */
            --byte1;          /* wwww  */

            work=byte1 & 0x0F;
            work>>=2;
            work+=0xD8;       /* 0b11011011; */
            in=work;
            in<<=8;

            byte1<<=6;
            byte2<<=2;
            byte2&=0x3C;      /* 0b00111100; */
            work=byte3;
            work>>=4;
            work&=0x03;       /* 0b00000011; */
            work|=byte1;
            work|=byte2;
            in+=work;

            work=byte3;
            work>>=2;
            work&=0x03;       /* 0b00000011; */
            work|=0xDC;       /* 0b110111xx; */
            in2=work;
            in2<<=8;

            byte3<<=6;
            byte4&=0x3F;      /* 0b00111111; */
            byte4|=byte3;
            in2+=byte4;
            inLen-=4;
            ++pIn;
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-3;
            *numSub+=numS;
            return -1;
          }
        } else if ((byte1 & 0xF0) == 0xF0) { /* minic iconv() behavior */
          if (inLen < 4 ||
              pIn[1] < 0x80 || 0xBF < pIn[1] ||
              pIn[2] < 0x80 || 0xBF < pIn[2] ||
              pIn[3] < 0x80 || 0xBF < pIn[3] ) {
            if (inLen == 1)
              errno=EINVAL;  /* 22 */
            else if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else if (inLen == 3 && ((pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else if (inLen >= 4 && ((pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80 || (pIn[3] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */

            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          } else {
            *pOut=subS;   /* Though returns replacement character, which iconv() does not return. */
            ++pOut;
            ++numS;
            pIn+=4;
            inLen-=4;
            /* UTF-8_IBM-850 0xF0908080 : converted value does not match, iconv=0x00, dmap=0x7F
               UTF-8_IBM-850 0xF0908081 : converted value does not match, iconv=0x01, dmap=0x7F
               UTF-8_IBM-850 0xF0908082 : converted value does not match, iconv=0x02, dmap=0x7F
               UTF-8_IBM-850 0xF0908083 : converted value does not match, iconv=0x03, dmap=0x7F 
               ....
               UTF-8_IBM-850 0xF09081BE : converted value does not match, iconv=0x7E, dmap=0x7F
               UTF-8_IBM-850 0xF09081BF : converted value does not match, iconv=0x1C, dmap=0x7F
               UTF-8_IBM-850 0xF09082A0 : converted value does not match, iconv=0xFF, dmap=0x7F
               UTF-8_IBM-850 0xF09082A1 : converted value does not match, iconv=0xAD, dmap=0x7F
               ....
            */
            continue;
            /* iconv() returns 0 with strange 1 byte converted values */
          }

        } else { /* invalid sequence */
          errno=EILSEQ;  /* 116 */
          *outBytesLeft-=(pOut-*outBuf);
          *inBytesLeft=inLen;
          *outBuf=pOut;
          *inBuf=pIn;
          *numSub+=numS;
          return -1;
        }
      }
      /* end of UTF-8 to UCS-2 */
      if (in == 0x0000) {
        *pOut=0x00;
      } else {
        if ((*pOut=dmapU2S[in]) == 0x00) {
          *pOut=subS;
          ++numS;
          errno=EINVAL;  /* 22 */
        } else if (*pOut == subS) {
          if (in != cd->srcSubS) {
            ++numS;
          }
        }
      }
      ++pOut;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_D2U) {
    /* use uchar * instead of UniChar to avoid memcpy */
    register uchar *    dmapD12U=(uchar *) (cd->cnv_dmap->dmapD12U);
    register uchar *    dmapD22U=(uchar *) (cd->cnv_dmap->dmapD22U);
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register int        offset;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register size_t   numS=0;
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
        ++pOut;
        *pOut=0x00;
        ++pOut;
        ++pIn;
        --inLen;
      } else {
        offset=*pIn;
        offset<<=1;
        if (dmapD12U[offset]   == 0x00 &&
            dmapD12U[offset+1] == 0x00) {         /* DBCS */
          if (inLen < 2) {
            if (*pIn == 0x80 || *pIn == 0xFF ||
                (cd->fromCcsid == 943 && (*pIn == 0x85 || *pIn == 0x86 || *pIn == 0xA0 || *pIn == 0xEB || *pIn == 0xEC || *pIn == 0xEF || *pIn == 0xFD || *pIn == 0xFE)) ||
                (cd->fromCcsid == 932 && (*pIn == 0x85 || *pIn == 0x86 || *pIn == 0x87 || *pIn == 0xEB || *pIn == 0xEC || *pIn == 0xED || *pIn == 0xEE || *pIn == 0xEF)) ||
                (cd->fromCcsid == 1381 && ((0x85 <= *pIn && *pIn <= 0x8B) || (0xAA <= *pIn && *pIn <= 0xAF) || (0xF8 <= *pIn && *pIn <= 0xFE))))
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            return -1;
          }
          offset-=0x100;
          ++pIn;
          offset<<=8;
          offset+=(*pIn * 2);
          if (dmapD22U[offset] == 0x00 &&
              dmapD22U[offset+1] == 0x00) {
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            return -1;
          }
          *pOut=dmapD22U[offset];
          ++pOut;
          *pOut=dmapD22U[offset+1];
          ++pOut;
          if (dmapD22U[offset] == 0xFF &&
              dmapD22U[offset+1] == 0xFD) {
            if (pIn[-1] * 0x100 + pIn[0] != cd->srcSubD)
              ++numS;
          }
          ++pIn;
          inLen-=2;
        } else {  /* SBCS */
          *pOut=dmapD12U[offset];
          ++pOut;
          *pOut=dmapD12U[offset+1];
          ++pOut;
          if (dmapD12U[offset]   == 0x00 &&
              dmapD12U[offset+1] == 0x1A) {
            if (*pIn != cd->srcSubS)
              ++numS;
          }
          ++pIn;
          --inLen;
        }
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_D28) {
    /* use uchar * instead of UniChar to avoid memcpy */
    register uchar *    dmapD12U=(uchar *) (cd->cnv_dmap->dmapD12U);
    register uchar *    dmapD22U=(uchar *) (cd->cnv_dmap->dmapD22U);
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register int        offset;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register size_t     numS=0;
    register UniChar    in;     /* copy part of U28 */
    register UniChar    ucs2;
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
        ++pOut;
        ++pIn;
        --inLen;
      } else {
        offset=*pIn;
        offset<<=1;
        if (dmapD12U[offset]   == 0x00 &&
            dmapD12U[offset+1] == 0x00) {         /* DBCS */
          if (inLen < 2) {
            if (*pIn == 0x80 || *pIn == 0xFF ||
                (cd->fromCcsid == 943 && (*pIn == 0x85 || *pIn == 0x86 || *pIn == 0xA0 || *pIn == 0xEB || *pIn == 0xEC || *pIn == 0xEF || *pIn == 0xFD || *pIn == 0xFE)) ||
                (cd->fromCcsid == 932 && (*pIn == 0x85 || *pIn == 0x86 || *pIn == 0x87 || *pIn == 0xEB || *pIn == 0xEC || *pIn == 0xED || *pIn == 0xEE || *pIn == 0xEF)) ||
                (cd->fromCcsid == 1381 && ((0x85 <= *pIn && *pIn <= 0x8B) || (0xAA <= *pIn && *pIn <= 0xAF) || (0xF8 <= *pIn && *pIn <= 0xFE))))
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            return -1;
          }
          offset-=0x100;
          ++pIn;
          offset<<=8;
          offset+=(*pIn * 2);
          if (dmapD22U[offset] == 0x00 &&
              dmapD22U[offset+1] == 0x00) {
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            return -1;
          }
          in=dmapD22U[offset];
          in<<=8;
          in+=dmapD22U[offset+1];
          ucs2=in;
          if (dmapD22U[offset] == 0xFF &&
              dmapD22U[offset+1] == 0xFD) {
            if (in != cd->srcSubD)
              ++numS;
          }
          ++pIn;
          inLen-=2;
        } else {  /* SBCS */
          in=dmapD12U[offset];
          in<<=8;
          in+=dmapD12U[offset+1];
          ucs2=in;
          if (dmapD12U[offset]   == 0x00 &&
              dmapD12U[offset+1] == 0x1A) {
            if (in != cd->srcSubS)
              ++numS;
          }
          ++pIn;
          --inLen;
        }
        if ((in & 0xFF80) == 0x0000) {     /* U28: in & 0b1111111110000000 == 0x0000 */
          *pOut=in;
          ++pOut;
        } else if ((in & 0xF800) == 0x0000) {     /* in & 0b1111100000000000 == 0x0000 */
          register uchar        byte;
          in>>=6;
          in&=0x001F;   /* 0b0000000000011111 */
          in|=0x00C0;   /* 0b0000000011000000 */
          *pOut=in;
          ++pOut;
          byte=ucs2;    /* dmapD12U[offset+1]; */
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;
        } else if ((in & 0xFC00) == 0xD800) {    /* There should not be no surrogate character in SBCS. */
          *pOut=0xEF;
          ++pOut;
          *pOut=0xBF;
          ++pOut;
          *pOut=0xBD;
          ++pOut;
        } else {
          register uchar        byte;
          register uchar        work;
          byte=(ucs2>>8);    /* dmapD12U[offset]; */
          byte>>=4;
          byte|=0xE0;   /* 0b11100000; */
          *pOut=byte;
          ++pOut;

          byte=(ucs2>>8);    /* dmapD12U[offset]; */
          byte<<=2;
          work=ucs2;    /* dmapD12U[offset+1]; */
          work>>=6;
          byte|=work;
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;

          byte=ucs2;    /* dmapD12U[offset+1]; */
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;
        }
        /* end of U28 */
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_U2D) {
    register uchar *    dmapU2D=cd->cnv_dmap->dmapU2D;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register char *     pSubD=(char *) &(cd->subD);
    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;   /* 22 */

        *inBytesLeft=inLen;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        return -1;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else {
        in<<=1;
        *pOut=dmapU2D[in];
        ++pOut;
        if (dmapU2D[in+1] == 0x00) {   /* SBCS */
          if (*pOut == subS) {
            if (in != cd->srcSubS)
              ++numS;
          }
        } else {
          *pOut=dmapU2D[in+1];
          ++pOut;
          if (dmapU2D[in]   == pSubD[0] &&
              dmapU2D[in+1] == pSubD[1]) {
            in>>=1;
            if (in != cd->srcSubD)
              ++numS;
          }
        }
      }
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return numS;        /* to minic iconv() behavior */

  } else if (cd->cnv_dmap->codingSchema == DMAP_T2D) {
    register uchar *    dmapU2D=cd->cnv_dmap->dmapU2D;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register char *     pSubD=(char *) &(cd->subD);
    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;  /* 22 */
        *inBytesLeft=inLen-1;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        ++numS;
        *numSub+=numS;
        return 0;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else if (0xD800 <= in && in <= 0xDBFF) { /* first byte of surrogate */
        errno=EINVAL;   /* 22 */
        *inBytesLeft=inLen-2;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn+2;
        ++numS;
        *numSub+=numS;
        return -1;
        
      } else if (0xDC00 <= in && in <= 0xDFFF) { /* second byte of surrogate */
        errno=EINVAL;   /* 22 */
        *inBytesLeft=inLen-1;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        ++numS;
        *numSub+=numS;
        return -1;
        
      } else {
        in<<=1;
        *pOut=dmapU2D[in];
        ++pOut;
        if (dmapU2D[in+1] == 0x00) {   /* SBCS */
          if (*pOut == subS) {
            if (in != cd->srcSubS)
              ++numS;
          }
        } else {
          *pOut=dmapU2D[in+1];
          ++pOut;
          if (dmapU2D[in]   == pSubD[0] &&
              dmapU2D[in+1] == pSubD[1]) {
            in>>=1;
            if (in != cd->srcSubD)
              ++numS;
          }
        }
      }
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;        /* to minic iconv() behavior */

  } else if (cd->cnv_dmap->codingSchema == DMAP_82D) {
    register uchar *    dmapU2D=cd->cnv_dmap->dmapU2D;
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register char       subS=cd->subS;
    register char *     pSubD=(char *) &(cd->subD);
    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t in;
      uint32_t          in2;
      if (pLastOutBuf < pOut)
        break;
      /* convert from UTF-8 to UCS-2 */
      if (*pIn == 0x00) {
        in=0x0000;
        ++pIn;
        --inLen;
      } else {                          /* 82U: */
        register uchar byte1=*pIn;
        if ((byte1 & 0x80) == 0x00) {         /* if (byte1 & 0b10000000 == 0b00000000) { */
          /* 1 bytes sequence:  0xxxxxxx => 00000000 0xxxxxxx*/
          in=byte1;
          ++pIn;
          --inLen;
        } else if ((byte1 & 0xE0) == 0xC0) {  /* (byte1 & 0b11100000 == 0b11000000) { */
          if (inLen < 2) {
            errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          if (byte1 == 0xC0 || byte1 == 0xC1) {  /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          /* 2 bytes sequence: 
             110yyyyy 10xxxxxx => 00000yyy yyxxxxxx */
          register uchar byte2;
          ++pIn;
          byte2=*pIn;
          if ((byte2 & 0xC0) == 0x80) {       /* byte2 & 0b11000000 == 0b10000000) { */
            register uchar work=byte1;
            work<<=6;
            byte2&=0x3F;      /* 0b00111111; */
            byte2|=work;
              
            byte1&=0x1F;      /* 0b00011111; */
            byte1>>=2;
            in=byte1;
            in<<=8;
            in+=byte2;
            inLen-=2;
            ++pIn;
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            *numSub+=numS;
            return -1;
          }
        } else if ((byte1 & 0xF0) == 0xE0) {    /* byte1 & 0b11110000 == 0b11100000 */
          /* 3 bytes sequence: 
             1110zzzz 10yyyyyy 10xxxxxx => zzzzyyyy yyxxxxxx */
          register uchar byte2;
          register uchar byte3;
          if (inLen < 3) {
            if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          if ((byte2 & 0xC0) != 0x80 ||
              (byte3 & 0xC0) != 0x80 ||
              (byte1 == 0xE0 && byte2 < 0xA0)) { /* invalid sequence, only 0xA0-0xBF allowed after 0xE0 */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-2;
            *numSub+=numS;
            return -1;
          }
          {
            register uchar work=byte2;
            work<<=6;
            byte3&=0x3F;      /* 0b00111111; */
            byte3|=work;
              
            byte2&=0x3F;      /* 0b00111111; */
            byte2>>=2;

            byte1<<=4;
            in=byte1 | byte2;;
            in<<=8;
            in+=byte3;
            inLen-=3;
            ++pIn;
          }
        } else if ((0xF0 <= byte1 && byte1 <= 0xF4)) {    /* (bytes1 & 11111000) == 0x1110000 */
          /* 4 bytes sequence
             11110uuu 10uuzzzz 10yyyyyy 10xxxxxx => 110110ww wwzzzzyy 110111yy yyxxxxxx
             where uuuuu = wwww + 1 */
          register uchar byte2;
          register uchar byte3;
          register uchar byte4;
          if (inLen < 4) {
            if ((inLen >= 2 && (pIn[1]  & 0xC0) != 0x80) ||
                (inLen >= 3 && (pIn[2]  & 0xC0) != 0x80) ||
                (cd->toCcsid == 13488) )
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          ++pIn;
          byte4=*pIn;
          if ((byte2 & 0xC0) == 0x80 &&         /* byte2 & 0b11000000 == 0b10000000 */
              (byte3 & 0xC0) == 0x80 &&         /* byte3 & 0b11000000 == 0b10000000 */
              (byte4 & 0xC0) == 0x80) {         /* byte4 & 0b11000000 == 0b10000000 */
            register uchar work=byte2;
            if (byte1 == 0xF0 && byte2 < 0x90) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
              /* iconv() returns 0 for 0xF4908080 and convert to 0x00 
            } else if (byte1 == 0xF4 && byte2 > 0x8F) {
              errno=EINVAL;
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
              */
            }

            work&=0x30;       /* 0b00110000; */
            work>>=4;
            byte1&=0x07;      /* 0b00000111; */
            byte1<<=2;
            byte1+=work;      /* uuuuu */
            --byte1;          /* wwww  */

            work=byte1 & 0x0F;
            work>>=2;
            work+=0xD8;       /* 0b11011011; */
            in=work;
            in<<=8;

            byte1<<=6;
            byte2<<=2;
            byte2&=0x3C;      /* 0b00111100; */
            work=byte3;
            work>>=4;
            work&=0x03;       /* 0b00000011; */
            work|=byte1;
            work|=byte2;
            in+=work;

            work=byte3;
            work>>=2;
            work&=0x03;       /* 0b00000011; */
            work|=0xDC;       /* 0b110111xx; */
            in2=work;
            in2<<=8;

            byte3<<=6;
            byte4&=0x3F;      /* 0b00111111; */
            byte4|=byte3;
            in2+=byte4;
            inLen-=4;
            ++pIn;
#ifdef match_with_GBK
            if ((0xD800 == in && in2 < 0xDC80) ||
                (0xD840 == in && in2 < 0xDC80) ||
                (0xD880 == in && in2 < 0xDC80) ||
                (0xD8C0 == in && in2 < 0xDC80) ||
                (0xD900 == in && in2 < 0xDC80) ||
                (0xD940 == in && in2 < 0xDC80) ||
                (0xD980 == in && in2 < 0xDC80) ||
                (0xD9C0 == in && in2 < 0xDC80) ||
                (0xDA00 == in && in2 < 0xDC80) ||
                (0xDA40 == in && in2 < 0xDC80) ||
                (0xDA80 == in && in2 < 0xDC80) ||
                (0xDAC0 == in && in2 < 0xDC80) ||
                (0xDB00 == in && in2 < 0xDC80) ||
                (0xDB40 == in && in2 < 0xDC80) ||
                (0xDB80 == in && in2 < 0xDC80) ||
                (0xDBC0 == in && in2 < 0xDC80)) {
#else
              if ((0xD800 <= in  && in  <= 0xDBFF) &&
                  (0xDC00 <= in2 && in2 <= 0xDFFF)) {
#endif
              *pOut=subS;
              ++pOut;
              ++numS;
              continue;
            }
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-3;
            *numSub+=numS;
            return -1;
          }
        } else if (0xF5 <= byte1 /* && byte1 <= 0xFF */) { /* minic iconv() behavior */
          if (inLen < 4 ||
              (inLen >= 4 && byte1 == 0xF8 && pIn[1] < 0x90) ||
              pIn[1] < 0x80 || 0xBF < pIn[1] ||
              pIn[2] < 0x80 || 0xBF < pIn[2] ||
              pIn[3] < 0x80 || 0xBF < pIn[3] ) {
            if (inLen == 1)
              errno=EINVAL;  /* 22 */
            else if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else if (inLen == 3 && ((pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else if (inLen >= 4 && (byte1 == 0xF8 || (pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80 || (pIn[3] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */

            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          } else if ((pIn[1] == 0x80 || pIn[1] == 0x90 || pIn[1] == 0xA0 || pIn[1] == 0xB0) && 
                     pIn[2] < 0x82) {
            *pOut=subS;   /* Though returns replacement character, which iconv() does not return. */
            ++pOut;
            ++numS;
            pIn+=4;
            inLen-=4;
            continue;
          } else {
            *pOut=pSubD[0];   /* Though returns replacement character, which iconv() does not return. */
            ++pOut;
            *pOut=pSubD[1];
            ++pOut;
            ++numS;
            pIn+=4;
            inLen-=4;
            continue;
            /* iconv() returns 0 with strange 1 byte converted values */
          }

        } else { /* invalid sequence */
          errno=EILSEQ;  /* 116 */
          *outBytesLeft-=(pOut-*outBuf);
          *inBytesLeft=inLen;
          *outBuf=pOut;
          *inBuf=pIn;
          *numSub+=numS;
          return -1;
        }
      }
      /* end of UTF-8 to UCS-2 */
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else {
        in<<=1;
        *pOut=dmapU2D[in];
        ++pOut;
        if (dmapU2D[in+1] == 0x00) {   /* SBCS */
          if (dmapU2D[in] == subS) {
            in>>=1;
            if (in != cd->srcSubS)
              ++numS;
          }
        } else {
          *pOut=dmapU2D[in+1];
          ++pOut;
          if (dmapU2D[in]   == pSubD[0] &&
              dmapU2D[in+1] == pSubD[1]) {
            in>>=1;
            if (in != cd->srcSubD)
              ++numS;
          }
        }
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_82U) {
    /* See http://unicode.org/versions/corrigendum1.html */
    /* convert from UTF-8 to UTF-16 can cover all conversion from UTF-8 to UCS-2 */
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    register size_t     numS=0;
    while (0 < inLen) {
      if (pLastOutBuf < pOut)
        break;
      if (*pIn == 0x00) {
        *pOut=0x00;
        ++pOut;
        *pOut=0x00;
        ++pOut;
        ++pIn;
        --inLen;
      } else {                          /* 82U: */
        register uchar byte1=*pIn;
        if ((byte1 & 0x80) == 0x00) {         /* if (byte1 & 0b10000000 == 0b00000000) { */
          /* 1 bytes sequence:  0xxxxxxx => 00000000 0xxxxxxx*/
          *pOut=0x00;
          ++pOut;
          *pOut=byte1;
          ++pOut;
          ++pIn;
          --inLen;
        } else if ((byte1 & 0xE0) == 0xC0) {  /* (byte1 & 0b11100000 == 0b11000000) { */
          if (inLen < 2) {
            errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          if (byte1 == 0xC0 || byte1 == 0xC1) {  /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          /* 2 bytes sequence: 
             110yyyyy 10xxxxxx => 00000yyy yyxxxxxx */
          register uchar byte2;
          ++pIn;
          byte2=*pIn;
          if ((byte2 & 0xC0) == 0x80) {       /* byte2 & 0b11000000 == 0b10000000) { */
            register uchar work=byte1;
            work<<=6;
            byte2&=0x3F;      /* 0b00111111; */
            byte2|=work;
              
            byte1&=0x1F;      /* 0b00011111; */
            byte1>>=2;
            *pOut=byte1;
            ++pOut;
            *pOut=byte2;
            ++pOut;
            inLen-=2;
            ++pIn;
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-1;
            *numSub+=numS;
            return -1;
          }
        } else if ((byte1 & 0xF0) == 0xE0) {    /* byte1 & 0b11110000 == 0b11100000 */
          /* 3 bytes sequence: 
             1110zzzz 10yyyyyy 10xxxxxx => zzzzyyyy yyxxxxxx */
          register uchar byte2;
          register uchar byte3;
          if (inLen < 3) {
            if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          if ((byte2 & 0xC0) != 0x80 ||
              (byte3 & 0xC0) != 0x80 ||
              (byte1 == 0xE0 && byte2 < 0xA0)) { /* invalid sequence, only 0xA0-0xBF allowed after 0xE0 */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-2;
            *numSub+=numS;
            return -1;
          }
          {
            register uchar work=byte2;
            work<<=6;
            byte3&=0x3F;      /* 0b00111111; */
            byte3|=work;
              
            byte2&=0x3F;      /* 0b00111111; */
            byte2>>=2;

            byte1<<=4;
            *pOut=byte1 | byte2;;
            ++pOut;
            *pOut=byte3;
            ++pOut;
            inLen-=3;
            ++pIn;
          }
        } else if ((0xF0 <= byte1 && byte1 <= 0xF4) ||    /* (bytes1 & 11111000) == 0x1110000 */
                   ((byte1&=0xF7) && 0xF0 <= byte1 && byte1 <= 0xF4)) { /* minic iconv() behavior */
          /* 4 bytes sequence
             11110uuu 10uuzzzz 10yyyyyy 10xxxxxx => 110110ww wwzzzzyy 110111yy yyxxxxxx
             where uuuuu = wwww + 1 */
          register uchar byte2;
          register uchar byte3;
          register uchar byte4;
          if (inLen < 4 || cd->toCcsid == 13488) {
            if ((inLen >= 2 && (pIn[1]  & 0xC0) != 0x80) ||
                (inLen >= 3 && (pIn[2]  & 0xC0) != 0x80) ||
                (cd->toCcsid == 13488) )
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn;
            *numSub+=numS;
            return -1;
          }
          ++pIn;
          byte2=*pIn;
          ++pIn;
          byte3=*pIn;
          ++pIn;
          byte4=*pIn;
          if ((byte2 & 0xC0) == 0x80 &&         /* byte2 & 0b11000000 == 0b10000000 */
              (byte3 & 0xC0) == 0x80 &&         /* byte3 & 0b11000000 == 0b10000000 */
              (byte4 & 0xC0) == 0x80) {         /* byte4 & 0b11000000 == 0b10000000 */
            register uchar work=byte2;
            if (byte1 == 0xF0 && byte2 < 0x90) {
              errno=EILSEQ;  /* 116 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
            } else if (byte1 == 0xF4 && byte2 > 0x8F) {
              errno=EINVAL;  /* 22 */
              *outBytesLeft-=(pOut-*outBuf);
              *inBytesLeft=inLen;
              *outBuf=pOut;
              *inBuf=pIn-3;
              *numSub+=numS;
              return -1;
            }

            work&=0x30;       /* 0b00110000; */
            work>>=4;
            byte1&=0x07;      /* 0b00000111; */
            byte1<<=2;
            byte1+=work;      /* uuuuu */
            --byte1;          /* wwww  */

            work=byte1 & 0x0F;
            work>>=2;
            work+=0xD8;       /* 0b11011011; */
            *pOut=work;
            ++pOut;

            byte1<<=6;
            byte2<<=2;
            byte2&=0x3C;      /* 0b00111100; */
            work=byte3;
            work>>=4;
            work&=0x03;       /* 0b00000011; */
            work|=byte1;
            work|=byte2;
            *pOut=work;
            ++pOut;

            work=byte3;
            work>>=2;
            work&=0x03;       /* 0b00000011; */
            work|=0xDC;       /* 0b110111xx; */
            *pOut=work;
            ++pOut;

            byte3<<=6;
            byte4&=0x3F;      /* 0b00111111; */
            byte4|=byte3;
            *pOut=byte4;
            ++pOut;
            inLen-=4;
            ++pIn;
          } else { /* invalid sequence */
            errno=EILSEQ;  /* 116 */
            *outBytesLeft-=(pOut-*outBuf);
            *inBytesLeft=inLen;
            *outBuf=pOut;
            *inBuf=pIn-3;
            *numSub+=numS;
            return -1;
          }
        } else if ((byte1 & 0xF0) == 0xF0) {
          if (cd->toCcsid == 13488) {
            errno=EILSEQ;  /* 116 */
          } else {
            if (inLen == 1)
              errno=EINVAL;  /* 22 */
            else if (inLen == 2 && (pIn[1]  & 0xC0) != 0x80)
              errno=EILSEQ;  /* 116 */
            else if (inLen == 3 && ((pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else if (inLen >= 4 && ((pIn[1]  & 0xC0) != 0x80 || (pIn[2] & 0xC0) != 0x80 || (pIn[3] & 0xC0) != 0x80))
              errno=EILSEQ;  /* 116 */
            else
              errno=EINVAL;  /* 22 */
          }
          *outBytesLeft-=(pOut-*outBuf);
          *inBytesLeft=inLen;
          *outBuf=pOut;
          *inBuf=pIn;
          *numSub+=numS;
          return -1;

        } else { /* invalid sequence */
          errno=EILSEQ;  /* 116 */
          *outBytesLeft-=(pOut-*outBuf);
          *inBytesLeft=inLen;
          *outBuf=pOut;
          *inBuf=pIn;
          *numSub+=numS;
          return -1;
        }
      }
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    *numSub+=numS;
    return 0;
  } else if (cd->cnv_dmap->codingSchema == DMAP_U28) {
    /* See http://unicode.org/versions/corrigendum1.html */
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    //    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;   /* 22 */
        *inBytesLeft=inLen;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        return -1;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else if ((in & 0xFF80) == 0x0000) {     /* U28: in & 0b1111111110000000 == 0x0000 */
        *pOut=in;
        ++pOut;
      } else if ((in & 0xF800) == 0x0000) {     /* in & 0b1111100000000000 == 0x0000 */
        register uchar        byte;
        in>>=6;
        in&=0x001F;   /* 0b0000000000011111 */
        in|=0x00C0;   /* 0b0000000011000000 */
        *pOut=in;
        ++pOut;
        byte=pIn[1];
        byte&=0x3F;   /* 0b00111111; */
        byte|=0x80;   /* 0b10000000; */
        *pOut=byte;
        ++pOut;
      } else {
        register uchar        byte;
        register uchar        work;
        byte=pIn[0];
        byte>>=4;
        byte|=0xE0;   /* 0b11100000; */
        *pOut=byte;
        ++pOut;

        byte=pIn[0];
        byte<<=2;
        work=pIn[1];
        work>>=6;
        byte|=work;
        byte&=0x3F;   /* 0b00111111; */
        byte|=0x80;   /* 0b10000000; */
        *pOut=byte;
        ++pOut;

        byte=pIn[1];
        byte&=0x3F;   /* 0b00111111; */
        byte|=0x80;   /* 0b10000000; */
        *pOut=byte;
        ++pOut;
      }
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    //    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_T28) {   /* UTF-16_UTF-8 */
    /* See http://unicode.org/versions/corrigendum1.html */
    register int        inLen=*inBytesLeft;
    register char *     pOut=*outBuf;
    register char *     pIn=*inBuf;
    register char *     pLastOutBuf = *outBuf + *outBytesLeft - 1;
    //    register size_t     numS=0;
    while (0 < inLen) {
      register uint32_t       in;
      if (inLen == 1) {
        errno=EINVAL;   /* 22 */
        *inBytesLeft=0;
        *outBytesLeft-=(pOut-*outBuf);
        *outBuf=pOut;
        *inBuf=pIn;
        return 0;
      }
      if (pLastOutBuf < pOut)
        break;
      in=pIn[0];
      in<<=8;
      in+=pIn[1];
      if (in == 0x0000) {
        *pOut=0x00;
        ++pOut;
      } else if ((in & 0xFF80) == 0x0000) {     /* U28: in & 0b1111111110000000 == 0x0000 */
        *pOut=in;
        ++pOut;
      } else if ((in & 0xF800) == 0x0000) {     /* in & 0b1111100000000000 == 0x0000 */
        register uchar        byte;
        in>>=6;
        in&=0x001F;   /* 0b0000000000011111 */
        in|=0x00C0;   /* 0b0000000011000000 */
        *pOut=in;
        ++pOut;
        byte=pIn[1];
        byte&=0x3F;   /* 0b00111111; */
        byte|=0x80;   /* 0b10000000; */
        *pOut=byte;
        ++pOut;
      } else if ((in & 0xFC00) == 0xD800) {     /* in & 0b1111110000000000 == 0b1101100000000000, first surrogate character */
        if (0xDC00 <= in ) {
          errno=EINVAL;   /* 22 */
          *inBytesLeft=inLen-1;
          *outBytesLeft-=(pOut-*outBuf);
          *outBuf=pOut;
          *inBuf=pIn;
          return -1;
          
        } else if (inLen < 4) {
          errno=EINVAL;   /* 22 */
          *inBytesLeft=inLen-2;
          *outBytesLeft-=(pOut-*outBuf);
          *outBuf=pOut;
          *inBuf=pIn+2;
          return -1;
          
        } else if ((pIn[2] & 0xFC) != 0xDC) {     /* pIn[2] & 0b11111100 == 0b11011100, second surrogate character */
          errno=EINVAL;   /* 22 */
          *inBytesLeft=inLen-2;
          *outBytesLeft-=(pOut-*outBuf);
          *outBuf=pOut;
          *inBuf=pIn+2;
          return -1;
          
        } else {
          register uchar        byte;
          register uchar        work;
          in>>=6;
          in&=0x000F;   /* 0b0000000000001111 */
          byte=in;      /* wwww  */
          ++byte;       /* uuuuu */
          work=byte;    /* save uuuuu */
          byte>>=2;
          byte|=0xF0;   /* 0b11110000; */
          *pOut=byte;
          ++pOut;

          byte=work;
          byte&=0x03;   /* 0b00000011; */
          byte<<=4;
          byte|=0x80;   /* 0b10000000; */
          work=pIn[1];
          work&=0x3C;   /* 0b00111100; */
          work>>=2;
          byte|=work;
          *pOut=byte;
          ++pOut;

          byte=pIn[1];
          byte&=0x03;   /* 0b00000011; */
          byte<<=4;
          byte|=0x80;   /* 0b10000000; */
          work=pIn[2];
          work&=0x03;   /* 0b00000011; */
          work<<=2;
          byte|=work;
          work=pIn[3];
          work>>=6;
          byte|=work;
          *pOut=byte;
          ++pOut;

          byte=pIn[3];
          byte&=0x3F;   /* 0b00111111; */
          byte|=0x80;   /* 0b10000000; */
          *pOut=byte;
          ++pOut;
          pIn+=2;
          inLen-=2;
        }
      } else if ((in & 0xFC00) == 0xDC00) {     /* in & 0b11111100 == 0b11011100, second surrogate character */
          errno=EINVAL;   /* 22 */
          *inBytesLeft=inLen-1;
          *outBytesLeft-=(pOut-*outBuf);
          *outBuf=pOut;
          *inBuf=pIn;
          return -1;
          
      } else {
        register uchar        byte;
        register uchar        work;
        byte=pIn[0];
        byte>>=4;
        byte|=0xE0;   /* 0b11100000; */
        *pOut=byte;
        ++pOut;

        byte=pIn[0];
        byte<<=2;
        work=pIn[1];
        work>>=6;
        byte|=work;
        byte&=0x3F;   /* 0b00111111; */
        byte|=0x80;   /* 0b10000000; */
        *pOut=byte;
        ++pOut;

        byte=pIn[1];
        byte&=0x3F;   /* 0b00111111; */
        byte|=0x80;   /* 0b10000000; */
        *pOut=byte;
        ++pOut;
      }
      pIn+=2;
      inLen-=2;
    }
    *outBytesLeft-=(pOut-*outBuf);
    *inBytesLeft=inLen;
    *outBuf=pOut;
    *inBuf=pIn;
    //    *numSub+=numS;
    return 0;

  } else if (cd->cnv_dmap->codingSchema == DMAP_U2U) {  /* UTF-16_UCS-2 */
    register int        inLen=*inBytesLeft;
    register int        outLen=*outBytesLeft;
    if (inLen <= outLen) {
      memcpy(*outBuf, *inBuf, inLen);
      (*outBytesLeft)-=inLen;
      (*inBuf)+=inLen;
      (*outBuf)+=inLen;
      *inBytesLeft=0;
      return 0;
    }
    memcpy(*outBuf, *inBuf, outLen);
    (*outBytesLeft)=0;
    (*inBuf)+=outLen;
    (*outBuf)+=outLen;
    *inBytesLeft-=outLen;
    return (*inBytesLeft);
      
  } else {
    return -1;
  }
  return 0;
}

// end was inline


