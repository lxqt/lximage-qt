cmake_minimum_required(VERSION 3.0.2)
project(lximage-qt)

include(GNUInstallDirs)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 6)
set(PATCH_VERSION 0)
set(LXIMAGE_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

set(LXQTBT_MINIMUM_VERSION "0.4.0")

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
find_package(Qt5X11Extras REQUIRED)
find_package(Qt5LinguistTools REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(fm-qt REQUIRED)
find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)
find_package(Exif REQUIRED)
message(STATUS "Building with Qt ${Qt5Core_VERSION}")

include(LXQtCompilerSettings NO_POLICY_SCOPE)

# TODO: make the X11 stuff optional.
# for screenshot support
find_package(X11 REQUIRED)

find_package(PkgConfig REQUIRED)
# Xfixes is needed to capture the mouse cursor image
pkg_check_modules(XFIXES REQUIRED xfixes)

# add src subdirectory
add_subdirectory(src)

# install an icon for the application
install(
    FILES data/lximage-qt.png
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps"
)

