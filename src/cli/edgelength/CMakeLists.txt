PROJECT(analyze-edgelength)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)



SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")


# Add library incl1udes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

# Add Source Files
FILE(GLOB srcs *.cpp)
FILE(GLOB hdrs *.h  )


ADD_EXECUTABLE(analyze-edgelength ${srcs} ${hdrs})
TARGET_LINK_LIBRARIES(analyze-edgelength cleaver ) 


