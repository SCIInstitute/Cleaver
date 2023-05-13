project(sfcreator CXX)

cmake_minimum_required(VERSION 3.10.2)

# Add library includes
include_directories(${CLEAVER2_SOURCE_DIR}/lib)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/nrrd2cleaver)

set(srcs
  main.cpp
  )

add_executable(sfcreator ${srcs})

target_link_libraries(sfcreator cleaver nrrd2cleaver)
