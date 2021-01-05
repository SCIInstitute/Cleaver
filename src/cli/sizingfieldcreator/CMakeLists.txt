project(sfcreator CXX)

cmake_minimum_required(VERSION 2.6)

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

# Add Source Files
file(GLOB srcs *.cpp )
file(GLOB hdrs *.h   )

add_executable(sfcreator ${srcs} ${hdrs})

target_link_libraries(sfcreator cleaver nrrd2cleaver boostparts)
