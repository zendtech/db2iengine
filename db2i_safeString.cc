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



#include "db2i_global.h"
#include "db2i_safeString.h"

// was inline

  SafeString::SafeString(char* buffer, size_t size) : 
  buf(buffer),
  curPos(0),
  allocSize(size)
  {
    DBUG_ASSERT(size > 0);
    buf[allocSize - 1] = 0xFF; // Set an overflow indicator
  }    

  SafeString& SafeString::strcat(const char* str)
  {
    return this->strncat(str, strlen(str));
  }
  
  SafeString& SafeString::strcat(char one)
  {
    if (curPos < allocSize - 2)
    {
      buf[curPos++] = one;
    }
    buf[curPos] = 0;
    
    return *this;    
  }
  
  SafeString& SafeString::strncat(const char* str, size_t len)
  {
    size_t amountToCopy = std::min(allocSize-1 - curPos, len);
    memcpy(buf + curPos, str, amountToCopy);
    curPos += amountToCopy;
    buf[curPos] = 0;
    return *this;
  }

// end was inline
