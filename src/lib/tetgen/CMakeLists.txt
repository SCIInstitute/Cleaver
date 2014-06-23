###############################################################################
#                                                                             #
# CMakeLists.txt for TetGen                                                   #
#                                                                             #
###############################################################################
PROJECT(tetgenlib)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# CXXFLAGS is the level of optimiztion, default is -O. One should try
# -O2, -O3 ... to find the best optimization level.
# ===================================================================

#CXXFLAGS = -g

# PREDCXXFLAGS is for compiling J. Shewchuk's predicates. It should
# always be equal to -O0 (no optimization). Otherwise, TetGen may not
# work properly.

#PREDCXXFLAGS = -O0

# SWITCHES is a list of switches to compile TetGen.
# =================================================
#
# By default, TetGen uses double precision floating point numbers.  If you
#   prefer single precision, use the -DSINGLE switch. 
#
# The source code of TetGen includes a lot of assertions, which are mainly
#   used for catching bugs at that places.  These assertions somewhat slow
#   down the speed of TetGen.  They can be skipped by define the -DNDEBUG
#   switch.

# SWITCHES = -Wall -DSELF_CHECK

# SWITCHES = -Wall -Wabi -Wctor-dtor-privacy \
#            -Woverloaded-virtual -Wno-pmf-conversions -Wsign-promo \
#            -Wsynth  -Wchar-subscripts -Wconversion -Wsign-compare \
#            -Wcomment  -Wimplicit -Wmissing-braces -Wparentheses \
#            -Wreturn-type -Wswitch -Wswitch-default \
#            -Wswitch-enum -Wtrigraphs -W -DSELF_CHECK

# RM should be set to the name of your favorite rm (file deletion program).
# RM = /bin/rm


# project options
# OPTION(BUILD_SHARED_LIBS OFF)

# source files
FILE(GLOB TetGen_SOURCE_FILES *.c*)
FILE(GLOB TetGen_HEADER_FILES *.h)
SET(TetGen_API_HEADER_FILES tetgen.h)

# copy header files over
SET(TetGen_INSTALLED_HEADER_FILES)
FOREACH(header_file ${TetGen_API_HEADER_FILES})
  # When debugging, uncomment this line
  #MESSAGE(STATUS "Copy header file: ${header_file}")
  GET_FILENAME_COMPONENT(file_no_path "${header_file}" NAME)
  CONFIGURE_FILE("${header_file}"
    "${CMAKE_BINARY_DIR}/include/TetGen/${file_no_path}" COPYONLY IMMEDIATE)
  SET(Cleaver_INSTALLED_HEADER_FILES
    ${Cleaver_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/TetGen/${file_no_path}")
ENDFOREACH(header_file)

# output library
ADD_LIBRARY(tetgen STATIC ${TetGen_HEADER_FILES} ${TetGen_SOURCE_FILES})

