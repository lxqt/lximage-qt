# set visibility to hidden to hide symbols, unlesss they're exporeted manually in the code
set(CMAKE_CXX_FLAGS "-DQT_NO_KEYWORDS -fno-exceptions")

include_directories(
  ${QT_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GLIB_INCLUDE_DIRS}
  ${EXIF_INCLUDE_DIRS}
  ${LIBFM_QT_INCLUDE_DIRS}
  ${X11_INCLUDE_DIR}
  ${XFIXES_INCLUDE_DIRS}
)

set(CMAKE_AUTOMOC TRUE)

# For some unknown reasons these are required for build on FreeBSD
link_directories(
  ${QT_LIBRARY_DIR}
  ${GLIB_LIBRARY_DIRS}
)

set(lximage-qt_SRCS
  lximage-qt.cpp
  mainwindow.cpp
  preferencesdialog.cpp
  application.cpp
  imageview.cpp
  modelfilter.cpp
  job.cpp
  loadimagejob.cpp
  saveimagejob.cpp
  screenshotdialog.cpp
  settings.cpp
)

qt4_add_dbus_adaptor(lximage-qt_SRCS
  org.lxde.LxImage.Application.xml
  application.h
  LxImage::Application
  applicationadaptor
  ApplicationAdaptor
)

set(lximage-qt_UIS
  mainwindow.ui
  preferencedialog.ui
  screenshotdialog.ui
)

qt4_wrap_ui(lximage-qt_UI_H
  ${lximage-qt_UIS}
)

# add translation for lximage-qt
# See http://www.cmake.org/Wiki/CMake:How_To_Build_Qt4_Software
file(GLOB TS_FILES translations/*.ts)
option (UPDATE_TRANSLATIONS "Update source translation translations/*.ts files")
if (UPDATE_TRANSLATIONS)
  qt4_create_translation(QM_FILES
    ${lximage-qt_SRCS}
    ${lximage-qt_UI_H}
    ${TS_FILES})
else (UPDATE_TRANSLATIONS)
  qt4_add_translation(QM_FILES ${TS_FILES})
endif (UPDATE_TRANSLATIONS)
add_custom_target (lximage-qt_translations DEPENDS ${QM_FILES})
install(FILES ${QM_FILES} DESTINATION share/lximage-qt/translations)
# prevent the generated files from being deleted during make clean
set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM true)

add_executable(lximage-qt
  ${lximage-qt_SRCS}
  ${lximage-qt_UI_H}
  ${QM_FILES}
)

add_definitions(
-DLXIMAGE_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/lximage-qt"
-DLIBFM_QT_API=Q_DECL_IMPORT    # FIXME: This should be done in libfm-qt headers instead :-(
)

target_link_libraries(lximage-qt
  ${QT_QTCORE_LIBRARY}
  ${QT_QTGUI_LIBRARY}
  ${QT_DBUS_LIBRARY}
  ${GLIB_LIBRARIES}
  ${EXIF_LIBRARIES}
  ${LIBFM_QT_LIBRARIES}
  ${X11_LIBRARIES}
  ${XFIXES_LIBRARIES}
)

install(TARGETS lximage-qt RUNTIME DESTINATION bin)
