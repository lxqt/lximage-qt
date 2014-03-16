/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  PCMan <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef LXIMAGE_SETTINGS_H
#define LXIMAGE_SETTINGS_H

#include <QString>
#include <qcache.h>
#include <QColor>

namespace LxImage {

class Settings {
public:

  Settings();
  ~Settings();

  bool load();
  bool save();

  bool useFallbackIconTheme() const {
    return useFallbackIconTheme_;
  };

  QString fallbackIconTheme() {
    return fallbackIconTheme_;
  }
  void setFallbackIconTheme(QString value) {
    fallbackIconTheme_ = value;
  }

  QColor bgColor() {
    return bgColor_;
  }
  void setBgColor(QColor color) {
    bgColor_ = color;
  }

  QColor fullScreenBgColor() {
    return fullScreenBgColor_;
  }
  void setFullScreenBgColor(QColor color) {
    fullScreenBgColor_ = color;
  }

  bool showThumbnails() {
    return showThumbnails_;
  }
  void setShowThumbnails(bool show) {
    showThumbnails_ = show;
  }

  bool showSidePane() {
    return showSidePane_;
  }

  int slideShowInterval() {
    return slideShowInterval_;
  }
  void setSlideShowInterval(int interval) {
    slideShowInterval_ = interval;
  }

private:
  bool useFallbackIconTheme_;
  QColor bgColor_;
  QColor fullScreenBgColor_;
  bool showThumbnails_;
  bool showSidePane_;
  int slideShowInterval_;
  QString fallbackIconTheme_;
};

}

#endif // LXIMAGE_SETTINGS_H
