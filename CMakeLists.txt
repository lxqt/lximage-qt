cmake_minimum_required(VERSION 2.8.11)

project(lximage-qt)

include(GNUInstallDirs)

# additional cmake files
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(MAJOR_VERSION 0)
set(MINOR_VERSION 4)
set(PATCH_VERSION 0)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5PrintSupport REQUIRED QUIET)
find_package(Qt5X11Extras REQUIRED QUIET)
find_package(Qt5LinguistTools REQUIRED QUIET)
message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0)

# FIXME: we'll need this to provide detail info for photos in the future
pkg_check_modules(EXIF REQUIRED libexif)

pkg_check_modules(LIBFM_QT REQUIRED libfm-qt5 libfm>=1.2)

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

# building tarball with CPack -------------------------------------------------
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
set(CPACK_GENERATOR TBZ2)
set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include(CPack)
