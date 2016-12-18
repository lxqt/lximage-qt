cmake_minimum_required(VERSION 3.0.2)
project(lximage-qt)

include(GNUInstallDirs)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 5)
set(PATCH_VERSION 1)
set(LXIMAGE_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

set(LXQTBT_MINIMUM_VERSION "0.3.0")

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5PrintSupport REQUIRED QUIET)
find_package(Qt5X11Extras REQUIRED QUIET)
find_package(Qt5LinguistTools REQUIRED QUIET)
find_package(Qt5Svg REQUIRED QUIET)
find_package(fm-qt REQUIRED QUIET)
find_package(lxqt-build-tools ${LXQTBT_MINIMUM_VERSION} REQUIRED)
message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")

include(LXQtCompilerSettings NO_POLICY_SCOPE)

find_package(PkgConfig REQUIRED)

# FIXME: we'll need this to provide detail info for photos in the future
pkg_check_modules(EXIF REQUIRED libexif)

# TODO: make the X11 stuff optional.
# for screenshot support
find_package(X11 REQUIRED)

# Xfixes is needed to capture the mouse cursor image
pkg_check_modules(XFIXES REQUIRED xfixes)

# add src subdirectory
add_subdirectory(src)

# install an icon for the application
install(
    FILES data/lximage-qt.png
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/48x48/apps"
)

