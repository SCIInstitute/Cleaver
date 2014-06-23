PROJECT(blobbies)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

# Add library incl1udes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

# Add Source Files
FILE(GLOB srcs *.cpp )
FILE(GLOB hdrs *.h   )

ADD_EXECUTABLE(blobbies ${srcs} ${hdrs})

TARGET_LINK_LIBRARIES(blobbies synthetic cleaver nrrd2cleaver teem particle_lattice tetgen boostparts )




