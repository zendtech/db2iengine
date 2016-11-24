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


#include "db2i_blobCollection.h"

/**
   Return the size to use when allocating space for blob reads.

  @param fieldIndex  The field to allocate for
  @param[out] shouldProtect  Indicates whether storage protection should be 
                             applied to the space, because the size returned is
                             smaller than the maximum possible size.   
*/

uint32 
BlobCollection::getSizeToAllocate(int fieldIndex, bool& shouldProtect)
{
  Field* field = table->getMySQLTable()->field[fieldIndex];
  uint fieldLength = field->max_display_length();

  if (fieldLength <= MAX_FULL_ALLOCATE_BLOB_LENGTH)
  {
    shouldProtect = false;
    return fieldLength;
  }

  shouldProtect = true;

  uint curMaxSize = table->getBlobFieldActualSize(fieldIndex);

  uint defaultAllocSize = min(defaultAllocation, fieldLength);

  return max(defaultAllocSize, curMaxSize);

}
      
void 
BlobCollection::generateBuffer(int fieldIndex)
{
  DBUG_ASSERT(table->db2Field(fieldIndex).isBlob());

  bool protect;    
  buffers[table->getBlobIdFromField(fieldIndex)].Malloc(getSizeToAllocate(fieldIndex, protect), protect);

  return;    
}

/**
  Realloc the read buffer associated with a blob field.

  This is used when the previous allocation for a blob field is found to be
  too small (this is discovered when QMY_READ trips over the protected boundary
  page).

  @param fieldIndex  The field to be reallocated
  @param size  The size of buffer to allocate for this field.
*/

ValidatedPointer<char>& 
BlobCollection::reallocBuffer(int fieldIndex, size_t size)
{
  ProtectedBuffer& buf = buffers[table->getBlobIdFromField(fieldIndex)];
  if (size <= buf.allocLen())
    return buf.ptr();

  table->updateBlobFieldActualSize(fieldIndex, size);

  DBUG_PRINT("BlobCollection::reallocBuffer",("PERF: reallocing %d to %d: ", fieldIndex, (int)size));

  bool protect;
  buf.Free();
  buf.Malloc(getSizeToAllocate(fieldIndex, protect), protect);
  return buf.ptr();
}

// was inline
  ProtectedBuffer::ProtectedBuffer() : 
  // bufptr(NULL),
  len(0),
  protectBuf(false)
  {;}
  
  void ProtectedBuffer::Malloc(size_t size, bool protect)
  {
    protectBuf = protect;
    bufptr.alloc(size + (protectBuf ? 0x1fff : 0x0));
    if ((void*)bufptr != NULL)
    {
      len = size;
      if (protectBuf) 
        mprotect(protectedPage(), 0x1000, PROT_NONE);
#ifndef DBUG_OFF
      // Prevents a problem with DBUG_PRINT over-reading in recent versions of 
      // MySQL
      *((char*)protectedPage()-1) = 0;
#endif
    }
  }
  
  void ProtectedBuffer::Free()
  {
    if ((void*)bufptr != NULL)
    {
      if (protectBuf)  
        mprotect(protectedPage(), 0x1000, PROT_READ | PROT_WRITE);
      bufptr.dealloc();
    }
  }
  
  ProtectedBuffer::~ProtectedBuffer()
  {
    Free();
  }

  void* ProtectedBuffer::protectedPage()
  {
    return (void*)(((address64_t)(void*)bufptr + len + 0x1000) & ~0xfff);
  }


  BlobCollection::BlobCollection(db2i_table* db2Table, uint32 defaultAllocSize) : 
  table(db2Table),
  buffers(NULL),
  defaultAllocation(defaultAllocSize)                     
  {
    buffers = new ProtectedBuffer[table->getBlobCount()];
  }

  BlobCollection::~BlobCollection()
  {
    delete[] buffers;
  }
    
  ValidatedPointer<char>& BlobCollection::getBufferPtr(int fieldIndex)
  {
    int blobIndex = table->getBlobIdFromField(fieldIndex);
    if ((char*)buffers[blobIndex].ptr() == NULL)
      generateBuffer(fieldIndex);
    
    return buffers[blobIndex].ptr();
  }

// end was inline


