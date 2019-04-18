PROJECT(sfcreator CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

# Add Source Files
FILE(GLOB srcs *.cpp )
FILE(GLOB hdrs *.h   )

ADD_EXECUTABLE(sfcreator ${srcs} ${hdrs})

TARGET_LINK_LIBRARIES(sfcreator cleaver nrrd2cleaver boostparts)
