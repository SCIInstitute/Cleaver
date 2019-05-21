PROJECT(cleaver-gui)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

###########################################
#CLEAVER VERSION NUMBERS
###########################################

ADD_DEFINITIONS(-DCLEAVER_VERSION_MAJOR="2")
ADD_DEFINITIONS(-DCLEAVER_VERSION_MINOR="4")

SET(CLEAVER2_VERSION_MAJOR 2)
SET(CLEAVER2_VERSION_MINOR 4)

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

# Add library includes
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
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
  BUNDLE DESTINATION ${CLEAVER2_BUNDLE_DIR} COMPONENT Runtime
  RUNTIME DESTINATION ${CLEAVER2_RUNTIME_DIR} COMPONENT Runtime
  )

###########################################
#APPLICATION Information
###########################################

SET(APPLICATION_NAME "Cleaver2")
SET(APPLICATION_VENDOR "SCI")

###########################################
#PACKAGING Information
###########################################
SET(PACKAGING_README "${CMAKE_SOURCE_DIR}/gui/CMake/ReadMe.txt")
SET(PACKAGING_LICENSE "${CMAKE_SOURCE_DIR}/gui/CMake/License.txt")
SET(PACKAGING_WELCOME "${CMAKE_SOURCE_DIR}/gui/CMake/Welcome.txt")
SET(PACKAGING_ICON "${CMAKE_SOURCE_DIR}/gui/Resources/Icons\\\\cleaverIcon.png")

###########################################
#Packaging:
#This part sets up the packager using CPack
###########################################
#TODO: Add Windows packaging
IF(APPLE)
  MESSAGE(STATUS "Configuring Cleaver2 packaging")

  INCLUDE(InstallRequiredSystemLibraries)

  SET(CPACK_PACKAGE_NAME "${APPLICATION_NAME}")

  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APPLICATION_NAME}")
  SET(CPACK_PACKAGE_VENDOR "${APPLICATION_VENDOR}")

  SET(CPACK_PACKAGE_DESCRIPTION_FILE ${PACKAGING_README} )
  SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APPLICATION_NAME} ${CLEAVER2_VERSION_MAJOR}.${CLEAVER2_VERSION_MINOR} is a multi-material tetrahedral meshing program.")

  SET(CPACK_RESOURCE_FILE_LICENSE ${PACKAGING_LICENSE})
  SET(CPACK_RESOURCE_FILE_WELCOME ${PACKAGING_WELCOME})

  SET(CPACK_PACKAGE_VERSION_MAJOR "${CLEAVER2_VERSION_MAJOR}")
  SET(CPACK_PACKAGE_VERSION_MINOR "${CLEAVER2_VERSION_MINOR}")
  SET(CPACK_PACKAGE_VERSION "${CLEAVER2_VERSION_MAJOR}.${CLEAVER2_VERSION_MINOR}")

  SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${APPLICATION_NAME}_${CLEAVER2_VERSION_MAJOR}.${CLEAVER2_VERSION_MINOR}")

  ############
  #Apple only
  ############

  SET(CPACK_GENERATOR "productbuild;TGZ")
  SET(CPACK_TOPLEVEL_TAG "")
  IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CPACK_INSTALL_PREFIX "/")
    SET(CMAKE_INSTALL_PREFIX "/")
  ENDIF()
  # productbuild packaging repair hack
  # call repair_package.sh after running package target in build
  # Tip: can inspect packages using the pkgutil --expand command
  IF(CPACK_GENERATOR MATCHES "productbuild")
    # make sure there are paths to packaging tools
    FIND_PROGRAM(PKGBUILD_EXE pkgbuild)
    FIND_PROGRAM(PRODUCTBUILD_EXE productbuild)
    FIND_PROGRAM(PKGUTIL_EXE pkgutil)
    SET(PKG_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Darwin")
    SET(PKG_BUILD_DIR "${CMAKE_BINARY_DIR}/_CPack_Packages/productbuild/${PKG_FILE_NAME}")
    SET(PKG_ROOT_DIR "${PKG_BUILD_DIR}/Applications")

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/gui/CMake/Modules/CPack.Distribution.dist.in
                   ${CMAKE_BINARY_DIR}/CPack.Distribution.dist)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/gui/CMake/Scripts/repair_package.sh.in
                   ${CMAKE_BINARY_DIR}/repair_package.sh)

  INCLUDE(CPack)
ENDIF()
