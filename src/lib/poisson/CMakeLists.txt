PROJECT(Poisson CXX Fortran)

#enable_language( Fortran )

cmake_minimum_required(VERSION 2.6.2)

#set(CMAKE_BUILD_TYPE release)
SET(CMAKE_BUILD_TYPE distribution)
SET(CMAKE_CXX_FLAGS_DISTRIBUTION "-O2")
SET(CMAKE_C_FLAGS_DISTRIBUTION "-O2")


find_package(Qt4 REQUIRED)
find_package(CUDA QUIET)
find_package(GLUT)
find_package(OpenGL REQUIRED)
find_package(BLAS QUIET)

if(BLAS_FOUND AND USE_BLAS)
    ADD_DEFINITIONS(-DBLAS_SUPPORT)
endif()


set( QT_USE_QTGUI TRUE )
set( QT_USE_QTOPENGL TRUE )
set( QT_USE_THREAD TRUE )

# Qt Includes
#INCLUDE( ${QT_USE_FILE} )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/" )
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
INCLUDE_DIRECTORIES(./qt-gui)
#INCLUDE_DIRECTORIES(./shaders)

# Add Non-Gui Files
FILE(GLOB cpp_src_files *.c*)
FILE(GLOB cpp_hdr_files *.h)

# copy header files over
SET(Poisson_API_HEADER_FILES ${cpp_hdr_files})
SET(Poisson INSTALLED_HEADER_FILES)
FOREACH(header_file ${Poisson_API_HEADER_FILES})
  # When debugging, uncomment this line
  #MESSAGE(STATUS "Copy header file: ${header_file}")
  GET_FILENAME_COMPONENT(file_no_path "${header_file}" NAME)
  CONFIGURE_FILE("${header_file}"
    "${CMAKE_BINARY_DIR}/include/Poisson/${file_no_path}" COPYONLY IMMEDIATE)
  SET(Poisson_INSTALLED_HEADER_FILES
    ${Poisson_INSTALLED_HEADER_FILES}
    "${CMAKE_BINARY_DIR}/include/Poisson/${file_no_path}")
ENDFOREACH(header_file)


if(CUDA_CUDA_LIBRARY AND FALSE)
    message(STATUS "CUDA Found on this system")

    IF(CUDA_FOUND)
        ADD_DEFINITIONS(-DCUDA_SUPPORT)
    ENDIF(CUDA_FOUND)

    FILE(GLOB cuda_src_files *.cu)
    FILE(GLOB cuda_hdr_files *.cuh)


    # Add Shaders to Project (For Easy IDE Editing) Doesn't Seem to Work...
    # FILE(GLOB vertex_shader_files ./shaders/*.vert)
    # FILE(GLOB fragment_shader_files ./shaders/*.frag)

    find_path(CUDA_CUT_INCLUDE_DIR
              cutil.h
              PATHS ${CUDA_SDK_ROOT_DIR}
              PATH_SUFFIXES "C/common/inc"
              DOC "Location of cutil.h"
              NO_DEFAULT_PATH
              )
    # Now search system paths
    find_path(CUDA_CUT_INCLUDE_DIR cutil.h DOC "Location of cutil.h")

    mark_as_advanced(CUDA_CUT_INCLUDE_DIR)


    #include directory of MOC'd files
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ./)

    SET(ALL_CUDA_LIBRARIES ${CUDA_CUT_LIBRARY} ${CUDA_CUDART_LIBRARY} ${CUDA_CUBLAS_LIBRARIES} /usr/local/cuda/lib/libcusparse.dylib)

    #this block sets up the shared library called kernel
    SET(CUDA_NVCC_FLAGS -arch compute_12 --shared  --compiler-options;'-fPIC' )
    INCLUDE_DIRECTORIES(${CUDA_INCLUDE_DIRS} ${CUDA_CUT_INCLUDE_DIR})
    cuda_add_library(kernel SHARED ${cuda_src_files} ${cuda_hdr_files})
    list(REMOVE_DUPLICATES ALL_CUDA_LIBRARIES)
    target_link_libraries(kernel ${ALL_CUDA_LIBRARIES})

    #include_directories(${CMAKE_SOURCE_DIR})
    #add_executable(Composites ${c_src_files} ${cpp_src_files} ${c_hdr_files} ${qt-gui_srcs} ${MOC_SRC} ${UI_SRC} ${RCC_SRC})
    #set (libs_to_link kernel ${OPENGL_LIBRARIES} ${BLAS_LIBRARIES})
    ADD_LIBRARY(poisson ${cpp_src_files} ${cpp_hdr_files} ${cuda_src_files} ${cuda_hdr_files})
    target_link_libraries(poisson ${libs_to_link})

else()
    ADD_LIBRARY(poisson ${cpp_src_files} ${cpp_hdr_files})
    SET_TARGET_PROPERTIES(poisson PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(poisson ${BLAS_LIBRARIES})
endif(CUDA_CUDA_LIBRARY AND FALSE)




