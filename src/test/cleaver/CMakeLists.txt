########################################################################

# Tests for Cleaver2
INCLUDE_DIRECTORIES(${CLEAVER2_SOURCE_DIR}/lib/cleaver)
LINK_DIRECTORIES(${CLEAVER2_SOURCE_DIR}/lib/cleaver)

newtest(vec3_unit_tests)
newtest(tetmesh_unit_tests)
newtest(linearviolationchecker_tests)
newtest(mesher_unit_tests)
