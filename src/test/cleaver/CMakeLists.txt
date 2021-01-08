########################################################################

# Tests for Cleaver2
include_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
link_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)

newtest(vec3_unit_tests)
newtest(tet_unit_tests)
newtest(tetmesh_unit_tests)
newtest(linearviolationchecker_tests)
newtest(mesher_unit_tests)
