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

find_path(SCIRun4_DIR
  NAMES SCIRun
  PATHS $ENV{HOME}
  "C:/"
  $ENV{HOME}/Documents
  $ENV{HOME}/Documents/Tools
  )


if (NOT SCIRun4_DIR)
  SET(SCIRun4_DIR SCIRun4_DIR_NOTFOUND CACHE PATH "Install location for SCIRun4")
  message(WARNING "SCIRun4 directory not found. Segmentation Tools Disabled.")
else()
  SET(SCIRun4_DIR "${SCIRun4_DIR}/SCIRun")
  message(STATUS "SCIRun4 found at: ${SCIRun4_DIR}")
  SET(SCIRun4_BINARY_DIR "${SCIRun4_DIR}/bin")
  SET(SCIRun4_FEMESHER_DIR "${SCIRun4_DIR}/bin/FEMesher")

  FILE(GLOB SEGMENTATION_DEPS 
    ${SCIRun4_BINARY_DIR}/ConvertFieldToNrrd*
    ${SCIRun4_BINARY_DIR}/ConvertNrrdToField*
    ${SCIRun4_BINARY_DIR}/ExtractIsosurface*
    ${SCIRun4_BINARY_DIR}/JoinFields*
    ${SCIRun4_BINARY_DIR}/morphsmooth*
    ${SCIRun4_BINARY_DIR}/TransformFieldWithTransform*
    ${SCIRun4_BINARY_DIR}/UnorientNrrdAndGetTransform*
    ${SCIRun4_BINARY_DIR}/ComputeTightenedLabels*
    ${SCIRun4_FEMESHER_DIR}/BuildMesh.py
    ${SCIRun4_FEMESHER_DIR}/MakeSoloNrrd.py
    ${SCIRun4_FEMESHER_DIR}/Utils.py
    ${CMAKE_SOURCE_DIR}/lib/Segmentation/*.py
    )
  foreach(file ${SEGMENTATION_DEPS})
    file( COPY ${file} DESTINATION ${CMAKE_BINARY_DIR}/bin)
    message (STATUS "copied ${file}")
  endforeach()
  add_definitions(-DUSE_BIOMESH_SEGMENTATION)
  add_definitions(-DTOOL_BINARY_DIR="${CMAKE_BINARY_DIR}/bin")
endif()
