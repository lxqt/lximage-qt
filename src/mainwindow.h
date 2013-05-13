/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

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


#ifndef LXIMAGE_MAINWINDOW_H
#define LXIMAGE_MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

#include "imageview.h"
#include <QImage>

#include <libfm/fm-folder.h>
#include <libfm-qt/foldermodel.h>
#include <libfm-qt/proxyfoldermodel.h>

namespace LxImage {

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow();
  virtual ~MainWindow();
  
  bool openImage(QString fileName);

private Q_SLOTS:
  void on_actionAbout_triggered();

  void on_actionOpen_triggered();
  void on_actionSave_triggered();
  void on_actionSaveAs_triggered();
  void on_actionQuit_triggered();

  void on_actionPrevious_triggered();
  void on_actionNext_triggered();

  void on_actionZoomIn_triggered();
  void on_actionZoomOut_triggered();
  void on_actionOriginalSize_triggered();
  void on_actionZoomFit_triggered();

private:
  static void onFolderLoaded(FmFolder* folder, MainWindow* pThis);
  
private:
  Ui::MainWindow ui;
  QImage image_;
  FmFolder* folder_;
  Fm::FolderModel* folderModel_;
  Fm::ProxyFolderModel* proxyModel_;
  QModelIndex currentIndex_;
};

};

#endif // LXIMAGE_MAINWINDOW_H
