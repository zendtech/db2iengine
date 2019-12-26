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


#ifndef DB2I_SQLSTATEMENTSTREAM_H
#define DB2I_SQLSTATEMENTSTREAM_H

#include "db2i_charsetSupport.h"
#include <qmyse.h>

/**
  @class SqlStatementStream
  
   This class handles building the stream of SQL statements expected by the 
   QMY_EXECUTE_IMMEDIATE and QMY_PREPARE_OPEN_CURSOR APIs. 
   Memory allocation is handled internally.
*/
class SqlStatementStream
{
  public:
    /**
      ctor to be used when multiple strings may be appended.
    */
    SqlStatementStream(uint32 firstStringSize);
    
    /**
      ctor to be used when only a single statement will be executed.
    */
    SqlStatementStream(const String& statement);

    /**
      ctor to be used when only a single statement will be executed.
    */
    SqlStatementStream(const char* statement);

    /**
      Append an SQL statement, specifiying the DB2 sort sequence under which
      the statement should be executed. This is important for CREATE TABLE
      and CREATE INDEX statements.
    */
    SqlStatementStream& addStatement(const String& append, const char* fileSortSequence, const char* fileSortSequenceLibrary);
    
    /**
      Append an SQL statement using default (*HEX) sort sequence.
    */
    SqlStatementStream& addStatement(const String& append);
    
    /**
      Append an SQL statement using default (*HEX) sort sequence.
    */
    SqlStatementStream& addStatement(const char* stmt);
    
    char* getPtrToData() const { return block; }
    uint32 getStatementCount() const { return statements; }
  private:
    SqlStatementStream& addStatementInternal(const char* stmt, 
                                             uint32 length, 
                                             const char* fileSortSequence, 
                                             const char* fileSortSequenceLibrary);
    
    uint32 storageRemaining() const;
    
    char* getNewSpace(size_t size);

    uint32 curSize; // The size of the usable memory.
    char* allocBase; // The allocated memory (with padding for aligment)
    char* block __attribute__((aligned(16))); // The usable memory chunck (aligned for ILE)
    char* ptr; // The current position within block.
    uint32 statements; // The number of statements that have been appended.
};

#endif

