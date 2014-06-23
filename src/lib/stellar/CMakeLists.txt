###############################################################################
#                                                                             #
# CMakeLists.txt for Stellar                                                  #
#                                                                             #
###############################################################################
PROJECT(stellarlib)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# project options
#SET(CMAKE_BUILD_TYPE Release)
OPTION(BUILD_SHARED_LIBS OFF)

ADD_DEFINITIONS(-DNOMAIN)
#ADD_DEFINITIONS(-DSTARLIBRARY)
ADD_DEFINITIONS(-DSELF_CHECK)
ADD_DEFINITIONS(-DLINUX)

if(CMAKE_COMPILER_IS_GNUCXX)
   message(STATUS "GCC detected, adding compile flags")
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pg -DSELF_CHECK -DLINUX -O3 -Wall -Wconversion -Wstrict-prototypes -Wno-strict-aliasing -fno-strict-aliasing" )
else(CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pg -DSELF_CHECK -O3 -Wall -fno-strict-aliasing" )
endif(CMAKE_COMPILER_IS_GNUCXX)



# source files
FILE(GLOB Stellar_SOURCE_FILES ./src/Starbase.c ./src/Stellar.c)
FILE(GLOB Stellar_HEADER_FILES ./src/*.h)
SET(Stellar_API_HEADER_FILES ./src/Starbase.h ./src/Stellar.h)

# copy header files over
SET(Stellar_INSTALLED_HEADER_FILES)
FOREACH(header_file ${Stellar_API_HEADER_FILES})
  # When debugging, uncomment this line
  #MESSAGE(STATUS "Copy header file: ${header_file}")
  GET_FILENAME_COMPONENT(file_no_path "${header_file}" NAME)
  CONFIGURE_FILE("${header_file}"
    "${CMAKE_BINARY_DIR}/include/Stellar/${file_no_path}" COPYONLY IMMEDIATE)
  SET(Stellar_INSTALLED_HEADER_FILES
    ${Stellar_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/Stellar/${file_no_path}")
ENDFOREACH(header_file)

# output library
ADD_LIBRARY(stellar STATIC ${Stellar_SOURCE_FILES} ${Stellar_HEADER_FILES})
#ADD_EXECUTABLE(stellar ${Stellar_SOURCE_FILES} ${Stellar_HEADER_FILES})
TARGET_LINK_LIBRARIES(stellar m)
