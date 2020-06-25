cmake_minimum_required (VERSION 2.8.0)
project (ibmdb2i)
include_directories(BEFORE special/include)
include_directories (
  ../../include
  ../../sql
  ../../pcre)

SET(IBMDB2I_SOURCES
  db2i_blobCollection.cc
  db2i_misc.cc
  db2i_charsetSupport.cc
  db2i_collationSupport.cc
  db2i_constraints.cc
  db2i_conversion.cc
  db2i_errors.cc
  db2i_file.cc
  db2i_ileBridge.cc
  db2i_ioBuffers.cc
  db2i_myconv.cc
  db2i_rir.cc
  db2i_sqlStatementStream.cc
  db2i_global.cc
  db2i_safeString.cc
  ha_ibmdb2i.cc
)

MYSQL_ADD_PLUGIN(ibmdb2i
                ${IBMDB2I_SOURCES}
                STORAGE_ENGINE MODULE_ONLY
                LINK_LIBRARIES /QOpenSys/usr/lib/libiconv.a)

target_compile_options(ibmdb2i PRIVATE -O0)
