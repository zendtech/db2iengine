cmake_minimum_required (VERSION 2.8.0)
project (ibmdb2i)
# Dave add -malign-power -malign-natural (below)
# Our power engine will run better. Also fewer gcc odd boundary over runs.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gxcoff -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -malign-power -malign-natural -gxcoff -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -malign-power -malign-natural -gxcoff -O0")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-brtl,-lc,-blibpath:/QOpenSys/pkgs/lib:/QOpenSys/usr/lib:/usr/lib,-bbigtoc,-lpthread,-liconv")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-brtl,-lc,-blibpath:/QOpenSys/pkgs/lib:/QOpenSys/usr/lib:/usr/lib,-bbigtoc,-lpthread,-liconv")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-brtl,-lc,-blibpath:/QOpenSys/pkgs/lib:/QOpenSys/usr/lib:/usr/lib,-bbigtoc,-lpthread,-liconv")
# message("-- ibmdb2i CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}")
# message("-- ibmdb2i CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG}")
# message("-- ibmdb2i CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
include_directories(BEFORE special/include)
include_directories (
  ../../include
  ../../sql 
  ../../pcre)
SET(IBMDB2I_SOURCES db2i_blobCollection.cc
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
ha_ibmdb2i.cc)
# LINK_INTERFACE_LIBRARIES
set(PASELIB_LIBRARIES
    -L/QOpenSys/usr/lib
    pthread
    c
    iconv)
# STATIC_ONLY  (MANDATORY) 
# (MANDATORY)  -- affects sql/sql_builtin.c (gen file on cmake, build-xxx.sh) 
# -- links ibmdb2i into mysqld (daemon)
# -- INSTALL SONAME 'ha_ibmdb2i'; (no longer required)
# MYSQL_ADD_PLUGIN(ibmdb2i ${IBMDB2I_SOURCES} 
#  STORAGE_ENGINE STATIC_ONLY MANDATORY
#  LINK_LIBRARIES ${PASELIB_LIBRARIES})

# MODULE_ONLY
# -- INSTALL SONAME 'ha_ibmdb2i';
MYSQL_ADD_PLUGIN(ibmdb2i ${IBMDB2I_SOURCES} 
 STORAGE_ENGINE MODULE_ONLY
 LINK_LIBRARIES ${PASELIB_LIBRARIES})

