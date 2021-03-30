# gtest external download
set(EXTERNAL_DIR "${CMAKE_BINARY_DIR}/externals")
set(GTEST_INSTALL_DIR "${EXTERNAL_DIR}/googletest")
if(WIN32)
  set(GTEST_SHARED_ARG ON)
else()
  set(GTEST_SHARED_ARG OFF)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC")
endif()

include(ExternalProject)

set(googletest_GIT_TAG "origin/cleaver")

set(_ep_arg_build_byproducts)
if("${CMAKE_GENERATOR}" STREQUAL "Ninja")
  set(_build_byproducts "")
  if(WIN32)
    foreach(_lib IN ITEMS gtest gtest_main gmock gmock_main)
      if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        list(APPEND _build_byproducts ${GTEST_INSTALL_DIR}/lib/lib/${CMAKE_FIND_LIBRARY_PREFIXES}${_lib}d${CMAKE_STATIC_LIBRARY_SUFFIX})
      else()
        list(APPEND _build_byproducts ${GTEST_INSTALL_DIR}/lib/lib/${CMAKE_FIND_LIBRARY_PREFIXES}${_lib}${CMAKE_STATIC_LIBRARY_SUFFIX})
      endif()
    endforeach()
  else()
    foreach(_lib IN ITEMS gtest gtest_main gmock gmock_main)
      list(APPEND _build_byproducts ${GTEST_INSTALL_DIR}/lib/lib/${CMAKE_FIND_LIBRARY_PREFIXES}${_lib}${CMAKE_STATIC_LIBRARY_SUFFIX})
    endforeach()
  endif()
  set(_ep_arg_build_byproducts BUILD_BYPRODUCTS ${_build_byproducts})
endif()

set(_cmake_cache_args)
if(DEFINED CMAKE_BUILD_TYPE)
  list(APPEND _cmake_cache_args -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE})
endif()
ExternalProject_Add(googletest
   PREFIX "${GTEST_INSTALL_DIR}"
   GIT_REPOSITORY "https://github.com/CIBC-Internal/googletest.git"
   GIT_TAG ${googletest_GIT_TAG}
   CMAKE_CACHE_ARGS
     ${_cmake_cache_args}
   INSTALL_DIR "${GTEST_INSTALL_DIR}/include"
   BINARY_DIR "${GTEST_INSTALL_DIR}/lib"
   INSTALL_COMMAND ""
   ${_ep_arg_build_byproducts}
)

# Copy and Specify Include Directory
ExternalProject_Get_Property(googletest source_dir)
ExternalProject_Add_Step(
   googletest
   copy_deps
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir}/googletest/include ${GTEST_INSTALL_DIR}/include
   COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_dir}/googlemock/include ${GTEST_INSTALL_DIR}/include
   DEPENDEES install
)
include_directories("${GTEST_INSTALL_DIR}/include")

# Set up library paths
set(GTEST_LIBRARY gtest)
add_library(${GTEST_LIBRARY} UNKNOWN IMPORTED)
ExternalProject_Get_Property(googletest binary_dir)
if(NOT WIN32)
  set(GTEST_LIBRARY_PATH_OTHER ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_STATIC_LIBRARY_SUFFIX})
  set_target_properties(${GTEST_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GTEST_LIBRARY_PATH_OTHER})
endif()
add_dependencies(${GTEST_LIBRARY} googletest)

set(GTEST_MAIN_LIBRARY gtest_main)
add_library(${GTEST_MAIN_LIBRARY} UNKNOWN IMPORTED)
if(NOT WIN32)
  set(GTEST_MAIN_LIBRARY_PATH_OTHER ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX})
  set_target_properties(${GTEST_MAIN_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY_PATH_OTHER})
endif()
add_dependencies(${GTEST_MAIN_LIBRARY} googletest)

set(GMOCK_LIBRARY gmock)
add_library(${GMOCK_LIBRARY} UNKNOWN IMPORTED)
if(NOT WIN32)
  set(GMOCK_LIBRARY_PATH_OTHER ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gmock${CMAKE_STATIC_LIBRARY_SUFFIX})
  set_target_properties(${GMOCK_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GMOCK_LIBRARY_PATH_OTHER})
endif()
add_dependencies(${GMOCK_LIBRARY} googletest)

set(GMOCK_MAIN_LIBRARY gmock_main)
add_library(${GMOCK_MAIN_LIBRARY} UNKNOWN IMPORTED)
if(NOT WIN32)
  set(GMOCK_MAIN_LIBRARY_PATH_OTHER ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main${CMAKE_STATIC_LIBRARY_SUFFIX})
  set_target_properties(${GMOCK_MAIN_LIBRARY} PROPERTIES IMPORTED_LOCATION ${GMOCK_MAIN_LIBRARY_PATH_OTHER})
endif()
add_dependencies(${GMOCK_MAIN_LIBRARY} ${GTEST_LIBRARY})


# Definitions for Tests
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin)
#test macro
function(NEWTEST name)
  add_executable(${name} ${name}.cpp ${ARGN})
  if(NOT WIN32)
    target_link_libraries(${name} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY} ${GMOCK_LIBRARY} ${GMOCK_MAIN_LIBRARY} cleaver -lpthread)
  else ()
    if (CMAKE_CONFIGURATION_TYPES)
      target_link_libraries(${name}
         ${binary_dir}/lib/$<CONFIG>/${CMAKE_FIND_LIBRARY_PREFIXES}gtest$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
         ${binary_dir}/lib/$<CONFIG>/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
         ${binary_dir}/lib/$<CONFIG>/${CMAKE_FIND_LIBRARY_PREFIXES}gmock$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
         ${binary_dir}/lib/$<CONFIG>/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
          cleaver )
    else()
      # Ninja
      target_link_libraries(${name}
       ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
       ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
       ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gmock$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
       ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
          cleaver )
    endif()
  endif()
  add_test(${name} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name})
  add_dependencies(${name} gtest gtest_main cleaver)
endfunction()

add_subdirectory(cleaver)
add_subdirectory(cli)
