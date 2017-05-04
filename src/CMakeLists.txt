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

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
    "Single output directory for building all executables.")

IF(NOT WIN32)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
ELSE()
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

# Build Cleaver Library
ADD_SUBDIRECTORY(lib/jsoncpp)
ADD_SUBDIRECTORY(lib/cleaver)

# TODO(jonbronson): Change to not use ITK by default once teem is added.
OPTION(USE_ITK "Use ITK instead of Teem for nrrd files." ON)
IF(NOT USE_ITK)
  ADD_SUBDIRECTORY(lib/teem)
ENDIF()

OPTION(BUILD_CLI "Build Cleaver CLI App (REQUIRES TEEM)" OFF)
OPTION(BUILD_GUI "Build Cleaver GUI App (REQUIRES TEEM)" OFF)

OPTION(BUILD_EXTRAS "Example use cases, tools" OFF)
IF(BUILD_EXTRAS)
  ADD_SUBDIRECTORY(examples)
ENDIF()

IF(BUILD_CLI OR BUILD_GUI)

  ADD_SUBDIRECTORY(lib/boost)
  INCLUDE_DIRECTORIES(lib/boost)
  ADD_SUBDIRECTORY(lib/nrrd2cleaver)

  IF(BUILD_CLI)
    ADD_SUBDIRECTORY(cli/mesher)
  ENDIF()

  IF(BUILD_EXTRAS)
      ADD_SUBDIRECTORY(lib/synthetic)
      ADD_SUBDIRECTORY(cli/sizingfieldcreator)
  ENDIF()

  IF(BUILD_GUI)
    ADD_SUBDIRECTORY(gui)
  ENDIF()

ENDIF()

# All of the testing and test framework options
INCLUDE(CTest)
IF(BUILD_TESTING)
  ENABLE_TESTING()
  # Use gcov for code coverage (useful for seeing test completeness)
  OPTION(USE_GCOV "Enable code coverage for GNUCXX Platforms" OFF)
  IF(USE_GCOV)
    IF(CMAKE_COMPILER_IS_GNUCXX)
      INCLUDE(${PROJECT_SOURCE_DIR}/CodeCoverage.cmake)
      SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
      SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    ENDIF()
  ENDIF()
  ADD_SUBDIRECTORY(test)
ENDIF()
