#-----------------------------------------------------------------------------
#
# Cleaver - A MultiMaterial Tetrahedral Mesher
# -- CMakeList
#
# Primary Author:        Jonathan R. Bronson (bronson@sci.utah.edu)
# Contributing Authors:  Joshua   A. Levine  (levine@sci.utah.edu)
#                        Mark     B. Kim     (mkim@sci.utah.edu)
#                        Shankar     Sastry  (sastry@sci.utah.edu)
#                        Brig        Bagley  (brig@sci.utah.edu)
#                        Ally        Warner  (allywarner@sci.utah.edu)
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

project(CLEAVER2)

cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(cleaverInitializeBuildType)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE INTERNAL
    "Single output directory for building all executables.")

mark_as_advanced( CLEAVER2_RUNTIME_DIR CLEAVER2_BUNDLE_DIR)
set(CLEAVER2_RUNTIME_DIR "bin" CACHE STRING
  "Directory for installing runtime files; path is relative to CMAKE_INSTALL_PREFIX")
set(CLEAVER2_BUNDLE_DIR "." CACHE STRING
  "Directory for installing runtime files; path is relative to CMAKE_INSTALL_PREFIX")

if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Build Cleaver Library
include_directories(lib/cli11)
add_subdirectory(lib/jsoncpp)
add_subdirectory(lib/cleaver)

# TODO(jonbronson): Change to not use ITK by default once teem is added.
option(USE_ITK "Use ITK instead of Teem for nrrd files." ON)
if(NOT USE_ITK)
  add_subdirectory(lib/teem)
endif()

option(BUILD_CLI "Build Cleaver CLI App (REQUIRES TEEM)" OFF)
option(BUILD_GUI "Build Cleaver GUI App (REQUIRES TEEM)" OFF)

option(BUILD_EXTRAS "Example use cases, tools" OFF)
if(BUILD_EXTRAS)
  add_subdirectory(examples)
endif()

if(BUILD_CLI OR BUILD_GUI)

  add_subdirectory(lib/nrrd2cleaver)

  if(BUILD_CLI)
    add_subdirectory(cli/mesher)
  endif()

  if(BUILD_EXTRAS)
      add_subdirectory(lib/synthetic)
      add_subdirectory(cli/sizingfieldcreator)
      add_subdirectory(cli/info)
  endif()

  if(BUILD_GUI)
    add_subdirectory(gui)
  endif()

endif()

# All of the testing and test framework options
include(CTest)
if(BUILD_TESTING)
  enable_testing()
  # Use gcov for code coverage (useful for seeing test completeness)
  option(USE_GCOV "Enable code coverage for GNUCXX Platforms" OFF)
  if(USE_GCOV)
    if(CMAKE_COMPILER_IS_GNUCXX)
      include(${PROJECT_SOURCE_DIR}/CodeCoverage.cmake)
      set(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
      set(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    endif()
  endif()
  add_subdirectory(test)
endif()
