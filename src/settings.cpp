/*
 * LXImage-Qt - a simple and fast image viewer
 * Copyright (C) 2013  PCMan <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "settings.h"
#include <QSettings>
#include <QIcon>
#include <QKeySequence>

using namespace LxImage;

inline static QString sortColumnToString(Fm::FolderModel::ColumnId value);
inline static Fm::FolderModel::ColumnId sortColumnFromString(const QString& str);

Settings::Settings():
  useFallbackIconTheme_(QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor")),
  bgColor_(255, 255, 255),
  fullScreenBgColor_(0, 0, 0),
  showExifData_(false),
  exifDatakWidth_(250),
  showThumbnails_(false),
  thumbnailSize_(64),
  thumbnailsPosition_(Qt::BottomDockWidgetArea),
  showSidePane_(false),
  slideShowInterval_(5),
  fallbackIconTheme_(QStringLiteral("oxygen")),
  maxRecentFiles_(5),
  fixedWindowWidth_(640),
  fixedWindowHeight_(480),
  lastWindowWidth_(640),
  lastWindowHeight_(480),
  lastWindowMaximized_(false),
  showOutline_(false),
  showMenubar_(true),
  showToolbar_(true),
  showAnnotationsToolbar_(false),
  forceZoomFit_(false),
  smoothOnZoom_(true),
  useTrash_(true),
  colorSpace_(1),
  sorting_(Fm::FolderModel::ColumnFileName) {
}

Settings::~Settings() {
}

bool Settings::load() {
  QSettings settings(QStringLiteral("lximage-qt"), QStringLiteral("settings"));
  fallbackIconTheme_ = settings.value(QStringLiteral("fallbackIconTheme"), fallbackIconTheme_).toString();
  bgColor_ = settings.value(QStringLiteral("bgColor"), bgColor_).value<QColor>();
  fullScreenBgColor_ = settings.value(QStringLiteral("fullScreenBgColor"), fullScreenBgColor_).value<QColor>();
  // showSidePane_;
  slideShowInterval_ = settings.value(QStringLiteral("slideShowInterval"), slideShowInterval_).toInt();
  maxRecentFiles_ = settings.value(QStringLiteral("maxRecentFiles"), maxRecentFiles_).toInt();
  recentlyOpenedFiles_ = settings.value(QStringLiteral("recentlyOpenedFiles")).toStringList();
  sorting_ = sortColumnFromString(settings.value(QStringLiteral("sorting")).toString());

  settings.beginGroup(QStringLiteral("Window"));
  fixedWindowWidth_ = settings.value(QStringLiteral("FixedWidth"), 640).toInt();
  fixedWindowHeight_ = settings.value(QStringLiteral("FixedHeight"), 480).toInt();
  lastWindowWidth_ = settings.value(QStringLiteral("LastWindowWidth"), 640).toInt();
  lastWindowHeight_ = settings.value(QStringLiteral("LastWindowHeight"), 480).toInt();
  lastWindowMaximized_ = settings.value(QStringLiteral("LastWindowMaximized"), false).toBool();
  rememberWindowSize_ = settings.value(QStringLiteral("RememberWindowSize"), true).toBool();
  showOutline_ = settings.value(QStringLiteral("ShowOutline"), false).toBool();
  showExifData_ = settings.value(QStringLiteral("ShowExifData"), false).toBool();
  exifDatakWidth_ = settings.value(QStringLiteral("ExifDatakWidth"), 250).toInt();
  showMenubar_ = settings.value(QStringLiteral("ShowMenubar"), true).toBool();
  showToolbar_ = settings.value(QStringLiteral("ShowToolbar"), true).toBool();
  showAnnotationsToolbar_ = settings.value(QStringLiteral("ShowAnnotationsToolbar"), false).toBool();
  forceZoomFit_ = settings.value(QStringLiteral("ForceZoomFit"), false).toBool();
  smoothOnZoom_ = settings.value(QStringLiteral("SmoothOnZoom"), true).toBool();
  useTrash_ = settings.value(QStringLiteral("UseTrash"), true).toBool();
  colorSpace_ = settings.value(QStringLiteral("ColorSpace"), 1).toInt();
  prefSize_ = settings.value(QStringLiteral("PrefSize"), QSize(400, 400)).toSize();
  settings.endGroup();

  // shortcuts
  settings.beginGroup(QStringLiteral("Shortcuts"));
  const QStringList actions = settings.childKeys();
  for(const auto& action : actions) {
    QString str = settings.value(action).toString();
    addShortcut(action, str);
  }
  settings.endGroup();

  settings.beginGroup(QStringLiteral("Thumbnail"));
  showThumbnails_ = settings.value(QStringLiteral("ShowThumbnails"), false).toBool();
  thumbnailsPositionFromString(settings.value(QStringLiteral("ThumbnailsPosition")).toString());
  setMaxThumbnailFileSize(qMax(settings.value(QStringLiteral("MaxThumbnailFileSize"), 4096).toInt(), 1024));
  setThumbnailSize(settings.value(QStringLiteral("ThumbnailSize"), 64).toInt());
  settings.endGroup();

  return true;
}

bool Settings::save() {
  QSettings settings(QStringLiteral("lximage-qt"), QStringLiteral("settings"));

  settings.setValue(QStringLiteral("fallbackIconTheme"), fallbackIconTheme_);
  settings.setValue(QStringLiteral("bgColor"), bgColor_);
  settings.setValue(QStringLiteral("fullScreenBgColor"), fullScreenBgColor_);
  settings.setValue(QStringLiteral("slideShowInterval"), slideShowInterval_);
  settings.setValue(QStringLiteral("maxRecentFiles"), maxRecentFiles_);
  settings.setValue(QStringLiteral("recentlyOpenedFiles"), recentlyOpenedFiles_);
  settings.setValue(QStringLiteral("sorting"), sortColumnToString(sorting_));

  settings.beginGroup(QStringLiteral("Window"));
  settings.setValue(QStringLiteral("FixedWidth"), fixedWindowWidth_);
  settings.setValue(QStringLiteral("FixedHeight"), fixedWindowHeight_);
  settings.setValue(QStringLiteral("LastWindowWidth"), lastWindowWidth_);
  settings.setValue(QStringLiteral("LastWindowHeight"), lastWindowHeight_);
  settings.setValue(QStringLiteral("LastWindowMaximized"), lastWindowMaximized_);
  settings.setValue(QStringLiteral("RememberWindowSize"), rememberWindowSize_);
  settings.setValue(QStringLiteral("ShowOutline"), showOutline_);
  settings.setValue(QStringLiteral("ShowMenubar"), showMenubar_);
  settings.setValue(QStringLiteral("ShowToolbar"), showToolbar_);
  settings.setValue(QStringLiteral("ShowAnnotationsToolbar"), showAnnotationsToolbar_);
  settings.setValue(QStringLiteral("ShowExifData"), showExifData_);
  settings.setValue(QStringLiteral("ExifDatakWidth"), exifDatakWidth_);
  settings.setValue(QStringLiteral("ForceZoomFit"), forceZoomFit_);
  settings.setValue(QStringLiteral("SmoothOnZoom"), smoothOnZoom_);
  settings.setValue(QStringLiteral("UseTrash"), useTrash_);
  settings.setValue(QStringLiteral("ColorSpace"), colorSpace_);
  settings.setValue(QStringLiteral("PrefSize"), prefSize_);
  settings.endGroup();

  // shortcuts
  settings.beginGroup(QStringLiteral("Shortcuts"));
  for(int i = 0; i < removedActions_.size(); ++i) {
    settings.remove(removedActions_.at(i));
  }
  QHash<QString, QString>::const_iterator it = actions_.constBegin();
  while(it != actions_.constEnd()) {
    settings.setValue(it.key(), it.value());
    ++it;
  }
  settings.endGroup();

  settings.beginGroup(QStringLiteral("Thumbnail"));
  settings.setValue(QStringLiteral("ShowThumbnails"), showThumbnails_);
  settings.setValue(QStringLiteral("ThumbnailsPosition"), thumbnailsPositionToString());
  settings.setValue(QStringLiteral("MaxThumbnailFileSize"), maxThumbnailFileSize());
  settings.setValue(QStringLiteral("ThumbnailSize"), thumbnailSize_);
  settings.endGroup();

  return true;
}

const QList<int>& Settings::thumbnailSizes() const {
  static const QList<int> _thumbnailSizes = {256, 224, 192, 160, 128, 96, 64};
  return _thumbnailSizes;
}

void Settings::setThumbnailsPosition(int pos) {
  switch(pos) {
    case Qt::TopDockWidgetArea:
      thumbnailsPosition_ = Qt::TopDockWidgetArea;
      break;
    case Qt::LeftDockWidgetArea:
      thumbnailsPosition_ = Qt::LeftDockWidgetArea;
      break;
    default:
      thumbnailsPosition_ = Qt::BottomDockWidgetArea;
      break;
  }
}

QString Settings::thumbnailsPositionToString() const {
  switch(thumbnailsPosition_) {
    case Qt::TopDockWidgetArea:
      return QStringLiteral("top");
    case Qt::LeftDockWidgetArea:
      return QStringLiteral("left");
    default:
      return QStringLiteral("bottom");
  }
}

void Settings::thumbnailsPositionFromString(const QString& str) {
  if(str == QStringLiteral("top")) {
    thumbnailsPosition_ = Qt::TopDockWidgetArea;
    return;
  }
  if(str == QStringLiteral("left")) {
    thumbnailsPosition_ = Qt::LeftDockWidgetArea;
    return;
  }
  thumbnailsPosition_ = Qt::BottomDockWidgetArea;
}

static QString sortColumnToString(Fm::FolderModel::ColumnId value) {
  QString ret;
  switch(value) {
  case Fm::FolderModel::ColumnFileName:
  default:
    ret = QLatin1String("name");
    break;
  case Fm::FolderModel::ColumnFileType:
    ret = QLatin1String("type");
    break;
  case Fm::FolderModel::ColumnFileSize:
    ret = QLatin1String("size");
    break;
  case Fm::FolderModel::ColumnFileMTime:
    ret = QLatin1String("mtime");
    break;
  case Fm::FolderModel::ColumnFileCrTime:
    ret = QLatin1String("crtime");
    break;
  }
  return ret;
}

static Fm::FolderModel::ColumnId sortColumnFromString(const QString& str) {
  Fm::FolderModel::ColumnId ret;
  if(str == QLatin1String("name")) {
    ret = Fm::FolderModel::ColumnFileName;
  }
  else if(str == QLatin1String("type")) {
    ret = Fm::FolderModel::ColumnFileType;
  }
  else if(str == QLatin1String("size")) {
    ret = Fm::FolderModel::ColumnFileSize;
  }
  else if(str == QLatin1String("mtime")) {
    ret = Fm::FolderModel::ColumnFileMTime;
  }
  else if(str == QLatin1String("crtime")) {
    ret = Fm::FolderModel::ColumnFileCrTime;
  }
  else {
    ret = Fm::FolderModel::ColumnFileName;
  }
  return ret;
}

