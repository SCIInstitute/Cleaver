#-------------------------------------------------------------------
#
# // Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
# -- Synthetic Data Library CMakeList
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

PROJECT(synthetic)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Add library incl1udes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ./)

# Add Source Files
FILE(GLOB srcs *.cpp )
FILE(GLOB hdrs *.h   )
SET(Synthetic_API_HEADER_FILES
    PlaneField.h
    PlaneSizingField.h
    SphereField.h
    SphereSizingField.h
    SphereVaryingField.h
    TorusField.h
    )

# copy header files over
SET(Synthetic_INSTALLED_HEADER_FILES)
FOREACH(header_file ${Synthetic_API_HEADER_FILES})
  # When debugging, uncomment this line
  #MESSAGE(STATUS "Copy header file: ${header_file}")
  GET_FILENAME_COMPONENT(file_no_path "${header_file}" NAME)
  CONFIGURE_FILE("${header_file}"
    "${CMAKE_BINARY_DIR}/include/Synthetic/${file_no_path}" COPYONLY IMMEDIATE)
  SET(Synthetic_INSTALLED_HEADER_FILES
    ${Synthetic_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/Synthetic/${file_no_path}")
ENDFOREACH(header_file)

# output library
ADD_LIBRARY(synthetic STATIC ${srcs} ${hdrs})


