# gtest external download
SET(EXTERNAL_DIR "${CMAKE_BINARY_DIR}/externals")
SET(GTEST_INSTALL_DIR "${EXTERNAL_DIR}/gtest")
IF(WIN32)
  SET(GTEST_SHARED_ARG ON)
ELSE()
  SET(GTEST_SHARED_ARG OFF)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
ENDIF()
INCLUDE(ExternalProject)

ExternalProject_Add( gtest
  PREFIX "${GTEST_INSTALL_DIR}"
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  INSTALL_DIR "${GTEST_INSTALL_DIR}/include"
  BINARY_DIR "${GTEST_INSTALL_DIR}/lib"
  INSTALL_COMMAND ""
  CMAKE_ARGS
  -Dgtest_build_tests:BOOL=OFF
  -DBUILD_GMOCK:BOOL=OFF
  -DBUILD_GTEST:BOOL=ON
  -Dgtest_force_shared_crt:BOOL=${GTEST_SHARED_ARG}
  )
ExternalProject_Add_Step( gtest copy_deps
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${GTEST_INSTALL_DIR}/src/gtest/googletest/include ${GTEST_INSTALL_DIR}/include
  DEPENDEES install
  )
INCLUDE_DIRECTORIES("${GTEST_INSTALL_DIR}/include")
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../core2d/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../core3d/include)
SET(GTEST_LIBRARY "${GTEST_INSTALL_DIR}/lib/googletest/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX};${GTEST_INSTALL_DIR}/lib/googletest/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}")
########################################################################

# Definitions for Tests
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin)
#test macro
function(NEWTEST name)
  ADD_EXECUTABLE(${name} ${name}.cc)
  IF(NOT WIN32)
    TARGET_LINK_LIBRARIES(${name} ${GTEST_LIBRARY} cleaver -lpthread)
  ELSE ()
    TARGET_LINK_LIBRARIES(${name} ${GTEST_LIBRARY} cleaver )
  ENDIF()
  ADD_TEST(${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name})
  ADD_DEPENDENCIES(${name} gtest cleaver)
ENDFUNCTION()

ADD_SUBDIRECTORY(cli)
ADD_SUBDIRECTORY(cleaver)
