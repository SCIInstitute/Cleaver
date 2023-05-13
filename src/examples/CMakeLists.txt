# Add library includes
include_directories(${CLEAVER2_BINARY_DIR}/include)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/nrrd2cleaver)
include_directories(${CLEAVER2_SOURCE_DIR}/lib)

add_executable(implicit_spheres implicit_spheres.cpp)
target_link_libraries(implicit_spheres cleaver)

add_executable(test_data test_data.cpp)
target_link_libraries(test_data cleaver nrrd2cleaver synthetic)
