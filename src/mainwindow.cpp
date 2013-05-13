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
  folder_(NULL),
  folderModel_(new Fm::FolderModel()),
  proxyModel_(new Fm::ProxyFolderModel()),
  image_() {

  ui.setupUi(this);
  proxyModel_->setSourceModel(folderModel_);
}

MainWindow::~MainWindow() {
  if(folder_) {
    g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderLoaded), this);
    g_object_unref(folder_);
  }
  delete folderModel_;
  delete proxyModel_;
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

void MainWindow::onFolderLoaded(FmFolder* folder, MainWindow* pThis) {
  qDebug("Finish loading: %d files", pThis->proxyModel_->rowCount());
  FmFileInfoList* files = fm_folder_get_files(folder);
  for(GList* l = fm_file_info_list_peek_head_link(files); l; l = l->next) {
    FmFileInfo* fi = FM_FILE_INFO(l->data);
    qDebug("%s", fm_file_info_get_disp_name(fi));
  }
}

bool MainWindow::openImage(QString fileName) {
  if(folder_) {
    g_signal_handlers_disconnect_by_func(folder_, gpointer(onFolderLoaded), this);
    g_object_unref(folder_);
    folder_ = NULL;
  }

  if(image_.load(fileName)) {
    ui.view->setImage(image_);
    QByteArray uname = fileName.toUtf8();
    uname = uname.left(uname.lastIndexOf('/'));
    folder_ = fm_folder_from_path_name(uname.constData());
    folderModel_->setFolder(folder_);
    currentIndex_ = proxyModel_->index(0, 0);

    g_signal_connect(folder_, "finish-loading", G_CALLBACK(onFolderLoaded), this);
  }
  else {
    image_ = QImage();
    ui.view->setImage(image_);
  }
  ui.view->zoomFit();
  return !image_.isNull();
}

void MainWindow::on_actionOpen_triggered() {
  QString fileName = QFileDialog::getOpenFileName(
                      this, tr("Open File"), QString(),
                       tr("Images (*.png *.xpm *.jpg *.jpeg *.bmp)"));
  // FIXME: support gio/gvfs and load the image file asynchronously?
  if(!fileName.isEmpty()) {
    openImage(fileName);
  }
}

void MainWindow::on_actionSave_triggered() {

}

void MainWindow::on_actionSaveAs_triggered() {

}

void MainWindow::on_actionQuit_triggered() {
  qApp->quit();
}

void MainWindow::on_actionNext_triggered() {
  if(currentIndex_.row() < proxyModel_->rowCount())
    currentIndex_ = proxyModel_->index(currentIndex_.row() + 1, 0);
  else
    currentIndex_ = proxyModel_->index(0, 0);
  FmFileInfo* info = proxyModel_->fileInfoFromIndex(currentIndex_);
  if(info) {
    FmPath* path = fm_file_info_get_path(info);
    char* pathStr = fm_path_to_str(path);
    if(image_.load(pathStr)) {
      ui.view->setImage(image_);
    }
    g_free(pathStr);
  }
}

void MainWindow::on_actionPrevious_triggered() {
  if(currentIndex_.row() > 0)
    currentIndex_ = proxyModel_->index(currentIndex_.row() - 1, 0);
  else
    currentIndex_ = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
  FmFileInfo* info = proxyModel_->fileInfoFromIndex(currentIndex_);
  if(info) {
    FmPath* path = fm_file_info_get_path(info);
    char* pathStr = fm_path_to_str(path);
    if(image_.load(pathStr)) {
      ui.view->setImage(image_);
    }
    g_free(pathStr);
  }
}
