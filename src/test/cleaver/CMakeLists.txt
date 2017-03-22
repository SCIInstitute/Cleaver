########################################################################

# Tests for Cleaver2
INCLUDE_DIRECTORIES(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
LINK_DIRECTORIES(${CLEAVER2_SOURCE_DIR}/lib/cleaver)

# mesher unit tests
newtest(mesher_unit_tests)
#vec3 unit tests
newtest(vec3_unit_tests)
#tet mesh
newtest(tetmesh_unit_tests)
