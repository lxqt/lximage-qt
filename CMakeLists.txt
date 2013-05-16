project(lximage-qt)
# newer cmake is required for CMAKE_AUTOMOC Qt support
cmake_minimum_required(VERSION 2.8.6)

find_package(Qt4 REQUIRED
  QtCore
  QtGui
  QtDBus
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED
  glib-2.0
)

pkg_check_modules(EXIF REQUIRED
  libexif
)

pkg_check_modules(LIBFM_QT REQUIRED
  libfm-qt
  libfm>=1.2
)

# for xsettings support
find_package(X11 REQUIRED)

# add src subdirectory
add_subdirectory(src)

# install a desktop entry file
configure_file(data/lximage-qt.desktop.in lximage-qt.desktop @ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/lximage-qt.desktop
  DESTINATION share/applications
)

# install an icon for the application
install(
  FILES data/lximage-qt.png
  # FIXME: is this the proper place to install an icon?
  DESTINATION share/icons/hicolor/48x48/apps
)

# update translations
add_custom_target(update_translations ALL DEPENDS
  lximage-qt_translations
)

# building tarball with CPack -------------------------------------------------
# To create a source distribution, type:
# make package_source
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set (CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
set (CPACK_PACKAGE_VENDOR "")
set (CPACK_PACKAGE_VERSION_MAJOR "0")
set (CPACK_PACKAGE_VERSION_MINOR "1")
set (CPACK_PACKAGE_VERSION_PATCH "0")
set (CPACK_GENERATOR TBZ2)
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_IGNORE_FILES /build/;.gitignore;.*~;.git;.kdev4;temp)
# set (CPACK_SOURCE_PACKAGE_FILE_NAME lximage-qt.0.1.0.tar.gz)
include (CPack)
