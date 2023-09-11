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

#ifndef LXIMAGE_SETTINGS_H
#define LXIMAGE_SETTINGS_H

#include <QString>
#include <QStringList>
#include <qcache.h>
#include <QColor>
#include <QSize>
#include <libfm-qt/core/thumbnailjob.h>
#include <libfm-qt/foldermodel.h>

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

  QString fallbackIconTheme() const {
    return fallbackIconTheme_;
  }
  void setFallbackIconTheme(QString value) {
    fallbackIconTheme_ = value;
  }

  QColor bgColor() const {
    return bgColor_;
  }
  void setBgColor(QColor color) {
    bgColor_ = color;
  }

  QColor fullScreenBgColor() const {
    return fullScreenBgColor_;
  }
  void setFullScreenBgColor(QColor color) {
    fullScreenBgColor_ = color;
  }

  bool showExifData() const {
    return showExifData_;
  }
  void setShowExifData(bool show) {
    showExifData_ = show;
  }

  int exifDatakWidth() const {
    return exifDatakWidth_;
  }
  void setExifDatakWidth(int width) {
    exifDatakWidth_ = width;
  }

  bool showThumbnails() const {
    return showThumbnails_;
  }
  void setShowThumbnails(bool show) {
    showThumbnails_ = show;
  }

  Qt::DockWidgetArea thumbnailsPosition() const {
    return thumbnailsPosition_;
  }
  void setThumbnailsPosition(int pos);

  int maxThumbnailFileSize() const {
    return Fm::ThumbnailJob::maxThumbnailFileSize();
  }

  void setMaxThumbnailFileSize(int size) {
    Fm::ThumbnailJob::setMaxThumbnailFileSize(size);
  }

  const QList<int>& thumbnailSizes() const;
  int thumbnailSize() const {
    return thumbnailSize_;
  }
  void setThumbnailSize(int size) {
    if(!thumbnailSizes().contains(thumbnailSize_)) {
      thumbnailSize_ = 64;
    }
    else {
      thumbnailSize_ = size;
    }
  }

  bool showSidePane() const {
    return showSidePane_;
  }

  int slideShowInterval() const {
    return slideShowInterval_;
  }
  void setSlideShowInterval(int interval) {
    slideShowInterval_ = interval;
  }

  QStringList recentlyOpenedFiles() const {
    return recentlyOpenedFiles_;
  }

  void setRecentlyOpenedFiles(const QStringList &recentlyOpenedFiles) {
    recentlyOpenedFiles_ = recentlyOpenedFiles;
  }

  int maxRecentFiles() const {
    return maxRecentFiles_;
  }
  void setMaxRecentFiles(int m) {
    maxRecentFiles_ = m;
  }

  bool rememberWindowSize() const {
    return rememberWindowSize_;
  }

  void setRememberWindowSize(bool rememberWindowSize) {
    rememberWindowSize_ = rememberWindowSize;
  }

  int windowWidth() const {
    if(rememberWindowSize_)
      return lastWindowWidth_;
    else
      return fixedWindowWidth_;
  }

  int windowHeight() const {
    if(rememberWindowSize_)
      return lastWindowHeight_;
    else
      return fixedWindowHeight_;
  }

  bool windowMaximized() const {
    if(rememberWindowSize_)
      return lastWindowMaximized_;
    else
      return false;
  }

  int fixedWindowWidth() const {
    return fixedWindowWidth_;
  }

  void setFixedWindowWidth(int fixedWindowWidth) {
    fixedWindowWidth_ = fixedWindowWidth;
  }

  int fixedWindowHeight() const {
    return fixedWindowHeight_;
  }

  void setFixedWindowHeight(int fixedWindowHeight) {
    fixedWindowHeight_ = fixedWindowHeight;
  }

  void setLastWindowWidth(int lastWindowWidth) {
      lastWindowWidth_ = lastWindowWidth;
  }

  void setLastWindowHeight(int lastWindowHeight) {
      lastWindowHeight_ = lastWindowHeight;
  }

  void setLastWindowMaximized(bool lastWindowMaximized) {
      lastWindowMaximized_ = lastWindowMaximized;
  }

  bool isOutlineShown() const {
    return showOutline_;
  }

  void showOutline(bool show) {
    showOutline_ = show;
  }

  bool isToolbarShown() const {
    return showToolbar_;
  }

  void showToolbar(bool show) {
    showToolbar_ = show;
  }

  bool isMenubarShown() const {
    return showMenubar_;
  }
  void showMenubar(bool show) {
    showMenubar_ = show;
  }

  bool isAnnotationsToolbarShown() const {
    return showAnnotationsToolbar_;
  }

  void showAnnotationsToolbar(bool show) {
    showAnnotationsToolbar_ = show;
  }

  bool forceZoomFit() const {
    return forceZoomFit_;
  }

  void setForceZoomFit(bool on) {
    forceZoomFit_ = on;
  }

  bool smoothOnZoom() const {
    return smoothOnZoom_;
  }
  void setSmoothOnZoom(bool on) {
    smoothOnZoom_ = on;
  }

  bool useTrash() const {
    return useTrash_;
  }
  void setUseTrash(bool on) {
    useTrash_ = on;
  }

  int colorSpace() const {
    return colorSpace_;
  }
  void setColorSpace(int cs) {
    colorSpace_ = cs;
  }

  QSize getPrefSize() const {
    return prefSize_;
  }
  void setPrefSize(const QSize &s) {
    prefSize_ = s;
  }

  QHash<QString, QString> customShortcutActions() const {
    return actions_;
  }
  void addShortcut(const QString &action, const QString &shortcut) {
    actions_.insert(action, shortcut);
  }
  void removeShortcut(const QString &action) {
    actions_.remove(action);
    removedActions_ << action;
  }

  Fm::FolderModel::ColumnId sorting() const {
    return sorting_;
  }
  void setSorting(Fm::FolderModel::ColumnId sorting) {
    sorting_ = sorting;
  }

private:
  QString thumbnailsPositionToString() const;
  void thumbnailsPositionFromString(const QString& str);

private:
  bool useFallbackIconTheme_;
  QColor bgColor_;
  QColor fullScreenBgColor_;
  bool showExifData_;
  int exifDatakWidth_;
  bool showThumbnails_;
  int thumbnailSize_;
  Qt::DockWidgetArea thumbnailsPosition_;
  bool showSidePane_;
  int slideShowInterval_;
  QString fallbackIconTheme_;
  QStringList recentlyOpenedFiles_;
  int maxRecentFiles_;

  bool rememberWindowSize_;
  int fixedWindowWidth_;
  int fixedWindowHeight_;
  int lastWindowWidth_;
  int lastWindowHeight_;
  bool lastWindowMaximized_;
  bool showOutline_;
  bool showMenubar_;
  bool showToolbar_;
  bool showAnnotationsToolbar_;
  bool forceZoomFit_;
  bool smoothOnZoom_;
  bool useTrash_;
  int colorSpace_;

  QSize prefSize_;

  QHash<QString, QString> actions_;
  QStringList removedActions_;

  Fm::FolderModel::ColumnId sorting_;
};

}

#endif // LXIMAGE_SETTINGS_H
