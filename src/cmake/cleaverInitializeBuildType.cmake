# This module allows to consistently manage the initialization and setting of build type
# CMake variables.
#
# Adapted from https://github.com/Slicer/Slicer/blob/5.2/CMake/SlicerInitializeBuildType.cmake

# Default build type to use if none was specified
if(NOT DEFINED cleaver_DEFAULT_BUILD_TYPE)
    set(cleaver_DEFAULT_BUILD_TYPE "Release")
endif()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)

    message(STATUS "Setting build type to '${cleaver_DEFAULT_BUILD_TYPE}' as none was specified.")

    set(CMAKE_BUILD_TYPE ${cleaver_DEFAULT_BUILD_TYPE} CACHE STRING "Choose the type of build." FORCE)
    mark_as_advanced(CMAKE_BUILD_TYPE)

    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug"
        "Release"
        "MinSizeRel"
        "RelWithDebInfo"
        )
endif()
