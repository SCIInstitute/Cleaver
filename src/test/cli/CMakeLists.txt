########################################################################
IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  ADD_DEFINITIONS(-DAPPLE)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  ADD_DEFINITIONS(-DWIN32)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  ADD_DEFINITIONS(-DLINUX)
endif()

# Tests for cli
ADD_DEFINITIONS(-DBINARY_DIR="${CLEAVER2_BINARY_DIR}/bin/")
ADD_DEFINITIONS(-DTEST_DATA_DIR="${CLEAVER2_SOURCE_DIR}/test/test_data/")


# newtest(cli_basic cli_common.cpp)
# newtest(cli_scale cli_common.cpp)
# newtest(cli_alpha cli_common.cpp)
# newtest(cli_padding cli_common.cpp)
# newtest(cli_regular cli_common.cpp)
# newtest(cli_short cli_common.cpp)
# newtest(cli_long cli_common.cpp)
newtest(cli_strict cli_common.cpp)
# newtest(cli_background cli_common.cpp)
# newtest(cli_sizing cli_common.cpp)
# newtest(cli_grading cli_common.cpp)
# newtest(cli_multiplier cli_common.cpp)
# newtest(cli_strip cli_common.cpp)
# newtest(cli_vtk cli_common.cpp)
# newtest(cli_vtkusg cli_common.cpp)
# newtest(cli_jacobian cli_common.cpp)
# newtest(cli_scirun cli_common.cpp)
# newtest(cli_matlab cli_common.cpp)

# TODO: Appears to have external dependency. Investigate/Fix.
# newtest(cli_segmentation cli_common.cpp)
