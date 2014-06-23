PROJECT(dumpAngles CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

# Add Source Files
FILE(GLOB srcs *.cpp )
FILE(GLOB hdrs *.h   )

ADD_EXECUTABLE(dumpAngles ${srcs} ${hdrs})

TARGET_LINK_LIBRARIES(dumpAngles cleaver boostparts)
