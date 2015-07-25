/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013 - 2014  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

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
#include <QImageReader>
#include <QImageWriter>
#include <QClipboard>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QDebug>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QShortcut>
#include <QDockWidget>
#include <QScrollBar>
#include "application.h"
#include <libfm-qt/path.h>
#include <libfm-qt/folderview.h>
#include <libfm-qt/filepropsdialog.h>
#include <libfm-qt/fileoperation.h>

using namespace LxImage;

MainWindow::MainWindow():
  QMainWindow(),
  currentFile_(NULL),
  slideShowTimer_(NULL),
  // currentFileInfo_(NULL),
  loadJob_(NULL),
  saveJob_(NULL),
  folder_(NULL),
  folderPath_(NULL),
  folderModel_(new Fm::FolderModel()),
  proxyModel_(new Fm::ProxyFolderModel()),
  modelFilter_(new ModelFilter()),
  imageModified_(false),
  contextMenu_(new QMenu(this)),
  thumbnailsDock_(NULL),
  thumbnailsView_(NULL),
  image_() {

  setAttribute(Qt::WA_DeleteOnClose); // FIXME: check if current image is saved before close

  Application* app = static_cast<Application*>(qApp);
  app->addWindow();

  Settings& settings = app->settings();

  ui.setupUi(this);
  connect(ui.actionScreenshot, SIGNAL(triggered(bool)), app, SLOT(screenshot()));
  connect(ui.actionPreferences, SIGNAL(triggered(bool)), app ,SLOT(editPreferences()));

  proxyModel_->addFilter(modelFilter_);
  proxyModel_->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
  proxyModel_->setSourceModel(folderModel_);

  // build context menu
  ui.view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.view, SIGNAL(customContextMenuRequested(QPoint)), SLOT(onContextMenu(QPoint)));

  // install an event filter on the image view
  ui.view->installEventFilter(this);
  ui.view->setBackgroundBrush(QBrush(settings.bgColor()));

  if(settings.showThumbnails())
    setShowThumbnails(true);

  contextMenu_->addAction(ui.actionPrevious);
  contextMenu_->addAction(ui.actionNext);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionZoomOut);
  contextMenu_->addAction(ui.actionZoomIn);
  contextMenu_->addAction(ui.actionOriginalSize);
  contextMenu_->addAction(ui.actionZoomFit);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionSlideShow);
  contextMenu_->addAction(ui.actionFullScreen);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionRotateClockwise);
  contextMenu_->addAction(ui.actionRotateCounterclockwise);
  contextMenu_->addAction(ui.actionFlipHorizontal);
  contextMenu_->addAction(ui.actionFlipVertical);
  contextMenu_->addAction(ui.actionFlipVertical);

  // create shortcuts
  QShortcut* shortcut = new QShortcut(Qt::Key_Left, this);
  connect(shortcut, SIGNAL(activated()), SLOT(on_actionPrevious_triggered()));
  shortcut = new QShortcut(Qt::Key_Right, this);
  connect(shortcut, SIGNAL(activated()), SLOT(on_actionNext_triggered()));
  shortcut = new QShortcut(Qt::Key_Escape, this);
  connect(shortcut, SIGNAL(activated()), SLOT(onExitFullscreen()));
}

