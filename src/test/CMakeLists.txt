########################################################################
# Tests for Cleaver2
SET(TEST_DATA_DIR ${CLEAVER2_SOURCE_DIR}/test/test_data)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/gtest)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/gtest/include)
link_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
#tet mesh
cxx_test(tetmesh_unit_tests gtest_main)
target_link_libraries(tetmesh_unit_tests cleaver)
#cli
#cxx_executable(cli_tests tests/cases gtest_main)
#target_link_libraries(cli_tests cleaver-cli)
#ADD_TEST(cli_tests ${CMAKE_BUILD_DIR}/bin/cli_tests)
