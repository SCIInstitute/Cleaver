# gtest external download
SET(EXTERNAL_DIR "${CMAKE_BINARY_DIR}/externals")
SET(GTEST_INSTALL_DIR "${EXTERNAL_DIR}/googletest")
IF(WIN32)
  SET(GTEST_SHARED_ARG ON)
ELSE()
  SET(GTEST_SHARED_ARG OFF)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
ENDIF()

INCLUDE(ExternalProject)

ExternalProject_Add(googletest
   PREFIX "${GTEST_INSTALL_DIR}"
   GIT_REPOSITORY "https://github.com/google/googletest.git"
   INSTALL_DIR "${GTEST_INSTALL_DIR}/include"
   BINARY_DIR "${GTEST_INSTALL_DIR}/lib"
   INSTALL_COMMAND ""
)

# Copy and Specfy Include Directory
ExternalProject_Get_Property(googletest source_dir)
ExternalProject_Add_Step(
   googletest
   copy_deps
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir}/googletest/include ${GTEST_INSTALL_DIR}/include
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir}/googlemock/include ${GTEST_INSTALL_DIR}/include
   DEPENDEES install
)
INCLUDE_DIRECTORIES("${GTEST_INSTALL_DIR}/include")


# Set up librarie paths
ExternalProject_Get_Property(googletest binary_dir)
SET(GTEST_LIBRARY_PATH ${binary_dir}/googlemock/gtest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
SET(GTEST_LIBRARY gtest)
ADD_LIBRARY(${GTEST_LIBRARY} UNKNOWN IMPORTED)
set_target_properties(${GTEST_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GTEST_LIBRARY_PATH})
add_dependencies(${GTEST_LIBRARY} googletest)

set(GTEST_MAIN_LIBRARY_PATH ${binary_dir}/googlemock/gtest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GTEST_MAIN_LIBRARY gtest_main)
add_library(${GTEST_MAIN_LIBRARY} UNKNOWN IMPORTED)
set_target_properties(${GTEST_MAIN_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY_PATH})
add_dependencies(${GTEST_MAIN_LIBRARY} googletest)

set(GMOCK_LIBRARY_PATH ${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GMOCK_LIBRARY gmock)
add_library(${GMOCK_LIBRARY} UNKNOWN IMPORTED)
set_target_properties(${GMOCK_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GMOCK_LIBRARY_PATH})
add_dependencies(${GMOCK_LIBRARY} googletest)

set(GMOCK_MAIN_LIBRARY_PATH ${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GMOCK_MAIN_LIBRARY gmock_main)
add_library(${GMOCK_MAIN_LIBRARY} UNKNOWN IMPORTED)
set_target_properties(${GMOCK_MAIN_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GMOCK_MAIN_LIBRARY_PATH})
add_dependencies(${GMOCK_MAIN_LIBRARY} ${GTEST_LIBRARY})


# Definitions for Tests
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin)
#test macro
function(NEWTEST name)
  ADD_EXECUTABLE(${name} ${name}.cpp ${ARGN})
  IF(NOT WIN32)
    TARGET_LINK_LIBRARIES(${name} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} ${GMOCK_MAIN_LIBRARY} cleaver -lpthread)
  ELSE ()
    TARGET_LINK_LIBRARIES(${name} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} ${GMOCK_MAIN_LIBRARY} cleaver )
  ENDIF()
  ADD_TEST(${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name})
  ADD_DEPENDENCIES(${name} gtest gtest_main cleaver)
ENDFUNCTION()

ADD_SUBDIRECTORY(cleaver)
ADD_SUBDIRECTORY(cli)