MainWindow::~MainWindow() {
  if(slideShowTimer_)
    delete slideShowTimer_;
  if(thumbnailsView_)
    delete thumbnailsView_;
  if(thumbnailsDock_)
    delete thumbnailsDock_;

  if(loadJob_) {
    loadJob_->cancel();
    // we don't need to do delete here. It will be done automatically
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

  Application* app = static_cast<Application*>(qApp);
  app->removeWindow();
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
  ui.view->setAutoZoomFit(false);
  ui.view->zoomOriginal();
}

void MainWindow::on_actionZoomFit_triggered() {
  ui.view->setAutoZoomFit(true);
  ui.view->zoomFit();
}

void MainWindow::on_actionZoomIn_triggered() {
  ui.view->setAutoZoomFit(false);
  ui.view->zoomIn();
}

void MainWindow::on_actionZoomOut_triggered() {
  ui.view->setAutoZoomFit(false);
  ui.view->zoomOut();
}

void MainWindow::onFolderLoaded(FmFolder* folder) {
  qDebug("Finish loading: %d files", proxyModel_->rowCount());
  // if currently we're showing a file, get its index in the folder now
  // since the folder is fully loaded.
  if(currentFile_ && !currentIndex_.isValid()) {
    currentIndex_ = indexFromPath(currentFile_);
    if(thumbnailsView_) { // showing thumbnails
      // select current file in the thumbnails view
      thumbnailsView_->childView()->setCurrentIndex(currentIndex_);
      thumbnailsView_->childView()->scrollTo(currentIndex_, QAbstractItemView::EnsureVisible);
    }
  }
}

void MainWindow::openImageFile(QString fileName) {
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

// paste the specified image into the current view,
// reset the window, remove loaded folders, and
// invalidate current file name.
void MainWindow::pasteImage(QImage newImage) {
  // cancel loading of current image
  if(loadJob_) {
    loadJob_->cancel(); // the job object will be freed automatically later
    loadJob_ = NULL;
  }
  setModified(true);

  currentIndex_ = QModelIndex(); // invaludate current index since we don't have a folder model now
  if(currentFile_)
    fm_path_unref(currentFile_);
  currentFile_ = NULL;

  image_ = newImage;
  ui.view->setImage(image_);
  ui.view->zoomOriginal();

  updateUI();
}

// popup a file dialog and retrieve the selected image file name
QString MainWindow::openFileName() {
  QString filterStr;
  QList<QByteArray> formats = QImageReader::supportedImageFormats();
  QList<QByteArray>::iterator it = formats.begin();
  for(;;) {
    filterStr += "*.";
    filterStr += (*it).toLower();
    ++it;
    if(it != formats.end())
      filterStr += ' ';
    else
      break;
  }

  QString fileName = QFileDialog::getOpenFileName(
    this, tr("Open File"), QString(),
          tr("Image files (%1)").arg(filterStr));
  return fileName;
}

// popup a file dialog and retrieve the selected image file name
QString MainWindow::saveFileName(QString defaultName) {
  QString filterStr;
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  QList<QByteArray>::iterator it = formats.begin();
  for(;;) {
    filterStr += "*.";
    filterStr += (*it).toLower();
    ++it;
    if(it != formats.end())
      filterStr += ' ';
    else
      break;
  }
  // FIXME: should we generate better filter strings? one format per item?

  QString fileName = QFileDialog::getSaveFileName(
    this, tr("Save File"), defaultName, tr("Image files (%1)").arg(filterStr));

  // use png format by default if the extension is not set
  if(!fileName.isEmpty() && fileName.indexOf('.') == -1)
    fileName += ".png";
  return fileName;
}

void MainWindow::on_actionOpenFile_triggered() {
  QString fileName = openFileName();
  if(!fileName.isEmpty()) {
    openImageFile(fileName);
  }
}

void MainWindow::on_actionNewWindow_triggered() {
  Application* app = static_cast<Application*>(qApp);
  MainWindow* window = new MainWindow();
  window->resize(app->settings().windowWidth(), app->settings().windowHeight());

  if(app->settings().windowMaximized())
        window->setWindowState(window->windowState() | Qt::WindowMaximized);

  window->show();
}

void MainWindow::on_actionSave_triggered() {
  if(saveJob_) // if we're currently saving another file
    return;

  if(!image_.isNull()) {
    if(currentFile_)
      saveImage(currentFile_);
    else
      on_actionSaveAs_triggered();
  }
}

void MainWindow::on_actionSaveAs_triggered() {
  if(saveJob_) // if we're currently saving another file
    return;
  QString fileName = saveFileName(currentFile_ ? Fm::Path(currentFile_).displayName() : QString());
  if(!fileName.isEmpty()) {
    FmPath* path = fm_path_new_for_str(qPrintable(fileName));
    // save the image file asynchronously
    saveImage(path);

    if(!currentFile_) { // if we haven't load any file yet
      currentFile_ = path;
      loadFolder(fm_path_get_parent(path));
    }
    else
      fm_path_unref(path);
  }
}

void MainWindow::on_actionDelete_triggered() {
  // delete the current file
  if(currentFile_) {
    FmPathList* paths = fm_path_list_new();
    fm_path_list_push_tail(paths, currentFile_);
    Fm::FileOperation::deleteFiles(paths);
    fm_path_list_unref(paths);
  }
}

void MainWindow::on_actionFileProperties_triggered() {
  if(currentIndex_.isValid()) {
    FmFileInfo* file = proxyModel_->fileInfoFromIndex(currentIndex_);
    // it's better to use an async job to query the file info since it's
    // possible that loading of the folder is not finished and the file info is
    // not available yet, but it's overkill for a rarely used function.
    if(file)
      Fm::FilePropsDialog::showForFile(file);
  }
}

void MainWindow::on_actionClose_triggered() {
  deleteLater();
}

void MainWindow::on_actionNext_triggered() {
  if(proxyModel_->rowCount() <= 1)
    return;
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
  if(proxyModel_->rowCount() <= 1)
    return;
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

// the image is loaded (the method is only called if the loading is not cancelled)
void MainWindow::onImageLoaded(LoadImageJob* job) {
  loadJob_ = NULL; // the job object will be freed later automatically

  image_ = job->image();
  ui.view->setAutoZoomFit(true);
  ui.view->setImage(image_);

  if(!currentIndex_.isValid())
    currentIndex_ = indexFromPath(currentFile_);

  updateUI();

  if(job->error()) {
    // if there are errors
    // TODO: show a info bar?
  }
}

void MainWindow::onImageSaved(SaveImageJob* job) {
  if(!job->error()) {
    setModified(false);
  }
  saveJob_ = NULL;
}

// filter events of other objects, mainly the image view.
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  // qDebug() << event;
  if(watched == ui.view) { // we got an event for the image view
    switch(event->type()) {
      case QEvent::Wheel: { // mouse wheel event
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if(wheelEvent->modifiers() == 0) {
          int delta = wheelEvent->delta();
          if(delta < 0)
            on_actionNext_triggered(); // next image
          else
            on_actionPrevious_triggered(); // previous image
        }
        break;
      }
      case QEvent::MouseButtonDblClick: {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if(mouseEvent->button() == Qt::LeftButton)
          ui.actionFullScreen->trigger();
        break;
      }
      default:;
    }
  }
  else if(thumbnailsView_ && watched == thumbnailsView_->childView()) {
    // scroll the thumbnail view with mouse wheel
    switch(event->type()) {
      case QEvent::Wheel: { // mouse wheel event
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if(wheelEvent->modifiers() == 0) {
          int delta = wheelEvent->delta();
          QScrollBar* hscroll = thumbnailsView_->childView()->horizontalScrollBar();
          if(hscroll)
            hscroll->setValue(hscroll->value() - delta);
          return true;
        }
        break;
      }
      default:;
    }
  }
  return QObject::eventFilter(watched, event);
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
  if(currentIndex_.isValid()) {
    if(thumbnailsView_) { // showing thumbnails
      // select current file in the thumbnails view
      thumbnailsView_->childView()->setCurrentIndex(currentIndex_);
      thumbnailsView_->childView()->scrollTo(currentIndex_, QAbstractItemView::EnsureVisible);
    }
  }

  QString title;
  if(currentFile_) {
    char* dispName = fm_path_display_basename(currentFile_);
    if(loadJob_) { // if loading is in progress
      title = tr("%1 (Loading...) - Image Viewer")
                .arg(QString::fromUtf8(dispName));
    }
    else {
      if(image_.isNull()) {
        title = tr("%1 (Failed to Load) - Image Viewer")
                  .arg(QString::fromUtf8(dispName));
      }
      else {
        title = tr("%1 (%2x%3) - Image Viewer")
                  .arg(QString::fromUtf8(dispName))
                  .arg(image_.width())
                  .arg(image_.height());
      }
    }
    g_free(dispName);
    // TODO: update status bar, show current index in the folder
  }
  else {
    title = tr("Image Viewer");
  }
  setWindowTitle(title);
}

// Load the specified image file asynchronously in a worker thread.
// When the loading is finished, onImageLoaded() will be called.
void MainWindow::loadImage(FmPath* filePath, QModelIndex index) {
  // cancel loading of current image
  if(loadJob_) {
    loadJob_->cancel(); // the job object will be freed automatically later
    loadJob_ = NULL;
  }
  if(imageModified_) {
    // TODO: ask the user to save the modified image?
    // this should be made optional
    setModified(false);
  }

  currentIndex_ = index;
  if(currentFile_)
    fm_path_unref(currentFile_);
  currentFile_ = fm_path_ref(filePath);
  // clear current image, but do not update the view now to prevent flickers
  image_ = QImage();

  // start a new gio job to load the specified image
  loadJob_ = new LoadImageJob(this, filePath);
  loadJob_->start();

  updateUI();
}

void MainWindow::saveImage(FmPath* filePath) {
  if(saveJob_) // do not launch a new job if the current one is still in progress
    return;
  // start a new gio job to save current image to the specified path
  saveJob_ = new SaveImageJob(this, filePath);
  saveJob_->start();
  // FIXME: add a cancel button to the UI? update status bar?
}

void MainWindow::on_actionRotateClockwise_triggered() {
  if(!image_.isNull()) {
    QTransform transform;
    transform.rotate(90.0);
    image_ = image_.transformed(transform, Qt::SmoothTransformation);
    ui.view->setImage(image_);
    setModified(true);
  }
}

void MainWindow::on_actionRotateCounterclockwise_triggered() {
  if(!image_.isNull()) {
    QTransform transform;
    transform.rotate(-90.0);
    image_ = image_.transformed(transform, Qt::SmoothTransformation);
    ui.view->setImage(image_);
    setModified(true);
  }
}

void MainWindow::on_actionCopy_triggered() {
  QClipboard *clipboard = QApplication::clipboard();
  QImage copiedImage = image_;
  // FIXME: should we copy the currently scaled result instead of the original image?
  /*
  double factor = ui.view->scaleFactor();
  if(factor == 1.0)
    copiedImage = image_;
  else
    copiedImage = image_.scaled();
  */
  clipboard->setImage(copiedImage);
}

void MainWindow::on_actionPaste_triggered() {
  QClipboard *clipboard = QApplication::clipboard();
  QImage image = clipboard->image();
  if(!image.isNull()) {
    pasteImage(image);
  }
}

void MainWindow::on_actionFlipVertical_triggered() {
  if(!image_.isNull()) {
    image_ = image_.mirrored(false, true);
    ui.view->setImage(image_);
    setModified(true);
  }
  setModified(true);
}

void MainWindow::on_actionFlipHorizontal_triggered() {
  if(!image_.isNull()) {
    image_ = image_.mirrored(true, false);
    ui.view->setImage(image_);
    setModified(true);
  }
  setModified(true);
}

void MainWindow::setModified(bool modified) {
  imageModified_ = modified;
  updateUI(); // TODO: update title bar to reflect the state change
}

void MainWindow::applySettings() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  if(isFullScreen())
    ui.view->setBackgroundBrush(QBrush(settings.fullScreenBgColor()));
  else
    ui.view->setBackgroundBrush(QBrush(settings.bgColor()));
}

void MainWindow::on_actionPrint_triggered() {
  // QPrinter printer(QPrinter::HighResolution);
  QPrinter printer;
  QPrintDialog dlg(&printer);
  if(dlg.exec() == QDialog::Accepted) {
    QPainter painter;
    painter.begin(&printer);
    QRect pageRect = printer.pageRect();
    int cols = (image_.width() / pageRect.width()) + (image_.width() % pageRect.width() ? 1 : 0);
    int rows = (image_.height() / pageRect.height()) + (image_.height() % pageRect.height() ? 1 : 0);
    // qDebug() << "page:" << printer.pageRect() << "image:" << image_.size() << "cols, rows:" << cols << rows;
    for(int row = 0; row < rows; ++row) {
      for(int col = 0; col < cols; ++col) {
        QRect srcRect(pageRect.width() * col, pageRect.height() * row, pageRect.width(), pageRect.height());
        // qDebug() << "row:" << row << "col:" << col << "src:" << srcRect << "page:" << printer.pageRect();
        painter.drawImage(QPoint(0, 0), image_, srcRect);
        if(col + 1 == cols && row + 1 == rows) // this is the last page
          break;
        printer.newPage();
      }
    }
    painter.end();
  }
}

// TODO: This can later be used for doing slide show
void MainWindow::on_actionFullScreen_triggered(bool checked) {
  if(checked)
    showFullScreen();
  else
    showNormal();
}

void MainWindow::on_actionSlideShow_triggered(bool checked) {
  if(checked) {
    if(!slideShowTimer_) {
      slideShowTimer_ = new QTimer();
      // switch to the next image when timeout
      connect(slideShowTimer_, SIGNAL(timeout()), SLOT(on_actionNext_triggered()));
    }
    Application* app = static_cast<Application*>(qApp);
    slideShowTimer_->start(app->settings().slideShowInterval() * 1000);
    // showFullScreen();
    ui.actionSlideShow->setIcon(QIcon::fromTheme("media-playback-stop"));
  }
  else {
    if(slideShowTimer_) {
      delete slideShowTimer_;
      slideShowTimer_ = NULL;
      ui.actionSlideShow->setIcon(QIcon::fromTheme("media-playback-start"));
    }
  }
}

void MainWindow::setShowThumbnails(bool show) {
  if(show) {
    if(!thumbnailsDock_) {
      thumbnailsDock_ = new QDockWidget(this);
      thumbnailsDock_->setFeatures(QDockWidget::NoDockWidgetFeatures); // FIXME: should use DockWidgetClosable
      thumbnailsDock_->setWindowTitle(tr("Thumbnails"));
      thumbnailsView_ = new Fm::FolderView(Fm::FolderView::IconMode);
      thumbnailsDock_->setWidget(thumbnailsView_);
      addDockWidget(Qt::BottomDockWidgetArea, thumbnailsDock_);
      QListView* listView = static_cast<QListView*>(thumbnailsView_->childView());
      listView->setFlow(QListView::TopToBottom);
      listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      listView->setSelectionMode(QAbstractItemView::SingleSelection);
      listView->installEventFilter(this);
      // FIXME: optimize the size of the thumbnail view
      // FIXME if the thumbnail view is docked elsewhere, update the settings.
      int scrollHeight = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
      thumbnailsView_->setFixedHeight(listView->gridSize().height() + scrollHeight);
      thumbnailsView_->setModel(proxyModel_);
      proxyModel_->setShowThumbnails(true);
      connect(thumbnailsView_->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(onThumbnailSelChanged(QItemSelection,QItemSelection)));
    }
  }
  else {
    if(thumbnailsDock_) {
      delete thumbnailsView_;
      thumbnailsView_ = NULL;
      delete thumbnailsDock_;
      thumbnailsDock_ = NULL;
    }
    proxyModel_->setShowThumbnails(false);
  }
}

void MainWindow::on_actionShowThumbnails_triggered(bool checked) {
  setShowThumbnails(checked);
}

void MainWindow::changeEvent(QEvent* event) {
  // TODO: hide menu/toolbars in full screen mode and make the background black.
  if(event->type() == QEvent::WindowStateChange) {
    Application* app = static_cast<Application*>(qApp);
    if(isFullScreen()) { // changed to fullscreen mode
      ui.view->setFrameStyle(QFrame::NoFrame);
      ui.view->setBackgroundBrush(QBrush(app->settings().fullScreenBgColor()));
      ui.toolBar->hide();
      ui.statusBar->hide();
      if(thumbnailsDock_)
        thumbnailsDock_->hide();
      // NOTE: in fullscreen mode, all shortcut keys in the menu are disabled since the menu
      // is disabled. We needs to add the actions to the main window manually to enable the
      // shortcuts again.
      ui.menubar->hide();
      Q_FOREACH(QAction* action, ui.menubar->actions()) {
        if(!action->shortcut().isEmpty())
          addAction(action);
      }
      addActions(ui.menubar->actions());
    }
    else { // restore to normal window mode
      ui.view->setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
      ui.view->setBackgroundBrush(QBrush(app->settings().bgColor()));
      // now we're going to re-enable the menu, so remove the actions previously added.
      Q_FOREACH(QAction* action, ui.menubar->actions()) {
        if(!action->shortcut().isEmpty())
          removeAction(action);
      }
      ui.menubar->show();
      ui.toolBar->show();
      ui.statusBar->show();
      if(thumbnailsDock_)
        thumbnailsDock_->show();
    }
  }
  QWidget::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  Settings& settings = static_cast<Application*>(qApp)->settings();
  if(settings.rememberWindowSize()) {
    settings.setLastWindowMaximized(isMaximized());

    if(!isMaximized()) {
        settings.setLastWindowWidth(width());
        settings.setLastWindowHeight(height());
    }
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  QWidget::closeEvent(event);
  Settings& settings = static_cast<Application*>(qApp)->settings();
  if(settings.rememberWindowSize()) {
    settings.setLastWindowMaximized(isMaximized());

    if(!isMaximized()) {
        settings.setLastWindowWidth(width());
        settings.setLastWindowHeight(height());
    }
  }
}

void MainWindow::onContextMenu(QPoint pos) {
  contextMenu_->exec(ui.view->mapToGlobal(pos));
}

void MainWindow::onExitFullscreen() {
  if(isFullScreen())
    showNormal();
}

void MainWindow::onThumbnailSelChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  // the selected item of thumbnail view is changed
  if(!selected.isEmpty()) {
    QModelIndex index = selected.first().topLeft();
    if(index.isValid() && index != currentIndex_) {
      FmFileInfo* file = proxyModel_->fileInfoFromIndex(index);
      if(file)
        loadImage(fm_file_info_get_path(file), index);
    }
  }
}
