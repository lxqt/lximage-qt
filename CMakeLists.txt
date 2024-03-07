cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)
# CMP0000: Call the cmake_minimum_required() command at the beginning of the top-level
# CMakeLists.txt file even before calling the project() command.
# The cmake_minimum_required(VERSION) command implicitly invokes the cmake_policy(VERSION)
# command to specify that the current project code is written for the given range of CMake
# versions.
project(lximage-qt)

include(GNUInstallDirs)

set(MAJOR_VERSION 2)
set(MINOR_VERSION 0)
set(PATCH_VERSION 0)
set(LXIMAGE_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

set(LXQTBT_MINIMUM_VERSION "2.0.0")
set(LIBFMQT_MINIMUM_VERSION "2.0.0")
set(QT_MINIMUM_VERSION "6.6.0")


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6Widgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt6Network ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt6DBus ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt6PrintSupport ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt6LinguistTools ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt6SvgWidgets ${QT_MINIMUM_VERSION} REQUIRED)
find_package(Qt6Svg ${QT_MINIMUM_VERSION} REQUIRED)
find_package(fm-qt6 ${LIBFMQT_MINIMUM_VERSION} REQUIRED)
find_package(lxqt2-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)
find_package(Exif REQUIRED)
message(STATUS "Building with Qt ${Qt6Core_VERSION}")

include(LXQtPreventInSourceBuilds)
include(LXQtCompilerSettings NO_POLICY_SCOPE)

# TODO: make the X11 stuff optional.
# for screenshot support
find_package(X11 REQUIRED)

find_package(PkgConfig REQUIRED)
# Xfixes is needed to capture the mouse cursor image
pkg_check_modules(XFIXES REQUIRED xfixes)

# add src subdirectory
add_subdirectory(src)

# install icon and metainfo for the application
install(FILES data/lximage-qt.svg
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps")
install(FILES data/lximage-qt.metainfo.xml
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
