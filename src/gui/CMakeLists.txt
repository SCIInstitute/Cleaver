project(cleaver-gui)

cmake_minimum_required(VERSION 3.10.2)

add_definitions(-DCLEAVER_VERSION_MAJOR="2")
add_definitions(-DCLEAVER_VERSION_MINOR="4")

find_package(OpenGL QUIET)

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
add_definitions(-DTETLIBRARY)

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

include_directories(./Application ./Application/DataWidgets)

file(GLOB moc_srcs ./Application/*.cpp ./Application/Data/*.cpp ./Application/DataWidgets/*.cpp ./Application/ToolWidgets/*.cpp ./Application/ViewWidgets/*.cpp)
file(GLOB moc_hdrs ./Application/MainWindow.h   ./Application/Data/*.h   ./Application/DataWidgets/*.h   ./Application/ToolWidgets/*.h   ./Application/ViewWidgets/MeshViewOptionsWidget.h ./Application/ViewWidgets/MeshWindow.h)

# Add QT Files
file(GLOB Forms_UIS ./Application/*.ui ./Application/DataWidgets/*.ui ./Application/ToolWidgets/*.ui ./Application/ViewWidgets/*.ui)
file(GLOB RCC_FILES ./Resources/*.qrc)
file(GLOB APP_ICNS Resources/Icons/*.ico)
file(GLOB WINDOW_ICNS Resources/Icons/cleaverIcon.png)
add_definitions(-DWINDOW_ICON="${CMAKE_SOURCE_DIR}/gui/Resources/Icons/cleaverIcon.png")

# Add library includes
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

# Add Source Files
file(GLOB srcs *.cpp ./Main/*.cpp )
file(GLOB hdrs *.h   ./Main/*.h   )

include_directories(${CMAKE_CURRENT_BINARY_DIR} ./)

# Generate QT Stuff
QT5_WRAP_UI(UI_SRC ${Forms_UIS})
QT5_ADD_RESOURCES(RCC_SRC ${RCC_FILES})

#icon
# For Apple set the icns file containing icons
if(APPLE)
  # set how it shows up in the Info.plist file
  set(MACOSX_BUNDLE_ICON_FILE cleaverIcon.icns)
  # set where in the bundle to put the icns file
  set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/Resources/cleaverIcon.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # include the icns file in the target
  set(srcs ${srcs} ${CMAKE_CURRENT_SOURCE_DIR}/Resources/cleaverIcon.icns)
elseif(WIN32)
  add_definitions(-DICON_RC_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/cleaverIcon.ico")
  set(srcs ${srcs} ${CMAKE_CURRENT_SOURCE_DIR}/Resources/cleaver-gui.rc)
endif()

add_executable(cleaver-gui MACOSX_BUNDLE WIN32
   ${srcs} ${hdrs}
   ${moc_srcs} ${MOC_SRC}
   ${UI_SRC} ${RCC_SRC}
   ${APP_ICNS}
   ${WINDOW_ICNS}
   )

if(WIN32)
  target_link_libraries(cleaver-gui
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
  target_link_libraries(cleaver-gui
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

install(TARGETS cleaver-gui
  BUNDLE DESTINATION ${CLEAVER2_BUNDLE_DIR} COMPONENT Runtime
  RUNTIME DESTINATION ${CLEAVER2_RUNTIME_DIR} COMPONENT Runtime
  )
