cmake_minimum_required( VERSION 3.5 )

project( ai_from_wonderland )

option( ENABLE_WARNING on )
option( ENABLE_DEPENDENCIES on )

string( TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++1z" )

if ( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_GENERATOR STREQUAL "Ninja" )
  set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fcolor-diagnostics" )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics" )
endif()

if( CMAKE_COMPILER_IS_GNUCXX )
  if( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9.2 OR CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 4.9.2 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always" )
  endif()
endif()

if(ENABLE_WARNING)
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic" )
else()
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w" )
endif()

include_directories( ${CMAKE_SOURCE_DIR}/source/shared/include )
include_directories( ${CMAKE_BINARY_DIR}/include )
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )

find_package( Threads REQUIRED )
find_package( Boost REQUIRED COMPONENTS timer random filesystem system )
#find_package( OpenSSL REQUIRED )

include_directories( ${Boost_INCLUDE_DIRS} )

include( ExternalProject )

if(WIN32)
  
  set(LIBRESSL_VERSION 2.5.1)
  set(LIBRESSL_LIBCRYPTO_WIN32_SUFFIX -41)
  set(LIBRESSL_LIBSSL_WIN32_SUFFIX -43)
  set(LIBRESSL_LIBTLS_WIN32_SUFFIX -15)
  
  set(LIBRESSL_HASH 1b3eb8c8e5507c553dac0a85163e09669b27619396b98c4a747a9a02b9a40603)
  
  string(REPLACE / \\ CMAKE_BINARY_DIR_WIN32 ${CMAKE_BINARY_DIR})
  set(LIBRESSL_LIBCRYPTO_LIBRARY ${CMAKE_BINARY_DIR_WIN32}\\bin\\libcrypto${LIBRESSL_LIBCRYPTO_WIN32_SUFFIX}.dll)
  set(LIBRESSL_LIBSSL_LIBRARY ${CMAKE_BINARY_DIR_WIN32}\\bin\\libssl${LIBRESSL_LIBSSL_WIN32_SUFFIX}.dll)
  set(LIBRESSL_LIBTLS_LIBRARY ${CMAKE_BINARY_DIR_WIN32}\\bin\\libtls${LIBRESSL_LIBTLS_WIN32_SUFFIX}.dll)
  
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( libressl_sub_directory x64 )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set( libressl_sub_directory x86 )
  else()
    message( FATAL_ERROR "unknown architeccture" )
  endif()
  
  ExternalProject_Add( libressl
    EXCLUDE_FROM_ALL 1
    PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject
    INSTALL_DIR       ${CMAKE_CURRENT_BINARY_DIR}
    URL               http://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-${LIBRESSL_VERSION}-windows.zip
    URL_HASH          SHA256=${LIBRESSL_HASH}
    BUILD_BYPRODUCTS  ${LIBRESSL_LIBRARIES}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    TEST_COMMAND      ""
    INSTALL_COMMAND   COMMAND ${CMAKE_COMMAND} -E copy_directory
                        ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject/src/libressl/include
                        ${CMAKE_CURRENT_BINARY_DIR}/include
                      COMMAND echo D | xcopy /Y /D ${CMAKE_BINARY_DIR_WIN32}\\ExternalProject\\src\\libressl\\${libressl_sub_directory}\\*.dll ${CMAKE_BINARY_DIR_WIN32}\\bin
                      COMMAND echo D | xcopy /Y /D ${CMAKE_BINARY_DIR_WIN32}\\ExternalProject\\src\\libressl\\${libressl_sub_directory}\\*.lib ${CMAKE_BINARY_DIR_WIN32}\\lib
  )
  
  unset( libressl_sub_directory )
  
else()
  
  ExternalProject_Add( libressl
    EXCLUDE_FROM_ALL 1
    PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject
    INSTALL_DIR       ${CMAKE_CURRENT_BINARY_DIR}
    GIT_REPOSITORY    git@github.com:libressl-portable/portable.git
    BUILD_IN_SOURCE   1
    CONFIGURE_COMMAND COMMAND ./autogen.sh
                      COMMAND ./configure --prefix=${CMAKE_BINARY_DIR}
    BUILD_COMMAND     make
    TEST_COMMAND      make check
    INSTALL_COMMAND   COMMAND make check
                      COMMAND make install
  )
  
  set(LIBRESSL_LIBCRYPTO_LIBRARY crypto)
  set(LIBRESSL_LIBSSL_LIBRARY ssl)
  set(LIBRESSL_LIBTLS_LIBRARY tls)
  
endif()

set(LIBRESSL_LIBRARIES
  ${LIBRESSL_LIBCRYPTO_LIBRARY}
  ${LIBRESSL_LIBSSL_LIBRARY}
  ${LIBRESSL_LIBTLS_LIBRARY}
)

ExternalProject_Add( picojson
  EXCLUDE_FROM_ALL  1
  PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject
  INSTALL_DIR       ${CMAKE_CURRENT_BINARY_DIR}
  GIT_REPOSITORY    https://github.com/kazuho/picojson.git
  GIT_TAG           master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject/src/picojson/picojson.h
      ${CMAKE_CURRENT_BINARY_DIR}/include/picojson.h
)

ExternalProject_Add( cmdline
  EXCLUDE_FROM_ALL  1
  PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject
  INSTALL_DIR       ${CMAKE_CURRENT_BINARY_DIR}
  GIT_REPOSITORY    git@github.com:tanakh/cmdline.git
  GIT_TAG           master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject/src/cmdline/cmdline.h
      ${CMAKE_CURRENT_BINARY_DIR}/include/cmdline.h
)

ExternalProject_Add( usagi
  EXCLUDE_FROM_ALL  1
  GIT_REPOSITORY    git@github.com:usagi/usagi.git
  PREFIX            ${CMAKE_CURRENT_BINARY_DIR}/ExternalProject
  INSTALL_DIR       ${CMAKE_CURRENT_BINARY_DIR}
  CMAKE_ARGS        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
                    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
                    -DEXAMPLES=off
  DEPENDS           picojson
)

file( GLOB_RECURSE wonderland_source_files source/wonderland/*.cxx )
add_executable( wonderland ${wonderland_source_files} )
if(ENABLE_RESOLVE_DEPENDENCIES)
add_dependencies( wonderland
  cmdline
  usagi
)
endif()

target_link_libraries( wonderland
  ${CMAKE_THREAD_LIBS_INIT}
  ${Boost_LIBRARIES}
  ${LIBRESSL_LIBRARIES}
)

if(WIN32)
  target_link_libraries( wonderland
    # http-client
    ws2_32
    # http-server
    Mswsock
  )
endif()
