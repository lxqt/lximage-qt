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


#ifndef LXIMAGE_LOADIMAGEJOB_H
#define LXIMAGE_LOADIMAGEJOB_H

#include <gio/gio.h>
#include <libfm/fm.h>
#include <QImage>

namespace LxImage {

class MainWindow;
  
class LoadImageJob {

public:
  LoadImageJob(MainWindow* window, FmPath* filePath);

  void cancel() {
    g_cancellable_cancel(cancellable_);
  }
  void start();

  QImage image() const {
    return image_;
  }

  FmPath* filePath() const {
    return path_;
  }

  GError* error() const {
    return error_;
  }

  bool isCancelled() const {
    return bool(g_cancellable_is_cancelled(cancellable_));
  }

private:
  ~LoadImageJob(); // prevent direct deletion

  static gboolean loadImageThread(GIOSchedulerJob *job, GCancellable *cancellable, LoadImageJob* pThis);
  static gboolean onImageLoaded(LoadImageJob* pThis);
  static void freeMe(LoadImageJob* pThis);

public:
  MainWindow* mainWindow_;
  GCancellable* cancellable_;
  FmPath* path_;
  QImage image_;
  GError* error_;
};

}

#endif // LXIMAGE_LOADIMAGEJOB_H
