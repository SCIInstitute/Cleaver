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


# newtest(cli_basic cli_common.cc)
# newtest(cli_scale cli_common.cc)
# newtest(cli_alpha cli_common.cc)
# newtest(cli_padding cli_common.cc)
# newtest(cli_regular cli_common.cc)
# newtest(cli_short cli_common.cc)
# newtest(cli_long cli_common.cc)
newtest(cli_strict cli_common.cc)
# newtest(cli_background cli_common.cc)
# newtest(cli_sizing cli_common.cc)
# newtest(cli_grading cli_common.cc)
# newtest(cli_multiplier cli_common.cc)
# newtest(cli_strip cli_common.cc)
# newtest(cli_vtk cli_common.cc)
# newtest(cli_vtkusg cli_common.cc)
# newtest(cli_jacobian cli_common.cc)
# newtest(cli_scirun cli_common.cc)
# newtest(cli_matlab cli_common.cc)

# TODO: Appears to have external dependency. Investigate/Fix.
# newtest(cli_segmentation cli_common.cc)
