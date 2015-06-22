PROJECT(cleaver-cli CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

# Add Source Files
FILE(GLOB srcs *.cpp )
FILE(GLOB hdrs *.h   )

ADD_EXECUTABLE(cleaver-cli ${srcs} ${hdrs})

TARGET_LINK_LIBRARIES(cleaver-cli 
  cleaver 
  nrrd2cleaver 
  teem 
  boostparts 
  )
if (USE_SEGMENTATION_TOOLS)
  target_link_libraries(cleaver-cli seg_tools)
endif()
