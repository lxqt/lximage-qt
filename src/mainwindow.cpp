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


#include "mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QImage>

using namespace LxImage;

MainWindow::MainWindow():
  QMainWindow(),
  image_() {

  ui.setupUi(this);
}

MainWindow::~MainWindow() {

}

void MainWindow::on_actionAbout_triggered() {
  QMessageBox::about(this, tr("About"), tr("LXImage - a simple and fast image viewer\n\nLXDE Project: http://lxde.org/"));
}

void MainWindow::on_actionOriginalSize_triggered() {
  ui.view->zoomOriginal();
}

void MainWindow::on_actionZoomFit_triggered() {
  ui.view->zoomFit();
}

void MainWindow::on_actionZoomIn_triggered() {
  ui.view->zoomIn();
}

void MainWindow::on_actionZoomOut_triggered() {
  ui.view->zoomOut();
}

void MainWindow::on_actionOpen_triggered() {
  QString fileName = QFileDialog::getOpenFileName(
                      this, tr("Open File"), QString(),
                       tr("Images (*.png *.xpm *.jpg *.jpeg *.bmp)"));
  // FIXME: support gio/gvfs and load the image file asynchronously?
  if(!fileName.isEmpty()) {
    if(image_.load(fileName)) {
      ui.view->setImage(image_);
    }
    else {
      image_ = QImage();
      ui.view->setImage(image_);
    }
    ui.view->zoomFit();
  }
}

void MainWindow::on_actionSave_triggered() {

}

void MainWindow::on_actionSaveAs_triggered() {

}

void MainWindow::on_actionQuit_triggered() {
  qApp->quit();
}
