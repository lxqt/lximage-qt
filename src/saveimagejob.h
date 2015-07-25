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


#ifndef LXIMAGE_SAVEIMAGEJOB_H
#define LXIMAGE_SAVEIMAGEJOB_H

#include <gio/gio.h>
#include <libfm/fm.h>
#include <QImage>
#include "job.h"

namespace LxImage {

class MainWindow;

class SaveImageJob : public Job {

public:
  SaveImageJob(MainWindow* window, FmPath* filePath);

  QImage image() const {
    return image_;
  }

  FmPath* filePath() const {
    return path_;
  }

protected:
  virtual bool run();
  virtual void finish();

private:
  ~SaveImageJob(); // prevent direct deletion

public:
  MainWindow* mainWindow_;
  FmPath* path_;
  QImage image_;
};

}

#endif // LXIMAGE_SAVEIMAGEJOB_H
