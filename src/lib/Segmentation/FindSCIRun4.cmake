#-----------------------------------------------------------------------------
#
#  Copyright (C) 2011, 2012, 2013, 2014
#  Scientific Computing and Imaging Institute
#  University of Utah
#
#
#  Permission is hereby  granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to  use, copy, modify,  merge, publish, distribute, sublicense,
#  and/or  sell  copies of  the Software, and  to permit persons to whom the
#  Software is  furnished to  do so,  subject  to  the following conditions:
#
#  The above copyright notice  and  this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING  BUT NOT  LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A  PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN  NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER  IN  AN  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT  OF  OR IN  CONNECTION  WITH THE  SOFTWARE  OR  THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#-----------------------------------------------------------------------------

#try to search for SCIRun4 somewhere

find_file(SCIRun4_DIR
  NAMES
  SCIRun
  SCIRun4
  PATHS
  $ENV{HOME}
  "C:/"
  $ENV{HOME}/Documents
  )

SET(SCIRun4_BINARY_DIR "${SCIRun4_DIR}/bin")
SET(SCIRun4_FEMESHER_DIR "${SCIRun4_DIR}/bin/FEMesher")
SET(required_deps
  ConvertFieldToNrrd
  ConvertNrrdToField
  ExtractIsosurface
  JoinFields
  morphsmooth
  TransformFieldWithTransform
  UnorientNrrdAndGetTransform
  ComputeTightenedLabels
  BuildMesh.py
  MakeSoloNrrd.py
  Utils.py
  )
foreach (file ${required_deps})
  find_file(${file}-dependency
    NAMES
    ${file}
    ${file}.exe
    PATHS
    ${SCIRun4_BINARY_DIR}
    ${SCIRun4_FEMESHER_DIR}
    NO_DEFAULT_PATH
    )
  if (${${file}-dependency} MATCHES "${file}-dependency-NOTFOUND")
    set(SCIRun4_DIR "SCIRun4_DIR-NOTFOUND" CACHE PATH "SCIRun4 directory" FORCE)
  endif()
  unset(${file}-dependency CACHE)
endforeach()

if (${SCIRun4_DIR} MATCHES "SCIRun4_DIR-NOTFOUND")
  message(WARNING "SCIRun4 directory not found. Segmentation Tools Disabled.")
  message(STATUS "Please set SCIRun4_DIR in CMake for Segmentation Tools.")
else()
  #copy the binaries from SCIRun to Cleaver binary dir
  message(STATUS "SCIRun4 found at: ${SCIRun4_DIR}")
  FILE(GLOB SEGMENTATION_DEPS
    ${SCIRun4_BINARY_DIR}/ConvertFieldToNrrd*
    ${SCIRun4_BINARY_DIR}/ConvertNrrdToField*
    ${SCIRun4_BINARY_DIR}/ExtractIsosurface*
    ${SCIRun4_BINARY_DIR}/JoinFields*
    ${SCIRun4_BINARY_DIR}/morphsmooth*
    ${SCIRun4_BINARY_DIR}/TransformFieldWithTransform*
    ${SCIRun4_BINARY_DIR}/UnorientNrrdAndGetTransform*
    ${SCIRun4_BINARY_DIR}/ComputeTightenedLabels*
    ${SCIRun4_BINARY_DIR}/*.dll
    ${SCIRun4_FEMESHER_DIR}/BuildMesh.py
    ${SCIRun4_FEMESHER_DIR}/MakeSoloNrrd.py
    ${SCIRun4_FEMESHER_DIR}/Utils.py
    ${CMAKE_SOURCE_DIR}/lib/Segmentation/*.py
    )
  foreach(file ${SEGMENTATION_DEPS})
    file( COPY ${file} DESTINATION ${CMAKE_BINARY_DIR}/bin)
  endforeach()
  # add definitions for Cleaver applications to determine whether to use tools
  add_definitions(-DUSE_BIOMESH_SEGMENTATION)
  # platform specific calls
  if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DLINUX)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_definitions(-DDARWIN)
  elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DWIN32)
  endif()
  add_subdirectory(${CMAKE_SOURCE_DIR}/lib/Segmentation)
  include_directories(${CMAKE_SOURCE_DIR}/lib/Segmentation)
  set(USE_SEGMENTATION_TOOLS ON)
  #MSVC/XCode put executables in Release/Debug folders. try copying unu to bin folder.
  SET(possible_unu_files
    ${CMAKE_BINARY_DIR}/bin/Release/unu
    ${CMAKE_BINARY_DIR}/bin/Release/unu.exe
    ${CMAKE_BINARY_DIR}/bin/Debug/unu
    ${CMAKE_BINARY_DIR}/bin/Debug/unu.exe
    ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/unu
    ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo/unu.exe
    ${CMAKE_BINARY_DIR}/bin/MinSizeRel/unu
    ${CMAKE_BINARY_DIR}/bin/MinSizeRel/unu.exe
    )
    foreach(file ${possible_unu_files})
      if (EXISTS ${file})
        file(COPY ${file} DESTINATION ${CMAKE_BINARY_DIR}/bin)
      endif()
    endforeach()
endif()
