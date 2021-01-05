#
# Teem: Tools to process and visualize scientific data and images              
# Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# (LGPL) as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also
# include exceptions to the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

# This is the root Teem CMakeLists file
cmake_minimum_required(VERSION 2.4)


# Remove PNG support, as teem uses deprecated libpng calls
set(Teem_PNG OFF CACHE BOOL "Build Teem with support for PNG images.")

# As of CMake 2.6 Policies were introduced in order to provide a mechanism for
# adding backwards compatibility one feature at a time.

# Run cmake --help-policy CMP<num> to see documentation.
if(COMMAND cmake_policy)
  # Library paths (/path/to/libmy.so not translated to -L/path/to -lmy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(Teem)

#-----------------------------------------------------------------------------
# Teem version number.  Must reflect the values in teem/src/air/air.h
set(Teem_VERSION_MAJOR "1")
set(Teem_VERSION_MINOR "10")
set(Teem_VERSION_PATCH "0")

# Version string should not include patch level.  The major.minor is
# enough to distinguish available features of the toolkit.
set(Teem_VERSION_STRING "${Teem_VERSION_MAJOR}.${Teem_VERSION_MINOR}.${Teem_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

# We need ansi c-flags, especially on HP
set(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_REQUIRED_FLAGS ${CMAKE_ANSI_CFLAGS})

#-----------------------------------------------------------------------------
# Test for some required system information.
include (CMakeBackwardCompatibilityC)

#-----------------------------------------------------------------------------
# Output directories.
set (LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE INTERNAL "Single output directory for building all libraries.")
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
#output directory for installing all include files.")
mark_as_advanced(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)
set(Teem_LIBRARY_PATH "${LIBRARY_OUTPUT_PATH}")
set(Teem_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")
set(C_TEST_PATH ${EXECUTABLE_OUTPUT_PATH})


file(MAKE_DIRECTORY ${Teem_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${Teem_BINARY_DIR}/include/teem)

#-----------------------------------------------------------------------------
# Find platform-specific differences in the handling of IEEE floating point 
# special values.

include(${Teem_SOURCE_DIR}/CMake/TestQnanhibit.cmake)
TEST_QNANHIBIT(QNANHIBIT_VALUE ${Teem_SOURCE_DIR}/CMake)
if(QNANHIBIT_VALUE)
  set(QNANHIBIT 1 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
else(QNANHIBIT_VALUE)
  set(QNANHIBIT 0 CACHE INTERNAL "The 22nd bit of 32-bit floating-point quiet NaN.")
endif(QNANHIBIT_VALUE)


#-----------------------------------------------------------------------------
# Teem build configuration options.
option(BUILD_SHARED_LIBS "Build Teem with shared libraries." OFF)
set(Teem_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
if(NOT Teem_BUILD_SHARED_LIBS)
  add_definitions(-DTEEM_STATIC)
endif(NOT Teem_BUILD_SHARED_LIBS)

option(Teem_ZLIB "Build Teem with support for gzip compression." ON)
option(Teem_PNG "Build Teem with support for PNG images." ON)
option(Teem_VTK_MANGLE "Build Teem using the mangled libraries from VTK for ZLIB and PNG." OFF)
set(Teem_VTK_ZLIB_MANGLE_IPATH "" CACHE PATH "Location of vtk_zlib_mangle.h")
option(Teem_VTK_TOOLKITS_IPATH "Location of VTK Build directory for ZLIB and PNG includes." OFF)

# Can't build PNG without ZLIB, so force it on.
if(Teem_PNG AND NOT Teem_ZLIB)
  set(Teem_ZLIB ON)
endif(Teem_PNG AND NOT Teem_ZLIB)

if(Teem_VTK_MANGLE)
  add_definitions(-DTEEM_VTK_MANGLE=1)
  include_directories( ${Teem_VTK_TOOLKITS_IPATH} )
  include_directories( ${Teem_VTK_ZLIB_MANGLE_IPATH} )
endif(Teem_VTK_MANGLE)


set(Teem_ZLIB_LIB "")
set(Teem_PNG_LIB "")
if(Teem_ZLIB)
  # Find ZLIB
  find_package(ZLIB)
  if(ZLIB_FOUND)
    add_definitions(-DTEEM_ZLIB)
    set(Teem_ZLIB_LIB ${ZLIB_LIBRARIES})
    set(Teem_ZLIB_IPATH ${ZLIB_INCLUDE_DIR})
  else(ZLIB_FOUND)
    # We need to set this as a cache variable, so that it will show up as
    # being turned off in the cache.
    message(WARNING "Turning off Teem_ZLIB, because it wasn't found.")
    set(Teem_ZLIB OFF CACHE BOOL "Build Teem with support for gzip compression." FORCE)
  endif(ZLIB_FOUND)

  if(Teem_PNG)
    find_package(PNG)
    if(PNG_FOUND)
      add_definitions(-DTEEM_PNG ${PNG_DEFINITIONS})
      set(Teem_PNG_LIB ${PNG_LIBRARIES})
      set(Teem_PNG_IPATH ${PNG_INCLUDE_DIR})
    else(PNG_FOUND)
      # We need to set this as a cache variable, so that it will show up as
      # being turned off in the cache.
      message(WARNING "Turning off Teem_PNG, because it wasn't found.")
      set(Teem_PNG OFF CACHE BOOL "Build Teem with support for PNG images." FORCE)
    endif(PNG_FOUND)
  endif(Teem_PNG)
endif(Teem_ZLIB)

# Try and locate BZIP2 stuff
option(Teem_BZIP2 "Build Teem with support for bzip compression." ON)
set(Teem_BZIP2_LIB "")

if(Teem_BZIP2)
  find_package(BZ2)

  if(BZ2_FOUND)
    add_definitions(-DTEEM_BZIP2)
    set(Teem_BZIP2_LIB ${BZ2_LIBRARIES})
    set(Teem_BZIP2_IPATH ${BZ2_INCLUDE_DIR})
  else(BZ2_FOUND)
    # We need to set this as a cache variable, so that it will show up as
    # being turned off in the cache.
    message(WARNING "Turning off Teem_BZIP2, because it wasn't found.")
    set(Teem_BZIP2 OFF CACHE BOOL "Build Teem with support for bzip compression." FORCE)
  endif(BZ2_FOUND)
endif(Teem_BZIP2)

# Look for threading libraries
option(Teem_PTHREAD "Build Teem with pthread library support." ON)
if(Teem_PTHREAD)
  include(FindThreads)
  if(CMAKE_USE_PTHREADS_INIT)
    add_definitions(-DTEEM_PTHREAD)
  else(CMAKE_USE_PTHREADS_INIT)
    # We need to set this as a cache variable, so that it will show up as
    # being turned off in the cache.
    message(WARNING "Turning off Teem_PTHREAD, because it wasn't found.")
    set(Teem_PTHREAD OFF CACHE BOOL "Build Teem with pthread library support." FORCE)
    set(Teem_PTHREAD OFF)
  endif(CMAKE_USE_PTHREADS_INIT)
endif(Teem_PTHREAD)

# Look for "levmar" library <http://www.ics.forth.gr/~lourakis/levmar/>
option(Teem_LEVMAR "Build Teem with levmar library support." OFF)
set(Teem_LEVMAR_LIB "")
if(Teem_LEVMAR)
  find_package(LEVMAR)

  if(LEVMAR_FOUND)
    add_definitions(-DTEEM_LEVMAR)
    set(Teem_LEVMAR_LIB ${LEVMAR_LIBRARIES})
    set(Teem_LEVMAR_IPATH ${LEVMAR_INCLUDE_DIR})
  else(LEVMAR_FOUND)
    # We need to set this as a cache variable, so that it will show up as
    # being turned off in the cache.
    message(WARNING "Turning off Teem_LEVMAR, because it wasn't found.")
    set(Teem_LEVMAR OFF CACHE BOOL "Build Teem with levmar library support." FORCE)
  endif(LEVMAR_FOUND)
endif(Teem_LEVMAR)

#-----------------------------------------------------------------------------

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  # For Visual Studio we don't care about warnings about deprecated sprintf and
  # the like.
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif(${CMAKE_GENERATOR} MATCHES "Visual Studio")

#Include Teem Defines
# Set compiler flags for endian-ness.
if(CMAKE_WORDS_BIGENDIAN)
  add_definitions(-DTEEM_ENDIAN=4321)
else(CMAKE_WORDS_BIGENDIAN)
  add_definitions(-DTEEM_ENDIAN=1234)
endif(CMAKE_WORDS_BIGENDIAN)

# Set compiler flags for 32 or 64 bit architecture (based on the size
# of a void pointer).
if(CMAKE_SIZEOF_VOID_P MATCHES  8)
  add_definitions(-DTEEM_32BIT=0)
else(CMAKE_SIZEOF_VOID_P MATCHES 8)
  add_definitions(-DTEEM_32BIT=1)
endif(CMAKE_SIZEOF_VOID_P MATCHES 8)

#The QNANHIBIT variable is configured by the root level CMakeLists.txt
if(QNANHIBIT)
  add_definitions(-DTEEM_QNANHIBIT=1)
else(QNANHIBIT)
  add_definitions(-DTEEM_QNANHIBIT=0)
endif(QNANHIBIT)

#DirectIO is the fast way to do multi-gigabyte I/O and currently only available
#for SGI platforms.  Use of DirectIO is enabled manually for now.
#option(USE_DIRECTIO "Use DirectIO for Nrrd file IO.  Only valid on SGI systems." 0)
#mark_as_advanced(USE_DIRECTIO)
#if(USE_DIRECTIO)
#  add_definitions(-DTEEM_DIO=1)
#else(USE_DIRECTIO)
add_definitions(-DTEEM_DIO=0)
#endif(USE_DIRECTIO)

set(BUILD_EXPERIMENTAL_LIBS OFF CACHE BOOL "Build Teem's experimental/unstable libraries")
set(BUILD_EXPERIMENTAL_APPS OFF CACHE BOOL "Build Teem's non-essential command-line tools")

set(Teem_LIBRARIES air hest biff nrrd ell unrrdu gage limn seek hoover echo ten dye mite)
if(BUILD_EXPERIMENTAL_LIBS)
  set(Teem_LIBRARIES ${Teem_LIBRARIES} bane moss coil alan pull)
endif(BUILD_EXPERIMENTAL_LIBS)

set(Teem_HEADER_FILES
  air/air.h
  hest/hest.h
  biff/biff.h
  nrrd/nrrd.h
  nrrd/nrrdDefines.h
  nrrd/nrrdMacros.h
  nrrd/nrrdEnums.h
  ell/ell.h
  ell/ellMacros.h
  unrrdu/unrrdu.h
  gage/gage.h
  limn/limn.h
  seek/seek.h
  hoover/hoover.h
  echo/echo.h
  ten/ten.h
  ten/tenMacros.h
  dye/dye.h
  mite/mite.h
  )
if(BUILD_EXPERIMENTAL_LIBS)
  set(Teem_HEADER_FILES ${Teem_HEADER_FILES}
    bane/bane.h
    moss/moss.h
    coil/coil.h
    alan/alan.h
    pull/pull.h
    )
endif(BUILD_EXPERIMENTAL_LIBS)
set(Teem_INSTALLED_HEADER_FILES)
foreach(header_file ${Teem_HEADER_FILES})
  # When debugging, uncomment this line
  #message(STATUS "Copy header file: ${header_file}")
  get_filename_component(file_no_path "${header_file}" NAME)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/${header_file}"
    "${CMAKE_BINARY_DIR}/include/teem/${file_no_path}" COPYONLY IMMEDIATE)
  set(Teem_INSTALLED_HEADER_FILES
    ${Teem_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/teem/${file_no_path}")
endforeach(header_file)

#---
# Include directory 
include_directories(
  "${CMAKE_BINARY_DIR}/include/"
  "${Teem_SOURCE_DIR}/include/"
  )


## Add external library path includes
if(Teem_ZLIB)
  include_directories(${Teem_ZLIB_IPATH})
  include_directories(${Teem_ZLIB_DLLCONF_IPATH})
  if(Teem_PNG)
    include_directories(${Teem_PNG_IPATH})
    include_directories(${Teem_PNG_DLLCONF_IPATH})
  endif(Teem_PNG)
endif(Teem_ZLIB)

if(Teem_BZIP2)
  include_directories(${Teem_BZIP2_IPATH})
endif(Teem_BZIP2)

if(Teem_LEVMAR)
  include_directories(${Teem_LEVMAR_IPATH})
endif(Teem_LEVMAR)

# All the source files
set(Teem_SOURCES)

#-----------------------------------------------------------------------------
# Macro for adding the current set of source files to the master list
macro(ADD_Teem_LIBRARY dir)
  set(srcs)
  foreach(src ${ARGN})
    set(srcs ${srcs} "src/${dir}/${src}")
  endforeach(src ${ARGN})
  set(Teem_SOURCES ${Teem_SOURCES} ${srcs})
  # This will group all the source files in the VS project by directory
  source_group( ${dir} FILES ${srcs} )
endmacro(ADD_Teem_LIBRARY name)


#-----------------------------------------------------------------------------
# Dispatch the build into the proper subdirectories.
foreach(dir ${Teem_LIBRARIES})
  include(src/${dir}/sources.cmake)
endforeach(dir ${Teem_LIBRARIES})

#-----------------------------------------------------------------------------
# Build the Mega library
#
# This needs to occur after the parsing of the subdirectories.

# Create the library target
add_library(teem ${Teem_SOURCES})

# Set up some library paths for installation.  Windows will ingore the
# RPATH stuff, and mac will ingore the INSTALL_RPATH.  Make sure for
# macs to set BUILD_WITH_INSTALL_RPATH OFF and set INSTALL_NAME_DIR.
# I belive INSTALL_NAME_DIR will be ignored on linux.
set_target_properties(teem PROPERTIES
  BUILD_WITH_INSTALL_RPATH OFF
  INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib
  INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/lib
  SOVERSION 1
  VERSION ${Teem_VERSION_STRING}
  )

if(Teem_BZIP2_LIB)
  target_link_libraries(teem ${Teem_BZIP2_LIB})
endif(Teem_BZIP2_LIB)
if(Teem_ZLIB_LIB)
  target_link_libraries(teem ${Teem_ZLIB_LIB})
  if(Teem_PNG_LIB)
    target_link_libraries(teem ${Teem_PNG_LIB})
  endif(Teem_PNG_LIB)
endif(Teem_ZLIB_LIB)

if(Teem_LEVMAR_LIB)
  target_link_libraries(teem ${Teem_LEVMAR_LIB})
endif(Teem_LEVMAR_LIB)

if(Teem_PTHREAD)
  target_link_libraries(teem ${CMAKE_THREAD_LIBS_INIT})
endif(Teem_PTHREAD)

if(UNIX)
  target_link_libraries(teem -lm)
endif(UNIX)

option(Teem_USE_LIB_INSTALL_SUBDIR "Add a Teem-X.Y.Z directory layer to the installation tree for libraries and archives." OFF)
if(Teem_USE_LIB_INSTALL_SUBDIR)
  set(EXTRA_INSTALL_PATH /Teem-${Teem_VERSION_STRING})
else(Teem_USE_LIB_INSTALL_SUBDIR)
  set(EXTRA_INSTALL_PATH "")
endif(Teem_USE_LIB_INSTALL_SUBDIR)

install(TARGETS teem
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib${EXTRA_INSTALL_PATH}
  ARCHIVE DESTINATION lib${EXTRA_INSTALL_PATH}
  ) 

# Now compile the binaries
add_subdirectory(src/bin)

# Stand-alone programs to process hex encoding of data
option(BUILD_HEX "Build dehex and enhex" OFF)
if(BUILD_HEX)
  add_subdirectory(src/hex)
endif(BUILD_HEX)

#-----------------------------------------------------------------------------
# For testing
option(BUILD_TEEM_TESTS "Enable this to perform testing of Teem" OFF)
if(BUILD_TEEM_TESTS)
  set(DART_ROOT "" CACHE INTERNAL "" FORCE)
  include(CTest)
  enable_testing()
  mark_as_advanced(TCL_TCLSH)
endif(BUILD_TEEM_TESTS)

#-----------------------------------------------------------------------------
# Help outside projects build Teem projects.
include(CMakeExportBuildSettings)
export_library_dependencies(${Teem_BINARY_DIR}/TeemLibraryDepends.cmake)
CMAKE_EXPORT_BUILD_SETTINGS(${Teem_BINARY_DIR}/TeemBuildSettings.cmake)

set(CFLAGS "${CMAKE_C_FLAGS}")
set(CC "${CMAKE_C_COMPILER}")

# For build tree usage
set(Teem_LIBRARY_DEPENDS_FILE ${Teem_BINARY_DIR}/TeemLibraryDepends.cmake)
set(Teem_EXECUTABLE_DIRS ${EXECUTABLE_OUTPUT_PATH})
set(Teem_LIBRARY_DIRS ${LIBRARY_OUTPUT_PATH})
set(Teem_USE_FILE ${Teem_SOURCE_DIR}/CMake/TeemUse.cmake)
set(Teem_INCLUDE_DIRS "${Teem_BINARY_DIR}/include")
set(Teem_BUILD_SETTINGS_FILE ${Teem_BINARY_DIR}/TeemBuildSettings.cmake)

set(prefix "${Teem_BINARY_DIR}")
set(exec_prefix "${prefix}")
set(exec_prefix_set "no")
set(bindir "${prefix}/bin")
set(includedir "${prefix}")
set(libdir "${prefix}/bin")
set(srcdir "${Teem_SOURCE_DIR}")

configure_file("${Teem_SOURCE_DIR}/CMake/TeemConfig.cmake.in"
  "${Teem_BINARY_DIR}/TeemConfig.cmake" @ONLY IMMEDIATE)

# For installed tree usage
set(Teem_LIBRARY_DEPENDS_FILE ${CMAKE_INSTALL_PREFIX}/lib/Teem-${Teem_VERSION_STRING}/TeemLibraryDepends.cmake)
set(Teem_EXECUTABLE_DIRS "${CMAKE_INSTALL_PREFIX}/bin")
set(Teem_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib${EXTRA_INSTALL_PATH}")
set(Teem_USE_FILE "${CMAKE_INSTALL_PREFIX}/lib/Teem-${Teem_VERSION_STRING}/TeemUse.cmake")
set(Teem_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include")
set(Teem_BUILD_SETTINGS_FILE ${CMAKE_INSTALL_PREFIX}/lib/Teem-${Teem_VERSION_STRING}/TeemBuildSettings.cmake)
set(Teem_BUILT_LIBRARIES teem) # The libraries built by teem.  Currently we only build the mega library.
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "${prefix}")
set(exec_prefix_set "no")
set(bindir "${prefix}/bin")
set(includedir "${prefix}")
set(libdir "${prefix}/lib${EXTRA_INSTALL_PATH}")
set(srcdir ".")

configure_file("${Teem_SOURCE_DIR}/CMake/TeemConfig.cmake.in"
  "${Teem_BINARY_DIR}/CMake/TeemConfig.cmake" @ONLY IMMEDIATE)

install(FILES ${Teem_INSTALLED_HEADER_FILES}
  DESTINATION include/teem
  )
install(FILES
  "${Teem_SOURCE_DIR}/CMake/TeemUse.cmake"
  "${Teem_BINARY_DIR}/TeemBuildSettings.cmake"
  "${Teem_BINARY_DIR}/TeemLibraryDepends.cmake"
  "${Teem_BINARY_DIR}/CMake/TeemConfig.cmake"
  DESTINATION lib/Teem-${Teem_VERSION_STRING}
  )

