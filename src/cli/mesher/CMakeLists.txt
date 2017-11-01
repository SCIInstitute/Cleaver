PROJECT(cleaver-cli CXX)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

# Add Source Files
FILE(GLOB srcs *.cpp )
FILE(GLOB hdrs *.h   )

ADD_EXECUTABLE(cleaver-cli ${srcs} ${hdrs})

TARGET_LINK_LIBRARIES(cleaver-cli
  cleaver
  nrrd2cleaver
  boostparts
  )

INSTALL(TARGETS cleaver-cli
  RUNTIME DESTINATION ${CLEAVER2_RUNTIME_DIR} COMPONENT RuntimeCLI
  )
