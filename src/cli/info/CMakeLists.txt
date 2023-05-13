project(mesh-info CXX)

cmake_minimum_required(VERSION 3.10.2)

# Add library includes
include_directories(${CLEAVER2_SOURCE_DIR}/lib)

set(srcs
  main.cpp
  )

add_executable(mesh-info ${srcs})

target_link_libraries(mesh-info
  cleaver
  )
