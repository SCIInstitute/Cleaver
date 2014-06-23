PROJECT(cleaver-gui)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)


# Find QT4, OpenGL
FIND_PACKAGE(BLAS QUIET)
FIND_PACKAGE(Qt4 QUIET)
FIND_PACKAGE(OpenGL QUIET)

if(NOT QT4_FOUND)


    #MESSAGE(STATUS "QT_LIBRARY_DIR = ${QT_LIBRARY_DIR}")
    #MESSAGE(STATUS "QT_INCLUDE_DIR = ${QT_INCLUDE_DIR}")
    #MESSAGE(STATUS "QT_MOC_EXECUTABLE  = ${QT_MOC_EXECUTABLE}")
    #MESSAGE(STATUS "QT_UIC_EXECUTABLE = ${QT_UIC_EXECUTABLE}")
    #MESSAGE(STATUS "QT_RCC_EXECUTABLE = ${QT_RCC_EXECUTABLE}")
    #MESSAGE(STATUS "QT_QTCORE_LIBRARY = ${QT_QTCORE_LIBRARY}")

    MESSAGE(STATUS "Unable to find Qt4. Cannot build Cleaver GUI")
    return()
endif()

if(WIN32)
  FIND_PACKAGE(GLEW)
endif()

OPTION(BUILD_CLEAVER_APP "Build Cleaver GUI App (REQUIRES TEEM)" ON)

# Ensure GUI and OpenGL Work
SET( QT_USE_QTGUI TRUE )
SET( QT_USE_QTOPENGL TRUE )
SET( QT_USE_THREAD TRUE )
SET( QT_USE_QTXML TRUE )
ADD_DEFINITIONS(-DTETLIBRARY)

# Qt Includes
INCLUDE( ${QT_USE_FILE} )

SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

INCLUDE_DIRECTORIES(./Application ./Application/DataWidgets)

FILE(GLOB moc_srcs ./Application/*.cpp ./Application/Data/*.cpp ./Application/DataWidgets/*.cpp ./Application/ToolWidgets/*.cpp ./Application/ViewWidgets/*.cpp)
FILE(GLOB moc_hdrs ./Application/*.h   ./Application/Data/*.h   ./Application/DataWidgets/*.h   ./Application/ToolWidgets/*.h   ./Application/ViewWidgets/*.h)

# Add QT Files
FILE(GLOB Forms_UIS ./Application/*.ui ./Application/DataWidgets/*.ui ./Application/ToolWidgets/*.ui ./Application/ViewWidgets/*.ui)
FILE(GLOB RCC_FILES ./Resources/*.qrc)

# Add library incl1udes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

# Add Source Files
FILE(GLOB srcs *.cpp ./Main/*.cpp )
FILE(GLOB hdrs *.h   ./Main/*.h   )

# Add Shader Files
FILE(GLOB vert ./Application/Shaders/*.vert)
FILE(GLOB frag ./Application/Shaders/*.frag)
FILE(GLOB vf_h ./Application/Shaders/*.h)

#MACRO(compile_shader arg1 arg2)
#    GET_FILENAME_COMPONENT(shader_path ${arg1} ABSOLUTE)
#    GET_FILENAME_COMPONENT(shader_name ${arg1} NAME_WE)
#    GET_FILENAME_COMPONENT(shader_ext ${arg1} EXT)
#    SET(shader_file "${shader_name}${shader_ext}")
#    if(shader_ext STREQUAL ".vert")
#        SET(${arg2} "${shader_name}_vert.h")
#        ADD_CUSTOM_COMMAND(
#            OUTPUT  ${arg2}
#            #MESSAGE(STATUS "Compiling ${arg1} into ${arg2}")
#            COMMAND ${CMAKE_COMMAND} -E copy ${arg1} ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file}
#            COMMAND xxd -i ${shader_file} ${arg2}
#            DEPENDS ${arg1}
#        )
#    elseif(shader_ext STREQUAL ".frag")
#        SET(${arg2} "${shader_name}_frag.h")
#        ADD_CUSTOM_COMMAND(
#            OUTPUT  ${arg2}
#            COMMAND ${CMAKE_COMMAND} -E copy ${arg1} ${CMAKE_CURRENT_SOURCE_DIR}/${shader_file}
#            COMMAND xxd -i ${shader_file} ${arg2}
#            DEPENDS ${arg1}
#        )
#    else()
#        MESSAGE(STATUS "invalid shader given to function Compile_Shader()")
#    endif()
#ENDMACRO()

#COMPILE_SHADER("default.vert" default_vertex_shader)
#MESSAGE(STATUS "default_shader = ${default_vertex_shader}")

#COMPILE_SHADER("default.frag" default_fragment_shader)
#MESSAGE(STATUS "default_shader = ${default_fragment_shader}")


ADD_CUSTOM_COMMAND(
    OUTPUT  default_vert.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Application/Shaders/default.vert ${CMAKE_CURRENT_SOURCE_DIR}/default.vert
    COMMAND xxd -i default.vert default_vert.h
    DEPENDS ./Application/Shaders/default.vert
)

ADD_CUSTOM_COMMAND(
    OUTPUT  default_frag.h
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/Application/Shaders/default.frag ${CMAKE_CURRENT_SOURCE_DIR}/default.frag
    COMMAND xxd -i default.frag default_frag.h
    DEPENDS ./Application/Shaders/default.frag
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ./)

# Generate QT Stuff
QT4_WRAP_UI(UI_SRC ${Forms_UIS})
QT4_WRAP_CPP(MOC_SRC ${moc_hdrs})
QT4_ADD_RESOURCES(RCC_SRC ${RCC_FILES})



ADD_CUSTOM_TARGET(comp_shaders
                  DEPENDS ${default_vertex_shader} ${default_fragment_shader})

ADD_EXECUTABLE(cleaver-gui ${srcs} ${hdrs} ${moc_srcs} ${MOC_SRC} ${UI_SRC} ${RCC_SRC} ${vert} ${frag} ${vf_h})

if(USE_STELLAR)
   set (OPTIONAL_LIBS ${OPTIONAL_LIBS} stellar)
endif()
if(USE_BLAS)
   SET (OPTIONAL_LIBS ${OPTIONAL_LIBS} poisson ${BLAS_LIBRARIES})
endif()

if(WIN32)
TARGET_LINK_LIBRARIES(cleaver-gui cleaver nrrd2cleaver teem synthetic tetgen ${OPENGL_LIBRARIES} ${QT_LIBRARIES} ${GLEW_LIBRARIES} )
else()
TARGET_LINK_LIBRARIES(cleaver-gui cleaver nrrd2cleaver teem synthetic tetgen ${OPTIONAL_LIBS} ${OPENGL_LIBRARIES} ${QT_LIBRARIES} )
endif()

ADD_DEPENDENCIES(cleaver-gui comp_shaders)
