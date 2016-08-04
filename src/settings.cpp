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


#include "settings.h"
#include <QSettings>
#include <QIcon>

using namespace LxImage;

Settings::Settings():
  useFallbackIconTheme_(QIcon::themeName().isEmpty() || QIcon::themeName() == "hicolor"),
  bgColor_(255, 255, 255),
  fullScreenBgColor_(0, 0, 0),
  showThumbnails_(false),
  showSidePane_(false),
  slideShowInterval_(5),
  fallbackIconTheme_("oxygen"),
  fixedWindowWidth_(640),
  fixedWindowHeight_(480),
  lastWindowWidth_(640),
  lastWindowHeight_(480),
  lastWindowMaximized_(false) {
}

Settings::~Settings() {
}

bool Settings::load() {
  QSettings settings("lximage-qt", "settings");
  fallbackIconTheme_ = settings.value("fallbackIconTheme", fallbackIconTheme_).toString();
  bgColor_ = settings.value("bgColor", bgColor_).value<QColor>();
  fullScreenBgColor_ = settings.value("fullScreenBgColor", fullScreenBgColor_).value<QColor>();
  // showThumbnails_;
  // showSidePane_;
  slideShowInterval_ = settings.value("slideShowInterval", slideShowInterval_).toInt();

  settings.beginGroup("Window");
  fixedWindowWidth_ = settings.value("FixedWidth", 640).toInt();
  fixedWindowHeight_ = settings.value("FixedHeight", 480).toInt();
  lastWindowWidth_ = settings.value("LastWindowWidth", 640).toInt();
  lastWindowHeight_ = settings.value("LastWindowHeight", 480).toInt();
  lastWindowMaximized_ = settings.value("LastWindowMaximized", false).toBool();
  rememberWindowSize_ = settings.value("RememberWindowSize", true).toBool();
  settings.endGroup();

  return true;
}

bool Settings::save() {
  QSettings settings("lximage-qt", "settings");

  settings.setValue("fallbackIconTheme", fallbackIconTheme_);
  settings.setValue("bgColor", bgColor_);
  settings.setValue("fullScreenBgColor", fullScreenBgColor_);
  settings.setValue("slideShowInterval", slideShowInterval_);

  settings.beginGroup("Window");
  settings.setValue("FixedWidth", fixedWindowWidth_);
  settings.setValue("FixedHeight", fixedWindowHeight_);
  settings.setValue("LastWindowWidth", lastWindowWidth_);
  settings.setValue("LastWindowHeight", lastWindowHeight_);
  settings.setValue("LastWindowMaximized", lastWindowMaximized_);
  settings.setValue("RememberWindowSize", rememberWindowSize_);
  settings.endGroup();

  return true;
}


