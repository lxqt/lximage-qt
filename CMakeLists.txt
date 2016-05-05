cmake_minimum_required(VERSION 3.0.2)

project(lximage-qt)

include(GNUInstallDirs)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 4)
set(PATCH_VERSION 0)
set(LXIMAGE_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

# C++ 11 support
if (CMAKE_VERSION VERSION_LESS "3.1")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    else()
        CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
        # -std=c++0x is deprecated but some tools e.g. qmake or older gcc are still using it
        if(COMPILER_SUPPORTS_CXX0X)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
        else()
            message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} does not support c++11/c++0x")
        endif()
    endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()


find_package(Qt5Widgets REQUIRED)
find_package(Qt5DBus REQUIRED)
find_package(Qt5PrintSupport REQUIRED QUIET)
find_package(Qt5X11Extras REQUIRED QUIET)
find_package(Qt5LinguistTools REQUIRED QUIET)
find_package(fm-qt REQUIRED QUIET)
find_package(lxqt REQUIRED) #just a build dependency for .desktop files translation
message(STATUS "Building with Qt ${Qt5Core_VERSION_STRING}")

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

# building tarball with CPack -------------------------------------------------
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
set(CPACK_GENERATOR TBZ2)
set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
include(CPack)
