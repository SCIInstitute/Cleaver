#-------------------------------------------------------------------
#
# Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
# -- Nrrd Volume to Cleaver Conversion Library CMakeList
#
# Author: Jonathan Bronson (bronson@sci.utah.edu)
#
#-------------------------------------------------------------------
#-------------------------------------------------------------------
#
# Copyright (c) 2014 Scientific Computing & Imaging Institute
# and the University of Utah. All Rights Reserved.
#
# Redistribution and use in source and binary forms is strictly
# prohibited without the direct consent of the author.
#
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#-------------------------------------------------------------------
#-------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10.2)
project(TEEM_NRRD2Cleaver)

include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)
include_directories(${CMAKE_BINARY_DIR}/include)

# Add Source Files
file(GLOB srcs *.cpp )
file(GLOB hdrs *.h   )
set(Nrrd2Cleaver_API_HEADER_FILES
   ${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver/NRRDTools.h)


# copy header files over
set(Nrrd2Cleaver_INSTALLED_HEADER_FILES)
foreach(header_file ${Nrrd2Cleaver_API_HEADER_FILES})
  # When debugging, uncomment this line
  #message(STATUS "Copy header file: ${header_file}")
  get_filename_component(file_no_path "${header_file}" NAME)
  configure_file("${header_file}"
    "${CMAKE_BINARY_DIR}/include/nrrd2cleaver/${file_no_path}" COPYONLY IMMEDIATE)
  set(Nrrd2Cleaver_INSTALLED_HEADER_FILES
    ${Nrrd2Cleaver_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/nrrd2cleaver/${file_no_path}")
endforeach()


# output library
add_library(nrrd2cleaver STATIC ${srcs} ${hdrs})
target_link_libraries(nrrd2cleaver teem cleaver)
