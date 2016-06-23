#-----------------------------------------------------------------------------
#
# Cleaver2 - A MultiMaterial Tetrahedral Mesher
# -- CMakeList
#
# Primary Author:        Jonathan R. Bronson (bronson@sci.utah.edu)
# Contributing Authors:  Joshua   A. Levine  (levine@sci.utah.edu)
#                        Mark     B. Kim     (mkim@sci.utah.edu)
#                        Shankar     Sastry  (sastry@sci.utah.edu)
#                        Brig        Bagley  (brig@sci.utah.edu)
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

# Global CMake options
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
OPTION(CMAKE_BUILD_TYPE Release)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Build Cleaver Library (no dependencies)
ADD_SUBDIRECTORY(lib/cleaver)

ADD_SUBDIRECTORY(examples)

OPTION(BUILD_FULL_CLEAVER "doc" ON)
OPTION(BUILD_CLEAVER_APP "Build Cleaver GUI App (REQUIRES TEEM)" ON)
IF(BUILD_FULL_CLEAVER)

  ADD_SUBDIRECTORY(lib/synthetic)
  ADD_SUBDIRECTORY(lib/boost)
  ADD_SUBDIRECTORY(lib/teem)
  ADD_SUBDIRECTORY(lib/nrrd2cleaver)
  add_subdirectory(lib/Segmentation)

  include_directories(lib/boost)

  ADD_SUBDIRECTORY(cli/sizingfieldcreator)

  ADD_SUBDIRECTORY(cli/mesher)
  if (BUILD_CLEAVER_APP)
    ADD_SUBDIRECTORY(gui)
  endif()

  ########################################################################
  # All of the testing and test framework options
  INCLUDE(CTest)
  if (BUILD_TESTING)
    ENABLE_TESTING()
    # Use gcov for code coverage (useful for seeing test completeness)
    OPTION(USE_GCOV "Enable code coverage for GNUCXX Platforms" OFF)
    if (USE_GCOV)
      if(CMAKE_COMPILER_IS_GNUCXX)
        INCLUDE(${PROJECT_SOURCE_DIR}/CodeCoverage.cmake)
        SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
        SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
      endif()
    endif()
    ADD_SUBDIRECTORY(lib/gtest)
    ADD_SUBDIRECTORY(test)
  endif()
ENDIF()
