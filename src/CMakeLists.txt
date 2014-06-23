#-----------------------------------------------------------------------------
#
# Cleaver2 - A MultiMaterial Tetrahedral Mesher
# -- CMakeList
#
# Primary Author:        Jonathan R. Bronson (bronson@sci.utah.edu)
# Contributing Authors:  Joshua   A. Levine  (levine@sci.utah.edu)
#                        Mark     B. Kim     (mkim@sci.utah.edu)
#                        Shankar     Sastry  (sastry@sci.utah.edu)
#
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
#
#  Copyright (C) 2011, 2012, 2013, 2014, Jonathan Bronson
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

PROJECT(CLEAVER2)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

OPTION(USE_CUDA "Use GPU Acceleration." OFF)
OPTION(USE_BLAS "Use BLAS Library."     OFF)
OPTION(USE_OPENMP "Use OpenMP Library." OFF)

SET(CMAKE_BUILD_TYPE Release)
#SET(CMAKE_BUILD_TYPE Debug)
#SET(CMAKE_BUILD_TYPE RelWithDebInfo)
#SET(CMAKE_CXX_FLAGS "-Wall -ansi -Weffc++" )
#SET(CMAKE_DEBUG_FLAGS "-Wall -ansi -Weffc++" )

#SET(CMAKE_CXX_FLAGS "-g ${CMAKE_CXX_FLAGS}" )
#SET(CMAKE_DEBUG_FLAGS "-Wall -ansi -O3" )

#set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} /GS-")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GS-")
#ADD_DEFINITIONS(-D_SECURE_SCL=0 -D_SCL_SECURE_NO_WARNINGS)
#ADD_DEFINITIONS(-D_HAS_ITERATOR_DEBUGGING=0)


if (USE_CUDA)
    ADD_DEFINITIONS(-DUSE_CUDA)
endif (USE_CUDA)

if(WIN32)
  SET(USE_STELLAR OFF CACHE BOOL "Use Stellar Library")
else()
  SET(USE_STELLAR ON  CACHE BOOL "Use Stellar Library")
  ADD_DEFINITIONS(-DUSE_STELLAR)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")


# Build Cleaver Library (no dependencies)
ADD_SUBDIRECTORY(lib/cleaver)
ADD_SUBDIRECTORY(lib/synthetic)
ADD_SUBDIRECTORY(lib/boost)

#ADD_SUBDIRECTORY(lib/gflags)

if(USE_BLAS)
  ADD_SUBDIRECTORY(lib/poisson)
endif()

ADD_SUBDIRECTORY(lib/teem)
ADD_SUBDIRECTORY(lib/nrrd2cleaver)

ADD_SUBDIRECTORY(lib/tetgen)

if(USE_STELLAR)
  ADD_SUBDIRECTORY(lib/stellar)
endif()

include_directories(lib/boost)

ADD_SUBDIRECTORY(cli/mesher)
ADD_SUBDIRECTORY(cli/sizingfieldcreator)
ADD_SUBDIRECTORY(cli/dumpAngles)

ADD_SUBDIRECTORY(cli/analyze)
#ADD_SUBDIRECTORY(cli/edgelength)
#ADD_SUBDIRECTORY(cli/minedgelength)
#ADD_SUBDIRECTORY(cli/maxedgelength)
#ADD_SUBDIRECTORY(cli/edgeratio)
#ADD_SUBDIRECTORY(cli/volume)
ADD_SUBDIRECTORY(gui)

# Build Teem
#IF(BUILD_CLEAVER_APP)
#   ADD_SUBDIRECTORY(teem)
# ENDIF(BUILD_CLEAVER_APP)



# Build Command-Line Unit Test
#ADD_SUBDIRECTORY(test)

