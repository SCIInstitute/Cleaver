#-------------------------------------------------------------------
#
# // Cleaver - A MultiMaterial Conforming Tetrahedral Meshing Library
# -- Library CMakeList
#
# Primary Author: Jonathan Bronson (bronson@sci.utah.edu)
#
#-------------------------------------------------------------------
#-------------------------------------------------------------------
#
# Copyright (c) 2011, 2012, Scientific Computing & Imaging Institute
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

project(cleaverlib)

cmake_minimum_required(VERSION 3.10.2)

# project version
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 1)
set(${PROJECT_NAME}_PATCH_LEVEL 0)

# project options
option(BUILD_SHARED_LIBS OFF)

# Add Teem library includes
include_directories(${Teem_BINARY_DIR}/include)

# source files
file(GLOB Cleaver_SOURCE_FILES *.c*)
file(GLOB Cleaver_HEADER_FILES *.h)
set(Cleaver_API_HEADER_FILES
    Cleaver.h
    CleaverMesher.h
    CleaverMesherImpl.h
    Octree.h            # remove on release
    AbstractField.h
    AbstractScalarField.h
    ScalarField.h
    SizingFieldCreator.h
    SizingFieldOracle.h
    ConstantField.h
    InverseField.h
    ScaledField.h
    AbstractVolume.h
    Volume.h
    TetMesh.h
    Vertex.h
    Geometry.h
    HalfEdge.h
    HalfFace.h
    Face.h
    Tet.h
    BoundingBox.h
    vec3.h
    Timer.h
    Status.h
    )

add_definitions(-DTETLIBRARY)

# Add library includes
include_directories(${CMAKE_BINARY_DIR}/include)

option(COPY_HEADER_FILES OFF)

if(COPY_HEADER_FILES) #TODO: Needs to be added to installation step for using as a lib
  # copy header files over
  set(Cleaver_INSTALLED_HEADER_FILES)
  foreach(header_file ${Cleaver_API_HEADER_FILES})
    # When debugging, uncomment this line
    get_filename_component(file_no_path "${header_file}" NAME)
    configure_file("${header_file}"
      "${CMAKE_BINARY_DIR}/include/cleaver/${file_no_path}" COPYONLY IMMEDIATE)
    set(Cleaver_INSTALLED_HEADER_FILES
      ${Cleaver_INSTALLED_HEADER_FILES}
      "${CMAKE_BINARY_DIR}/include/cleaver/${file_no_path}")
  endforeach()
endif()

# output library
add_library(cleaver STATIC ${Cleaver_HEADER_FILES} ${Cleaver_SOURCE_FILES})
target_link_libraries(cleaver jsoncpp)
