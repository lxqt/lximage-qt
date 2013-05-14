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


#ifndef LXIMAGE_APPLICATION_H
#define LXIMAGE_APPLICATION_H

#include <QApplication>
#include <libfm-qt/libfmqt.h>
#include "mainwindow.h"

namespace LxImage {

class Application : public QApplication {
  Q_OBJECT

public:
  Application(int& argc, char** argv);
  bool init();
  bool parseCommandLineArgs(int argc, char** argv);

  void newWindow(QStringList files = QStringList());
  
private:
  Fm::LibFmQt libFm;
  bool isPrimaryInstance;
  QTranslator translator;
  QTranslator qtTranslator;
};

}

#endif // LXIMAGE_APPLICATION_H
