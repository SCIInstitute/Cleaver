project(cleaver-cli CXX)

cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

# Add Source Files
file(GLOB srcs *.cpp )
file(GLOB hdrs *.h   )

add_executable(cleaver-cli ${srcs} ${hdrs})

target_link_libraries(cleaver-cli
  cleaver
  nrrd2cleaver
  boostparts
  )

install(TARGETS cleaver-cli
  RUNTIME DESTINATION ${CLEAVER2_RUNTIME_DIR} COMPONENT RuntimeCLI
  )
