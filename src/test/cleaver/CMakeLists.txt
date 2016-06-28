########################################################################

# Tests for Cleaver2
include_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
link_directories(${CLEAVER2_SOURCE_DIR}/lib/cleaver)

# mesher unit tests
newtest(mesher_unit_tests)
#vec3 unit tests
newtest(vec3_unit_tests)
#tet mesh
newtest(tetmesh_unit_tests)
