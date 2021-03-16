########################################################
#
#  This is a CMake configuration file.
#  To use it you need CMake which can be
#  downloaded from here:
#    http://www.cmake.org/cmake/resources/software.html
#
#########################################################

# The interesting parts of Boost have been extracted using
# the BCP tool:
#   http://www.boost.org/doc/libs/1_45_0/tools/bcp/doc/html/index.html
#
# bcp call: bcp program_options

# WARNING: When doing this from fresh boost sources, note that
# in the file /boost/config/user.hpp the BOOST_ALL_NO_LIB
# define has been uncommented. Otherwise, we will get linker
# errors on MSVC because of boost auto-linking.

project(boostparts)

cmake_minimum_required(VERSION 3.10.2)

# project version
set(${PROJECT_NAME}_MAJOR_VERSION 0)
set(${PROJECT_NAME}_MINOR_VERSION 1)
set(${PROJECT_NAME}_PATCH_LEVEL 0)

# project options
option(BUILD_SHARED_LIBS OFF)

# copy header files over
set(Boost_INSTALLED_HEADER_FILES)
foreach(header_file ${Boost_API_HEADER_FILES})
  # When debugging, uncomment this line
  #message(STATUS "Copy header file: ${header_file}")
  get_filename_component(file_no_path "${header_file}" NAME)
  configure_file("${header_file}"
    "${CMAKE_BINARY_DIR}/include/boost/${file_no_path}" COPYONLY IMMEDIATE)
  set(Boost_INSTALLED_HEADER_FILES
    ${Boost_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/boost/${file_no_path}")
endforeach()

set(Boost_API_HEADER_FILES
    cmdline.hpp
    config.hpp
    environment_iterator.hpp
    ../eof_iterator.hpp
    ../errors.hpp
    ../option.hpp
    ../options_description.hpp
    ../parsers.hpp
    ../positional_options.hpp
    ../value_semantic.hpp
    ../variables_map.hpp
    ../version.hpp
    detail/cmdline.hpp
    detail/config_file.hpp
    detail/convert.hpp
    detail/parsers.hpp
    detail/utf8_codecvt_facet.hpp
    detail/value_semantic.hpp
    )



# source files
set(SOURCE_FILES
    libs/program_options/src/cmdline.cpp
    libs/program_options/src/config_file.cpp
    libs/program_options/src/convert.cpp
    libs/program_options/src/options_description.cpp
    libs/program_options/src/parsers.cpp
    libs/program_options/src/positional_options.cpp
    libs/program_options/src/split.cpp
    libs/program_options/src/utf8_codecvt_facet.cpp
    libs/program_options/src/value_semantic.cpp
    libs/program_options/src/variables_map.cpp
    libs/program_options/src/winmain.cpp
    libs/regex/src/c_regex_traits.cpp
    libs/regex/src/cpp_regex_traits.cpp
    libs/regex/src/cregex.cpp
    libs/regex/src/fileiter.cpp
    libs/regex/src/icu.cpp
    libs/regex/src/instances.cpp
    libs/regex/src/posix_api.cpp
    libs/regex/src/regex.cpp
    libs/regex/src/regex_debug.cpp
    libs/regex/src/regex_raw_buffer.cpp
    libs/regex/src/regex_traits_defaults.cpp
    libs/regex/src/static_mutex.cpp
    libs/regex/src/usinstances.cpp
    libs/regex/src/w32_regex_traits.cpp
    libs/regex/src/wc_regex_traits.cpp
    libs/regex/src/wide_posix_api.cpp
    libs/regex/src/winstances.cpp
    libs/smart_ptr/src/sp_collector.cpp
    libs/smart_ptr/src/sp_debug_hooks.cpp
   )

#file(GLOB HEADER_FILES ../*.hpp)
include_directories( ${CMAKE_CURRENT_SOURCE_DIR} )

# output library
add_library(boostparts  ${SOURCE_FILES} ${HEADER_FILES})
message(STATUS "creating library {boost}")

# Special compiler and linker flags for MSVC
if( MSVC )
    message(STATUS "Applying MSVC flags for Boost")
    add_definitions( /DUNICODE /D_UNICODE /Zc:wchar_t-  )
    add_definitions(-DBOOST_ALL_NO_LIB)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP" )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oi /GL" )
    set_target_properties( boostparts PROPERTIES STATIC_LIBRARY_FLAGS "/LTCG" )
endif()
