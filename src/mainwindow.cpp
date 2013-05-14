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
#include <QBuffer>

using namespace LxImage;

MainWindow::MainWindow():
  QMainWindow(),
  currentFile_(NULL),
  // currentFileInfo_(NULL),
  isLoading_(false),
  cancellable_(NULL),
  folder_(NULL),
  folderPath_(NULL),
  folderModel_(new Fm::FolderModel()),
  proxyModel_(new Fm::ProxyFolderModel()),
  modelFilter_(new ModelFilter()),
  image_() {

  ui.setupUi(this);
  proxyModel_->addFilter(modelFilter_);
  proxyModel_->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
  proxyModel_->setSourceModel(folderModel_);
}

MainWindow::~MainWindow() {
  if(cancellable_) {
    g_cancellable_cancel(cancellable_);
    // the cancellable object is freed in loadImageDataFree().
  }
  if(currentFile_)
    fm_path_unref(currentFile_);
  //if(currentFileInfo_)
  //  fm_file_info_unref(currentFileInfo_);
  if(folder_) {
    g_signal_handlers_disconnect_by_func(folder_, gpointer(_onFolderLoaded), this);
    g_object_unref(folder_);
  }
  if(folderPath_)
    fm_path_unref(folderPath_);
  delete folderModel_;
  delete proxyModel_;
  delete modelFilter_;
}

