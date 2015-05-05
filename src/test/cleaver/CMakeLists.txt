########################################################################

# Tests for Cleaver2
include_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/gtest)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/gtest/include)
link_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)

# mesher unit tests
cxx_test(mesher_unit_tests gtest_main)
target_link_libraries(mesher_unit_tests cleaver)

#vec3 unit tests
cxx_test(vec3_unit_tests gtest_main)
target_link_libraries(vec3_unit_tests cleaver)

#tet mesh
cxx_test(tetmesh_unit_tests gtest_main)
target_link_libraries(tetmesh_unit_tests cleaver)
