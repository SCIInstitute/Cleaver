# Add library includes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

ADD_EXECUTABLE(implicit_spheres implicit_spheres.cpp)
TARGET_LINK_LIBRARIES(implicit_spheres cleaver)

ADD_EXECUTABLE(test_data test_data.cpp)
TARGET_LINK_LIBRARIES(test_data cleaver nrrd2cleaver synthetic)
