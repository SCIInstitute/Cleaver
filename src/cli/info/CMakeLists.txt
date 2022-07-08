project(mesh-info CXX)

cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/lib)

# Add Source Files
file(GLOB srcs *.cpp )
file(GLOB hdrs *.h   )

add_executable(mesh-info ${srcs} ${hdrs})

target_link_libraries(mesh-info
  cleaver
  )