void MainWindow::on_actionAbout_triggered() {
  QMessageBox::about(this, tr("About"),
                     tr("LXImage - a simple and fast image viewer\n\n"
                     "Copyright (C) 2013\n"
                     "LXDE Project: http://lxde.org/\n\n"
                     "Authors:\n"
                     "Hong Jen Yee (PCMan) <pcman.tw@gmail.com>"));
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

void MainWindow::onFolderLoaded(FmFolder* folder) {
  qDebug("Finish loading: %d files", proxyModel_->rowCount());

  // if currently we're showing a file, get its index in the folder now
  // since the folder is fully loaded.
  if(currentFile_ && !currentIndex_.isValid())
    currentIndex_ = indexFromPath(currentFile_);
}

void MainWindow::_onFolderLoaded(FmFolder* folder, MainWindow* pThis) {
  pThis->onFolderLoaded(folder);
}

void MainWindow::on_actionOpen_triggered() {
  QString fileName = QFileDialog::getOpenFileName(
                      this, tr("Open File"), QString(),
                       tr("Images (*.png *.xpm *.jpg *.jpeg *.bmp)"));
  if(!fileName.isEmpty()) {
    FmPath* path = fm_path_new_for_str(qPrintable(fileName));
    if(currentFile_ && fm_path_equal(currentFile_, path)) {
      // the same file! do not load it again
      fm_path_unref(path);
      return;
    }
    // load the image file asynchronously
    loadImage(path);
    loadFolder(fm_path_get_parent(path));
    fm_path_unref(path);
  }
}

void MainWindow::on_actionSave_triggered() {

}

void MainWindow::on_actionSaveAs_triggered() {

}

void MainWindow::on_actionClose_triggered() {
  deleteLater();
}

void MainWindow::on_actionNext_triggered() {
  if(currentIndex_.isValid()) {
    QModelIndex index;
    if(currentIndex_.row() < proxyModel_->rowCount() - 1)
      index = proxyModel_->index(currentIndex_.row() + 1, 0);
    else
      index = proxyModel_->index(0, 0);
    FmFileInfo* info = proxyModel_->fileInfoFromIndex(index);
    if(info) {
      FmPath* path = fm_file_info_get_path(info);
      qDebug("try load: %s", fm_path_get_basename(path));
      loadImage(path, index);
    }
  }
}

void MainWindow::on_actionPrevious_triggered() {
  if(currentIndex_.isValid()) {
    QModelIndex index;
    qDebug("current row: %d", currentIndex_.row());
    if(currentIndex_.row() > 0)
      index = proxyModel_->index(currentIndex_.row() - 1, 0);
    else
      index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
    FmFileInfo* info = proxyModel_->fileInfoFromIndex(index);
    if(info) {
      FmPath* path = fm_file_info_get_path(info);
      qDebug("try load: %s", fm_path_get_basename(path));
      loadImage(path, index);
    }
  }
}

void MainWindow::on_actionFirst_triggered() {
  QModelIndex index = proxyModel_->index(0, 0);
  if(index.isValid()) {
    FmFileInfo* info = proxyModel_->fileInfoFromIndex(index);
    if(info) {
      FmPath* path = fm_file_info_get_path(info);
      qDebug("try load: %s", fm_path_get_basename(path));
      loadImage(path, index);
    }
  }
}

void MainWindow::on_actionLast_triggered() {
  QModelIndex index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
  if(index.isValid()) {
    FmFileInfo* info = proxyModel_->fileInfoFromIndex(index);
    if(info) {
      FmPath* path = fm_file_info_get_path(info);
      qDebug("try load: %s", fm_path_get_basename(path));
      loadImage(path, index);
    }
  }
}

// This is called from the worker thread, not main thread
gboolean MainWindow::loadImageThread(GIOSchedulerJob* job, GCancellable* cancellable, LoadImageData* data) {
  GFile* gfile = fm_path_to_gfile(data->path);
  GFileInputStream* fileStream = g_file_read(gfile, data->cancellable, &data->error);
  g_object_unref(gfile);

  if(fileStream) { // if the file stream is successfually opened
    QBuffer imageBuffer;
    GInputStream* inputStream = G_INPUT_STREAM(fileStream);
    while(!g_cancellable_is_cancelled(data->cancellable)) {
      char buffer[4096];
      gssize readSize = g_input_stream_read(inputStream, 
                                            buffer, 4096,
                                            data->cancellable, &data->error);
      if(readSize == -1 || readSize == 0) // error or EOF
        break;
      // append the bytes read to the image buffer
      imageBuffer.buffer().append(buffer, readSize);
    }
    g_input_stream_close(inputStream, NULL, NULL);

    // FIXME: maybe it's a better idea to implement a GInputStream based QIODevice.
    if(!data->error) // load the image from buffer if there are no errors
      data->image = QImage::fromData(imageBuffer.buffer());
  }
  // do final step in the main thread
  if(!g_cancellable_is_cancelled(data->cancellable))
    g_io_scheduler_job_send_to_mainloop(job, GSourceFunc(_onImageLoaded), data, NULL);
  return FALSE;
}

void MainWindow::loadFolder(FmPath* newFolderPath) {
  if(folder_) { // an folder is already loaded
    if(fm_path_equal(newFolderPath, folderPath_)) // same folder, ignore
      return;
    // free current folder
    g_signal_handlers_disconnect_by_func(folder_, gpointer(_onFolderLoaded), this);
    g_object_unref(folder_);
    fm_path_unref(folderPath_);
  }

  folderPath_ = fm_path_ref(newFolderPath);
  folder_ = fm_folder_from_path(newFolderPath);
  g_signal_connect(folder_, "finish-loading", G_CALLBACK(_onFolderLoaded), this);

  folderModel_->setFolder(folder_);
  currentIndex_ = QModelIndex(); // set current index to invalid
}

// the image is loaded
void MainWindow::onImageLoaded(MainWindow::LoadImageData* data) {
  isLoading_ = false;
  cancellable_ = NULL; // cancellable will be freed later in loadImageDataFree().

  image_ = data->image;
  ui.view->setImage(data->image);
  ui.view->zoomFit();

  if(currentFile_)
    fm_path_unref(currentFile_);
  currentFile_ = fm_path_ref(data->path);
  currentIndex_ = data->index;
  if(!currentIndex_.isValid())
    currentIndex_ = indexFromPath(currentFile_);

  if(data->error) {
    // if there are errors
    // TODO: show a info bar?
  }

  updateUI();
}

QModelIndex MainWindow::indexFromPath(FmPath* filePath) {
  // if the folder is already loaded, figure out our index
  // otherwise, it will be done again in onFolderLoaded() when the folder is fully loaded.
  if(folder_ && fm_folder_is_loaded(folder_)) {
    QModelIndex index;
    int count = proxyModel_->rowCount();
    for(int row = 0; row < count; ++row) {
      index = proxyModel_->index(row, 0);
      FmFileInfo* info = proxyModel_->fileInfoFromIndex(index);
      if(info && fm_path_equal(filePath, fm_file_info_get_path(info))) {
        return index;
      }
    }
  }
  return QModelIndex();
}


void MainWindow::updateUI() {
  if(currentFile_) {
    char* dispName = fm_path_display_basename(currentFile_);
    QString title = tr("%1 (%2x%3) - Image Viewer")
                      .arg(QString::fromUtf8(dispName))
                      .arg(image_.width())
                      .arg(image_.height());
    g_free(dispName);
    setWindowTitle(title);

    // TODO: update status bar, show current index in the folder
  }
  else {
    setWindowTitle(tr("Image Viewer"));
  }
}

// this function is called from main thread only
gboolean MainWindow::_onImageLoaded(LoadImageData* data) {
  // only do processing if the job is not cancelled
  if(!g_cancellable_is_cancelled(data->cancellable)) {
    data->mainWindow->onImageLoaded(data);
  }
  return TRUE;
}

void MainWindow::loadImageDataFree(LoadImageData* data) {
  g_object_unref(data->cancellable);
  fm_path_unref(data->path);
  if(data->error)
    g_error_free(data->error);
  delete data;
}

// Load the specified image file asynchronously in a worker thread.
// When the loading is finished, onImageLoaded() will be called.
void MainWindow::loadImage(FmPath* filePath, QModelIndex index) {
  // cancel loading of current image
  if(isLoading_) {
    g_cancellable_cancel(cancellable_);
    // the cancellable object is freed in loadImageDataFree().
    // we do not own a ref and hence there is no need to unref it.
  }

  // start a new gio job to load the specified image
  LoadImageData* data = new LoadImageData();
  data->cancellable = g_cancellable_new();
  data->path = fm_path_ref(filePath);
  data->index = index;
  data->mainWindow = this;
  data->error = NULL;
  cancellable_ = data->cancellable;

  isLoading_ = true;
  g_io_scheduler_push_job(GIOSchedulerJobFunc(loadImageThread),
                          data, GDestroyNotify(loadImageDataFree),
                          G_PRIORITY_DEFAULT, cancellable_);
}
