INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

ADD_EXECUTABLE(implicit_spheres implicit_spheres.cpp)
TARGET_LINK_LIBRARIES(implicit_spheres cleaver)
