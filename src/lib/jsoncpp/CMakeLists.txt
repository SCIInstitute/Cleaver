project(jsoncpp)

cmake_minimum_required(VERSION 2.6)


# source files
file(GLOB JSONCPP_SOURCE_FILES jsoncpp.cpp)
file(GLOB JSONCPP_HEADER_FILES json/*.h)
set(JSONCPP_API_HEADER_FILES
	json/json.h
	json/json-forwards.h
	)


# copy header files over
set(JSONCPP_INSTALLED_HEADER_FILES)
foreach(header_file ${JSONCPP_API_HEADER_FILES})
  get_filename_component(file_no_path "${header_file}" NAME)
  configure_file("${header_file}"
    "${CMAKE_BINARY_DIR}/include/jsoncpp/${file_no_path}" COPYONLY IMMEDIATE)
  set(JSONCPP_INSTALLED_HEADER_FILES
    ${JSONCPP_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/jsoncpp/${file_no_path}")
endforeach(header_file)

# output library
add_library(jsoncpp STATIC ${JSONCPP_HEADER_FILES} ${JSONCPP_SOURCE_FILES})

