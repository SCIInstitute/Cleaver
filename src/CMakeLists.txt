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

# Global CMake options
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
OPTION(CMAKE_BUILD_TYPE Release)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
if(MVSC)
   add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Build Cleaver Library (no dependencies)
ADD_SUBDIRECTORY(lib/cleaver)
ADD_SUBDIRECTORY(lib/synthetic)
ADD_SUBDIRECTORY(lib/boost)
ADD_SUBDIRECTORY(lib/teem)
ADD_SUBDIRECTORY(lib/nrrd2cleaver)

include_directories(lib/boost)

ADD_SUBDIRECTORY(cli/mesher)
ADD_SUBDIRECTORY(cli/sizingfieldcreator)
ADD_SUBDIRECTORY(gui)
########################################################################
# All of the testing and test framework options
INCLUDE(CTest)
if (BUILD_TESTING)
  ENABLE_TESTING()
  ADD_SUBDIRECTORY(tests)
  include_directories(tests/include)
  include_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
  cxx_executable(tetmesh_unit_tests tests/cases gtest_main)
  target_link_libraries(tetmesh_unit_tests cleaver)
  ADD_TEST(tetmesh_unit_tests ${CMAKE_BUILD_DIR}/bin/tetmesh_unit_tests)
  #add_custom_command(TARGET tetmesh_unit_tests POST_BUILD COMMAND tetmesh_unit_tests)
endif()
########################################################################
# Use gcov for code coverage (useful for seeing test completeness)
OPTION(USE_GCOV "Enable code coverage for GNUCXX Platforms" OFF)
if (USE_GCOV)
  if(CMAKE_COMPILER_IS_GNUCXX)
    INCLUDE(CodeCoverage.cmake)
    SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    setup_target_for_coverage(${PROJECT_NAME}_coverage tetmesh_unit_tests coverage)
  endif()
endif()
