########################################################################
# Tests for Cleaver2
include_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/gtest)
include_directories(${CLEAVER2_SOURCE_DIR}/lib/gtest/include)
link_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
add_definitions(-DBINARY_DIR="${CLEAVER2_BINARY_DIR}/bin/")
add_definitions(-DTEST_DATA_DIR="${CLEAVER2_SOURCE_DIR}/test/test_data/")
#tet mesh
cxx_test(tetmesh_unit_tests gtest_main)
target_link_libraries(tetmesh_unit_tests cleaver)
#cli
cxx_test(cli_basic gtest_main test/cli_common.cc)
cxx_test(cli_scale gtest_main test/cli_common.cc)
cxx_test(cli_alpha gtest_main test/cli_common.cc)
cxx_test(cli_padding gtest_main test/cli_common.cc)
cxx_test(cli_regular gtest_main test/cli_common.cc)
cxx_test(cli_short gtest_main test/cli_common.cc)
cxx_test(cli_long gtest_main test/cli_common.cc)
cxx_test(cli_strict gtest_main test/cli_common.cc)
