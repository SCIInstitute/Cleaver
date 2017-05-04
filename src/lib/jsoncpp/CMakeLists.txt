PROJECT(jsoncpp)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


# source files
FILE(GLOB JSONCPP_SOURCE_FILES jsoncpp.cpp)
FILE(GLOB JSONCPP_HEADER_FILES json/*.h)
SET(JSONCPP_API_HEADER_FILES
	json/json.h
	json/json-forwards.h
	)


# copy header files over
SET(JSONCPP_INSTALLED_HEADER_FILES)
FOREACH(header_file ${JSONCPP_API_HEADER_FILES})
  GET_FILENAME_COMPONENT(file_no_path "${header_file}" NAME)
  CONFIGURE_FILE("${header_file}"
    "${CMAKE_BINARY_DIR}/include/jsoncpp/${file_no_path}" COPYONLY IMMEDIATE)
  SET(JSONCPP_INSTALLED_HEADER_FILES
    ${JSONCPP_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/jsoncpp/${file_no_path}")
ENDFOREACH(header_file)

# output library
ADD_LIBRARY(jsoncpp STATIC ${JSONCPP_HEADER_FILES} ${JSONCPP_SOURCE_FILES})

