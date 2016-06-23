PROJECT(cleaver-gui)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

add_definitions(-DCLEAVER_VERSION_MAJOR="2")
add_definitions(-DCLEAVER_VERSION_MINOR="1")
if(WIN32)
  add_definitions(-DWIN32_ICON_FILE="${CMAKE_SOURCE_DIR}/gui/Resources/Icons/cleaverIcon.ico")
endif()

FIND_PACKAGE(OpenGL QUIET)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
if(WIN32)
  cmake_policy(SET CMP0020 OLD)
endif()

# Find QT5
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)

#headers
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5OpenGL_INCLUDE_DIRS})
include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})

#defines
add_definitions(${Qt5Widgets_DEFINITIONS})
add_definitions(${Qt5OpenGL_DEFINITIONS})
add_definitions(${Qt5Core_DEFINITIONS})
add_definitions(${Qt5Gui_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

# Ensure GUI and OpenGL Work
ADD_DEFINITIONS(-DTETLIBRARY)

SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

INCLUDE_DIRECTORIES(./Application ./Application/DataWidgets)

FILE(GLOB moc_srcs ./Application/*.cpp ./Application/Data/*.cpp ./Application/DataWidgets/*.cpp ./Application/ToolWidgets/*.cpp ./Application/ViewWidgets/*.cpp)
FILE(GLOB moc_hdrs ./Application/MainWindow.h   ./Application/Data/*.h   ./Application/DataWidgets/*.h   ./Application/ToolWidgets/*.h   ./Application/ViewWidgets/MeshViewOptionsWidget.h ./Application/ViewWidgets/MeshWindow.h)

# Add QT Files
FILE(GLOB Forms_UIS ./Application/*.ui ./Application/DataWidgets/*.ui ./Application/ToolWidgets/*.ui ./Application/ViewWidgets/*.ui)
FILE(GLOB RCC_FILES ./Resources/*.qrc)

# Add library incl1udes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

# Add Source Files
FILE(GLOB srcs *.cpp ./Main/*.cpp )
FILE(GLOB hdrs *.h   ./Main/*.h   )

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ./)

# Generate QT Stuff
QT5_WRAP_UI(UI_SRC ${Forms_UIS})
QT5_ADD_RESOURCES(RCC_SRC ${RCC_FILES})

#icon
# For Apple set the icns file containing icons
IF(APPLE)
  # set how it shows up in the Info.plist file
  SET(MACOSX_BUNDLE_ICON_FILE cleaverIcon.icns)
  # set where in the bundle to put the icns file
  SET_SOURCE_FILES_PROPERTIES(${CMAKE_CURRENT_SOURCE_DIR}/Resources/cleaverIcon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # include the icns file in the target
  SET(srcs ${srcs} ${CMAKE_CURRENT_SOURCE_DIR}/Resources/cleaverIcon.icns)
ENDIF(APPLE)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()
endif()


ADD_EXECUTABLE(cleaver-gui MACOSX_BUNDLE  ${srcs} ${hdrs} ${moc_srcs} ${MOC_SRC} ${UI_SRC} ${RCC_SRC})

if(WIN32)
  TARGET_LINK_LIBRARIES(cleaver-gui
    cleaver
    nrrd2cleaver
    ${OPENGL_LIBRARIES}
    ${QT_LIBRARIES}
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Core
    Qt5::Gui
    Qt5::WinMain
    )
else()
  TARGET_LINK_LIBRARIES(cleaver-gui
    cleaver
    nrrd2cleaver
    ${OPENGL_LIBRARIES}
    ${QT_LIBRARIES}
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Core
    Qt5::Gui
    ${OPTIONAL_LIBS})
endif()

INSTALL(TARGETS cleaver-gui
  BUNDLE DESTINATION . COMPONENT Runtime
  RUNTIME DESTINATION bin COMPONENT Runtime
  )
