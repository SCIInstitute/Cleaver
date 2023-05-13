project(cleaver-cli CXX)

cmake_minimum_required(VERSION 3.10.2)

# Add library includes
include_directories(${CLEAVER2_SOURCE_DIR}/lib)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/nrrd2cleaver)

set(srcs
  main.cpp
  )

add_executable(cleaver-cli ${srcs})

target_link_libraries(cleaver-cli
  cleaver
  nrrd2cleaver
  )

install(TARGETS cleaver-cli
  RUNTIME DESTINATION ${CLEAVER2_RUNTIME_DIR} COMPONENT RuntimeCLI
  )
