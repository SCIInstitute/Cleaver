PROJECT(mesh-info CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)

# Add Source Files
FILE(GLOB srcs *.cpp )
FILE(GLOB hdrs *.h   )

ADD_EXECUTABLE(mesh-info ${srcs} ${hdrs})

TARGET_LINK_LIBRARIES(mesh-info
  cleaver
  boostparts
  )
