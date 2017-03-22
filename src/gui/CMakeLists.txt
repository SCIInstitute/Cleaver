PROJECT(cleaver-gui)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

ADD_DEFINITIONS(-DCLEAVER_VERSION_MAJOR="2")
ADD_DEFINITIONS(-DCLEAVER_VERSION_MINOR="2")

FIND_PACKAGE(OpenGL QUIET)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)
IF(WIN32)
  CMAKE_POLICY(SET CMP0020 OLD)
ENDIF()

# Find QT5
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)

#headers
INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Qt5OpenGL_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Qt5Gui_INCLUDE_DIRS})

#defines
ADD_DEFINITIONS(${Qt5Widgets_DEFINITIONS})
ADD_DEFINITIONS(${Qt5OpenGL_DEFINITIONS})
ADD_DEFINITIONS(${Qt5Core_DEFINITIONS})
ADD_DEFINITIONS(${Qt5Gui_DEFINITIONS})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

# Ensure GUI and OpenGL Work
ADD_DEFINITIONS(-DTETLIBRARY)

SET( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/")

INCLUDE_DIRECTORIES(./Application ./Application/DataWidgets)

FILE(GLOB moc_srcs ./Application/*.cpp ./Application/Data/*.cpp ./Application/DataWidgets/*.cpp ./Application/ToolWidgets/*.cpp ./Application/ViewWidgets/*.cpp)
FILE(GLOB moc_hdrs ./Application/MainWindow.h   ./Application/Data/*.h   ./Application/DataWidgets/*.h   ./Application/ToolWidgets/*.h   ./Application/ViewWidgets/MeshViewOptionsWidget.h ./Application/ViewWidgets/MeshWindow.h)

# Add QT Files
FILE(GLOB Forms_UIS ./Application/*.ui ./Application/DataWidgets/*.ui ./Application/ToolWidgets/*.ui ./Application/ViewWidgets/*.ui)
FILE(GLOB RCC_FILES ./Resources/*.qrc)
FILE(GLOB APP_ICNS Resources/Icons/*.ico)
FILE(GLOB WINDOW_ICNS Resources/Icons/cleaverIcon.png)
ADD_DEFINITIONS(-DWINDOW_ICON="${CMAKE_SOURCE_DIR}/gui/Resources/Icons/cleaverIcon.png")

# Add library incl1udes
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/nrrd2cleaver)

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
ELSEIF(WIN32)
  ADD_DEFINITIONS(-DICON_RC_FILE="${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icons/cleaverIcon.ico")
  SET(srcs ${srcs} ${CMAKE_CURRENT_SOURCE_DIR}/Resources/cleaver-gui.rc)
ENDIF(APPLE)
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")
  ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  ENDIF()
ENDIF()


ADD_EXECUTABLE(cleaver-gui MACOSX_BUNDLE WIN32
   ${srcs} ${hdrs}
   ${moc_srcs} ${MOC_SRC}
   ${UI_SRC} ${RCC_SRC}
   ${APP_ICNS}
   ${WINDOW_ICNS}
   )

IF(WIN32)
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
ELSE()
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
ENDIF()

INSTALL(TARGETS cleaver-gui
  BUNDLE DESTINATION . COMPONENT Runtime
  RUNTIME DESTINATION bin COMPONENT Runtime
  )
