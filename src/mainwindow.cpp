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

#include "mainwindow.h"
#include <QActionGroup>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QColorSpace>
#include <QClipboard>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QScreen>
#include <QWindow>
#include <QDockWidget>
#include <QScrollBar>
#include <QHeaderView>
#include <QStandardPaths>
#include <QDateTime>
#include <QX11Info>

#include "application.h"
#include <libfm-qt/folderview.h>
#include <libfm-qt/filepropsdialog.h>
#include <libfm-qt/fileoperation.h>
#include <libfm-qt/folderitemdelegate.h>
#include <libfm-qt/utilities.h>

#include "mrumenu.h"
#include "resizeimagedialog.h"
#include "upload/uploaddialog.h"
#include "ui_shortcuts.h"

using namespace LxImage;

MainWindow::MainWindow():
  QMainWindow(),
  contextMenu_(new QMenu(this)),
  slideShowTimer_(nullptr),
  image_(),
  // currentFileInfo_(nullptr),
  imageModified_(false),
  startMaximized_(false),
  folder_(nullptr),
  folderModel_(new Fm::FolderModel()),
  proxyModel_(new Fm::ProxyFolderModel()),
  modelFilter_(new ModelFilter()),
  thumbnailsDock_(nullptr),
  thumbnailsView_(nullptr),
  loadJob_(nullptr),
  saveJob_(nullptr),
  fileMenu_(nullptr),
  showFullScreen_(false) {

  setAttribute(Qt::WA_DeleteOnClose); // FIXME: check if current image is saved before close

  Application* app = static_cast<Application*>(qApp);
  app->addWindow();

  Settings& settings = app->settings();

  ui.setupUi(this);
  if(QX11Info::isPlatformX11()) {
    connect(ui.actionScreenshot, &QAction::triggered, app, &Application::screenshot);
  }
  else {
    ui.actionScreenshot->setVisible(false);
  }
  connect(ui.actionPreferences, &QAction::triggered, app , &Application::editPreferences);

  proxyModel_->addFilter(modelFilter_);
  proxyModel_->sort(settings.sorting(), Qt::AscendingOrder);
  proxyModel_->setSourceModel(folderModel_);
  connect(proxyModel_, &Fm::ProxyFolderModel::sortFilterChanged, this, &MainWindow::onSortFilterChanged);

  ui.view->setSmoothOnZoom(settings.smoothOnZoom());

  // build context menu
  ui.view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ui.view, &QWidget::customContextMenuRequested, this, &MainWindow::onContextMenu);

  connect(ui.view, &ImageView::fileDropped, this, &MainWindow::onFileDropped);

  connect(ui.view, &ImageView::zooming, this, &MainWindow::onZooming);

  // install an event filter on the image view
  ui.view->installEventFilter(this);

  ui.view->setBackgroundBrush(QBrush(settings.bgColor()));

  ui.view->updateOutline();

  ui.view->showOutline(settings.isOutlineShown());
  ui.actionShowOutline->setChecked(settings.isOutlineShown());

  setShowExifData(settings.showExifData());
  ui.actionShowExifData->setChecked(settings.showExifData());

  setShowThumbnails(settings.showThumbnails());
  ui.actionShowThumbnails->setChecked(settings.showThumbnails());

  addAction(ui.actionMenubar);
  on_actionMenubar_triggered(settings.isMenubarShown());
  ui.actionMenubar->setChecked(settings.isMenubarShown());

  ui.toolBar->setVisible(settings.isToolbarShown());
  ui.actionToolbar->setChecked(settings.isToolbarShown());
  connect(ui.actionToolbar, &QAction::triggered, this, [this](int checked) {
    if(!isFullScreen()) { // toolbar is hidden in fullscreen
      ui.toolBar->setVisible(checked);
    }
  });
  // toolbar visibility can change in its context menu
  connect(ui.toolBar, &QToolBar::visibilityChanged, this, [this](int visible) {
    if(!isFullScreen()) { // toolbar is hidden in fullscreen
      ui.actionToolbar->setChecked(visible);
    }
  });

  ui.annotationsToolBar->setVisible(settings.isAnnotationsToolbarShown());
  ui.actionAnnotations->setChecked(settings.isAnnotationsToolbarShown());
  connect(ui.actionAnnotations, &QAction::triggered, this, [this](int checked) {
    if(!isFullScreen()) { // annotations toolbar is hidden in fullscreen
      ui.annotationsToolBar->setVisible(checked);
    }
  });
  // annotations toolbar visibility can change in its context menu
  connect(ui.annotationsToolBar, &QToolBar::visibilityChanged, this, [this](int visible) {
    if(!isFullScreen()) { // annotations toolbar is hidden in fullscreen
      ui.actionAnnotations->setChecked(visible);
    }
  });

  auto aGroup = new QActionGroup(this);
  ui.actionZoomFit->setActionGroup(aGroup);
  ui.actionOriginalSize->setActionGroup(aGroup);
  ui.actionZoomFit->setChecked(true);

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
  contextMenu_->addAction(ui.actionShowOutline);
  contextMenu_->addAction(ui.actionShowExifData);
  contextMenu_->addAction(ui.actionShowThumbnails);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionMenubar);
  contextMenu_->addAction(ui.actionToolbar);
  contextMenu_->addAction(ui.actionAnnotations);
  contextMenu_->addSeparator();
  contextMenu_->addAction(ui.actionRotateClockwise);
  contextMenu_->addAction(ui.actionRotateCounterclockwise);
  contextMenu_->addAction(ui.actionFlipHorizontal);
  contextMenu_->addAction(ui.actionFlipVertical);

  auto sortGroup = new QActionGroup(ui.menu_View);
  sortGroup->setExclusive(true);
  sortGroup->addAction(ui.actionByFileName);
  sortGroup->addAction(ui.actionByMTime);
  sortGroup->addAction(ui.actionByCrTime);
  sortGroup->addAction(ui.actionByFileSize);
  sortGroup->addAction(ui.actionByFileType);
  connect(ui.menu_View, &QMenu::aboutToShow, this, &MainWindow::sortMenuAboutToShow);

  // Open images when MRU items are clicked
  ui.menuRecently_Opened_Files->setMaxItems(settings.maxRecentFiles());
  connect(ui.menuRecently_Opened_Files, &MruMenu::itemClicked, this, &MainWindow::onFileDropped);

  // Create an action group for the annotation tools
  QActionGroup *annotationGroup = new QActionGroup(this);
  annotationGroup->addAction(ui.actionDrawNone);
  annotationGroup->addAction(ui.actionDrawArrow);
  annotationGroup->addAction(ui.actionDrawRectangle);
  annotationGroup->addAction(ui.actionDrawCircle);
  annotationGroup->addAction(ui.actionDrawNumber);
  ui.actionDrawNone->setChecked(true);

  // the "Open With..." menu
  connect(ui.menu_File, &QMenu::aboutToShow, this, &MainWindow::fileMenuAboutToShow);
  connect(ui.openWithMenu, &QMenu::aboutToShow, this, &MainWindow::createOpenWithMenu);
  connect(ui.openWithMenu, &QMenu::aboutToHide, this, &MainWindow::deleteOpenWithMenu);

  // create hard-coded keyboard shortcuts; they are set in setShortcuts() if not ambiguous
  QShortcut* shortcut = new QShortcut(this);
  hardCodedShortcuts_[Qt::Key_Left] = shortcut;
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_triggered);
  shortcut = new QShortcut(this);
  hardCodedShortcuts_[Qt::Key_Backspace] = shortcut;
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_triggered);
  shortcut = new QShortcut(this);
  hardCodedShortcuts_[Qt::Key_Right] = shortcut;
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_triggered);
  shortcut = new QShortcut(this);
  hardCodedShortcuts_[Qt::Key_Space] = shortcut;
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_triggered);
  shortcut = new QShortcut(this);
  hardCodedShortcuts_[Qt::Key_Home] = shortcut; // already in GUI but will be forced if removed
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionFirst_triggered);
  shortcut = new QShortcut(this);
  hardCodedShortcuts_[Qt::Key_End] = shortcut; // already in GUI but will be forced if removed
  connect(shortcut, &QShortcut::activated, this, &MainWindow::on_actionLast_triggered);
  shortcut = new QShortcut(this);
  hardCodedShortcuts_[Qt::Key_Escape] = shortcut;
  connect(shortcut, &QShortcut::activated, this, &MainWindow::onKeyboardEscape);

  // set custom and hard-coded shortcuts
  setShortcuts();
}

MainWindow::~MainWindow() {
  delete slideShowTimer_;
  delete thumbnailsView_;
  delete thumbnailsDock_;

  if(loadJob_) {
    loadJob_->cancel();
    // we don't need to do delete here. It will be done automatically
  }
  //if(currentFileInfo_)
  //  fm_file_info_unref(currentFileInfo_);
  delete folderModel_;
  delete proxyModel_;
  delete modelFilter_;

  Application* app = static_cast<Application*>(qApp);
  app->removeWindow();
}

void MainWindow::on_actionMenubar_triggered(bool checked) {
  if(!checked) {
    // If menubar is hidden, shortcut keys inside menus will be disabled. Therefore,
    // we need to add menubar actions manually to enable shortcuts before hiding menubar.
    addActions(ui.menubar->actions());
    ui.menubar->setVisible(false);
  }
  else if(!isFullScreen()) { // menubar is hidden in fullscreen
    ui.menubar->setVisible(true);
    // when menubar is shown again, remove the previously added actions.
    const auto _actions = ui.menubar->actions();
    for(const auto& action : _actions) {
      removeAction(action);
    }
  }
}

void MainWindow::on_actionAbout_triggered() {
  QMessageBox::about(this, tr("About"),
                     QStringLiteral("<center><b><big>LXImage-Qt %1</big></b></center><br>").arg(qApp->applicationVersion())
                     + tr("A simple and fast image viewer")
                     + QStringLiteral("<br><br>")
                     + tr("Copyright (C) ") + tr("2013-2021")
                     + QStringLiteral("<br><a href='https://lxqt-project.org'>")
                     + tr("LXQt Project")
                     + QStringLiteral("</a><br><br>")
                     + tr("Development: ")
                     + QStringLiteral("<a href='https://github.com/lxqt/lximage-qt'>https://github.com/lxqt/lximage-qt</a><br><br>")
                     + tr("Author: ")
                     + QStringLiteral("<a href='mailto:pcman.tw@gmail.com?Subject=My%20Subject'>Hong Jen Yee (PCMan)</a>"));
}

void MainWindow::on_actionOriginalSize_triggered() {
  if(ui.actionOriginalSize->isChecked()) {
    ui.view->setAutoZoomFit(false);
    ui.view->zoomOriginal();
  }
}

void MainWindow::on_actionHiddenShortcuts_triggered() {
    class HiddenShortcutsDialog : public QDialog {
    public:
        explicit HiddenShortcutsDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags())
            : QDialog(parent, f) {
            ui.setupUi(this);
            ui.treeWidget->setRootIsDecorated(false);
            ui.treeWidget->header()->setSectionResizeMode(QHeaderView::Stretch);
            ui.treeWidget->header()->setSectionsClickable(true);
            ui.treeWidget->sortByColumn(0, Qt::AscendingOrder);
            ui.treeWidget->setSortingEnabled(true);
        }
    private:
        Ui::HiddenShortcutsDialog ui;
    };
    HiddenShortcutsDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionZoomFit_triggered() {
  if(ui.actionZoomFit->isChecked()) {
    ui.view->setAutoZoomFit(true);
    ui.view->zoomFit();
  }
}

void MainWindow::on_actionZoomIn_triggered() {
  if(!ui.view->image().isNull()) {
    ui.view->setAutoZoomFit(false);
    ui.view->zoomIn();
  }
}

void MainWindow::on_actionZoomOut_triggered() {
  if(!ui.view->image().isNull()) {
    ui.view->setAutoZoomFit(false);
    ui.view->zoomOut();
  }
}

void MainWindow::onZooming() {
  ui.actionZoomFit->setChecked(false);
  ui.actionOriginalSize->setChecked(false);
}

void MainWindow::on_actionDrawNone_triggered() {
  ui.view->activateTool(ImageView::ToolNone);
}

void MainWindow::on_actionDrawArrow_triggered() {
  ui.view->activateTool(ImageView::ToolArrow);
}

void MainWindow::on_actionDrawRectangle_triggered() {
  ui.view->activateTool(ImageView::ToolRectangle);
}

void MainWindow::on_actionDrawCircle_triggered() {
  ui.view->activateTool(ImageView::ToolCircle);
}

void MainWindow::on_actionDrawNumber_triggered() {
  ui.view->activateTool(ImageView::ToolNumber);
}

void MainWindow::on_actionByFileName_triggered(bool /*checked*/) {
  proxyModel_->sort(Fm::FolderModel::ColumnFileName, Qt::AscendingOrder);
}

void MainWindow::on_actionByMTime_triggered(bool /*checked*/) {
  proxyModel_->sort(Fm::FolderModel::ColumnFileMTime, Qt::AscendingOrder);
}

void MainWindow::on_actionByCrTime_triggered(bool /*checked*/) {
  proxyModel_->sort(Fm::FolderModel::ColumnFileCrTime, Qt::AscendingOrder);
}

void MainWindow::on_actionByFileSize_triggered(bool /*checked*/) {
  proxyModel_->sort(Fm::FolderModel::ColumnFileSize, Qt::AscendingOrder);
}

void MainWindow::on_actionByFileType_triggered(bool /*checked*/) {
  proxyModel_->sort(Fm::FolderModel::ColumnFileType, Qt::AscendingOrder);
}

void MainWindow::onSortFilterChanged() {
  // update currentIndex_ and scroll to it in thumbnails view
  if(currentFile_)
    currentIndex_ = indexFromPath(currentFile_);
  if(thumbnailsView_ && currentIndex_.isValid()) {
    thumbnailsView_->childView()->scrollTo(currentIndex_, QAbstractItemView::EnsureVisible);
  }
  // remember the sorting if possible
  static_cast<Application*>(qApp)->settings().setSorting(static_cast<Fm::FolderModel::ColumnId>(proxyModel_->sortColumn()));
}

// This is needed only because sorting may be changed from inside the thumbnails view.
void MainWindow::sortMenuAboutToShow() {
  auto sortColumn = proxyModel_->sortColumn();
  if(sortColumn == Fm::FolderModel::ColumnFileName) {
    ui.actionByFileName->setChecked(true);
  }
  else if(sortColumn == Fm::FolderModel::ColumnFileMTime) {
    ui.actionByMTime->setChecked(true);
  }
  else if(sortColumn == Fm::FolderModel::ColumnFileCrTime) {
    ui.actionByCrTime->setChecked(true);
  }
  else if(sortColumn == Fm::FolderModel::ColumnFileSize) {
    ui.actionByFileSize->setChecked(true);
  }
  else if(sortColumn == Fm::FolderModel::ColumnFileType) {
    ui.actionByFileType->setChecked(true);
  }
  else { // sorting is not supported
    ui.actionByFileName->setChecked(false);
    ui.actionByMTime->setChecked(false);
    ui.actionByCrTime->setChecked(false);
    ui.actionByFileSize->setChecked(false);
    ui.actionByFileType->setChecked(false);
  }
}

void MainWindow::onFolderLoaded() {
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
  // this is used to open the first image of a folder
  else if (!currentFile_) {
    on_actionFirst_triggered();
  }
}

void MainWindow::openImageFile(const QString& fileName) {
  const Fm::FilePath path = Fm::FilePath::fromPathStr(qPrintable(fileName));
    // the same file! do not load it again
  if(currentFile_ && currentFile_ == path)
    return;

  if (QFileInfo(fileName).isDir()) {
    if(path == folderPath_)
      return;

    const QList<QByteArray> formats = QImageReader::supportedImageFormats();
    QStringList formatsFilters;
    for (const QByteArray& format: formats)
      formatsFilters << QStringLiteral("*.") + QString::fromUtf8(format);
    QDir dir(fileName);
    dir.setNameFilters(formatsFilters);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    if(dir.entryList().isEmpty())
      return;

    currentFile_ = Fm::FilePath{};
    loadFolder(path);
  }
  else {
    // load the image file asynchronously
    loadImage(path);
    loadFolder(path.parent());
  }
}

// paste the specified image into the current view,
// reset the window, remove loaded folders, and
// invalidate current file name.
void MainWindow::pasteImage(QImage newImage) {
  // cancel loading of current image
  if(loadJob_) {
    loadJob_->cancel(); // the job object will be freed automatically later
    loadJob_ = nullptr;
  }
  setModified(true);

  currentIndex_ = QModelIndex(); // invaludate current index since we don't have a folder model now
  currentFile_ = Fm::FilePath{};

  image_ = newImage;
  ui.view->setImage(image_);
  // always fit the image on pasting
  ui.actionZoomFit->setChecked(true);
  ui.view->setAutoZoomFit(true);
  ui.view->zoomFit();

  updateUI();
}

// popup a file dialog and retrieve the selected image file name
QString MainWindow::openFileName() {
  QString filterStr;
  QList<QByteArray> formats = QImageReader::supportedImageFormats();
  QList<QByteArray>::iterator it = formats.begin();
  for(;;) {
    filterStr += QLatin1String("*.");
    filterStr += QString::fromUtf8((*it).toLower());
    ++it;
    if(it != formats.end())
      filterStr += QLatin1Char(' ');
    else
      break;
  }

  QString curFileName;
  if(currentFile_) {
    curFileName = QString::fromUtf8(currentFile_.displayName().get());
  }
  else {
    curFileName = QString::fromUtf8(Fm::FilePath::homeDir().toString().get());
  }
  QString fileName = QFileDialog::getOpenFileName(
    this, tr("Open File"), curFileName,
          tr("Image files (%1)").arg(filterStr));
  return fileName;
}

QString MainWindow::openDirectory() {
  QString curDirName;
  if(currentFile_ && currentFile_.parent()) {
    curDirName = QString::fromUtf8(currentFile_.parent().displayName().get());
  }
  else {
    curDirName = QString::fromUtf8(Fm::FilePath::homeDir().toString().get());
  }
  QString directory = QFileDialog::getExistingDirectory(this,
          tr("Open directory"), curDirName);
  return directory;
}

// popup a file dialog and retrieve the selected image file name
QString MainWindow::saveFileName(const QString& defaultName) {
  QString filterStr;
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  QList<QByteArray>::iterator it = formats.begin();
  for(;;) {
    filterStr += QLatin1String("*.");
    filterStr += QString::fromUtf8((*it).toLower());
    ++it;
    if(it != formats.end())
      filterStr += QLatin1Char(' ');
    else
      break;
  }
  // FIXME: should we generate better filter strings? one format per item?

  QString fileName;
  QFileDialog diag(this, tr("Save File"), defaultName,
               tr("Image files (%1)").arg(filterStr));
  diag.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
  diag.setFileMode(QFileDialog::FileMode::AnyFile);
  diag.setDefaultSuffix(QStringLiteral("png"));

  if (diag.exec()) {
    fileName = diag.selectedFiles().at(0);
  }

  return fileName;
}

void MainWindow::on_actionOpenFile_triggered() {
  QString fileName = openFileName();
  if(!fileName.isEmpty()) {
    openImageFile(fileName);
  }
}

void MainWindow::on_actionOpenDirectory_triggered() {
  QString directory = openDirectory();
  if(!directory.isEmpty()) {
    openImageFile(directory);
  }
}

void MainWindow::on_actionReload_triggered() {
  if (currentFile_) {
    loadImage(currentFile_);
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
  QString curFileName;
  if(currentFile_) {
    curFileName = QString::fromUtf8(currentFile_.displayName().get());
  }

  QString fileName = saveFileName(curFileName);
  if(!fileName.isEmpty()) {
    const Fm::FilePath path = Fm::FilePath::fromPathStr(qPrintable(fileName));
    // save the image file asynchronously
    saveImage(path);
  }
}

void MainWindow::on_actionDelete_triggered() {
  // delete or trash the current file
  if(currentFile_) {
    if(static_cast<Application*>(qApp)->settings().useTrash()) {
      Fm::FileOperation::trashFiles({currentFile_}, false);
    }
    else {
      Fm::FileOperation::deleteFiles({currentFile_}, true);
    }
  }
}

void MainWindow::on_actionFileProperties_triggered() {
  if(currentIndex_.isValid()) {
    const auto file = proxyModel_->fileInfoFromIndex(currentIndex_);
    // it's better to use an async job to query the file info since it's
    // possible that loading of the folder is not finished and the file info is
    // not available yet, but it's overkill for a rarely used function.
    if(file)
      Fm::FilePropsDialog::showForFile(std::move(file));
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
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::on_actionPrevious_triggered() {
  if(proxyModel_->rowCount() <= 1)
    return;
  if(currentIndex_.isValid()) {
    QModelIndex index;
    if(currentIndex_.row() > 0)
      index = proxyModel_->index(currentIndex_.row() - 1, 0);
    else
      index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::on_actionFirst_triggered() {
  QModelIndex index = proxyModel_->index(0, 0);
  if(index.isValid()) {
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::on_actionLast_triggered() {
  QModelIndex index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
  if(index.isValid()) {
    const auto info = proxyModel_->fileInfoFromIndex(index);
    if(info)
      loadImage(info->path(), index);
  }
}

void MainWindow::loadFolder(const Fm::FilePath & newFolderPath) {
  if(folder_) { // an folder is already loaded
    if(newFolderPath == folderPath_) { // same folder, ignore
      return;
    }
    disconnect(folder_.get(), nullptr, this, nullptr); // disconnect from all signals
  }

  folderPath_ = newFolderPath;
  folder_ = Fm::Folder::fromPath(folderPath_);
  currentIndex_ = QModelIndex(); // set current index to invalid
  folderModel_->setFolder(folder_);
  if(folder_->isLoaded()) { // the folder may be already loaded elsewhere
      onFolderLoaded();
  }
  connect(folder_.get(), &Fm::Folder::finishLoading, this, &MainWindow::onFolderLoaded);
  connect(folder_.get(), &Fm::Folder::filesRemoved, this, &MainWindow::onFilesRemoved);
}

// the image is loaded (the method is only called if the loading is not cancelled)
void MainWindow::onImageLoaded() {
  // Note: As the signal finished() is emitted from different thread,
  // we can get it even after canceling the job (and setting the loadJob_
  // to nullptr). This simple check should be enough.
  if (sender() == loadJob_)
  {
    // Add to the MRU menu
    ui.menuRecently_Opened_Files->addItem(QString::fromUtf8(loadJob_->filePath().localPath().get()));

    image_ = loadJob_->image();
    exifData_ = loadJob_->getExifData();

    loadJob_ = nullptr; // the job object will be freed later automatically

    Settings& settings = static_cast<Application*>(qApp)->settings();

    int cs = settings.colorSpace();
    if(cs > 0 && cs < 6) {
      image_.convertToColorSpace(QColorSpace(static_cast<QColorSpace::NamedColorSpace>(cs)));
    }

    // set image zoom, like in loadImage()
    if(settings.forceZoomFit()) {
      ui.actionZoomFit->setChecked(true);
    }
    ui.view->setAutoZoomFit(ui.actionZoomFit->isChecked());
    if(ui.actionOriginalSize->isChecked()) {
      ui.view->zoomOriginal();
    }

    ui.view->setImage(image_);

    // currentIndex_ should be corrected after loading
    currentIndex_ = indexFromPath(currentFile_);

    updateUI();

    /* we resized and moved the window without showing
       it in updateUI(), so we need to show it here */
    if(!isVisible()) {
      if(startMaximized_) {
        setWindowState(windowState() | Qt::WindowMaximized);
      }
      showAndRaise();
    }
  }
}

void MainWindow::onImageSaved() {
  if(!saveJob_->failed()) {
    loadImage(saveJob_->filePath());
    loadFolder(saveJob_->filePath().parent());
  }
  saveJob_ = nullptr;
}

// filter events of other objects, mainly the image view.
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  if(watched == ui.view) { // we got an event for the image view
    switch(event->type()) {
      case QEvent::Wheel: { // mouse wheel event
        QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
        if(wheelEvent->modifiers() == 0) {
          QPoint angleDelta = wheelEvent->angleDelta();
          Qt::Orientation orient = (qAbs(angleDelta.x()) > qAbs(angleDelta.y()) ? Qt::Horizontal : Qt::Vertical);
          int delta = (orient == Qt::Horizontal ? angleDelta.x() : angleDelta.y());
          // NOTE: Each turn of a mouse wheel can change the image without problem but
          // touchpads trigger wheel events with much smaller angle deltas. Therefore,
          // we wait until a threshold is passed. 120 is an appropriate value because
          // most mouse types create angle deltas that are multiples of 120.
          static int deltaThreshold = 0;
          deltaThreshold += abs(delta);
          if(deltaThreshold >= 120) {
            deltaThreshold = 0;
            if(delta < 0)
              on_actionNext_triggered(); // next image
            else
              on_actionPrevious_triggered(); // previous image
          }
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
  return QObject::eventFilter(watched, event);
}

QModelIndex MainWindow::indexFromPath(const Fm::FilePath & filePath) {
  // if the folder is already loaded, figure out our index
  // otherwise, it will be done again in onFolderLoaded() when the folder is fully loaded.
  if(folder_ && folder_->isLoaded()) {
    QModelIndex index;
    int count = proxyModel_->rowCount();
    for(int row = 0; row < count; ++row) {
      index = proxyModel_->index(row, 0);
      const auto info = proxyModel_->fileInfoFromIndex(index);
      if(info && filePath == info->path()) {
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

    if(exifDataDock_) {
      setShowExifData(true);
    }
  }

  QString title;
  if(currentFile_) {
    const Fm::CStrPtr dispName = currentFile_.displayName();
    if(loadJob_) { // if loading is in progress
      title = tr("[*]%1 (Loading...) - Image Viewer")
                .arg(QString::fromUtf8(dispName.get()));
      ui.statusBar->setText();
    }
    else {
      if(image_.isNull()) {
        title = tr("[*]%1 (Failed to Load) - Image Viewer")
                  .arg(QString::fromUtf8(dispName.get()));
        ui.statusBar->setText();
      }
      else {
        const QString filePath = QString::fromUtf8(dispName.get());
        title = tr("[*]%1 (%2x%3) - Image Viewer")
                  .arg(filePath)
                  .arg(image_.width())
                  .arg(image_.height());
        ui.statusBar->setText(QStringLiteral("%1×%2").arg(image_.width()).arg(image_.height()),
                              filePath);
        if (!isVisible()) {
          /* Here we try to implement the following behavior as far as possible:
              (1) A minimum size of 400x400 is assumed;
              (2) The window is scaled to fit the image;
              (3) But for too big images, the window is scaled down;
              (4) The window is centered on the screen.

             To have a correct position, we should move the window BEFORE
             it's shown and we also need to know the dimensions of its view.
             Therefore, we first use show() without really showing the window. */

          // the maximization setting may be lost in resizeEvent because we resize the window below
          startMaximized_ = static_cast<Application*>(qApp)->settings().windowMaximized();
          setAttribute(Qt::WA_DontShowOnScreen);
          show();
          int scrollThickness = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
          QSize newSize = size() + image_.size() - ui.view->size() + QSize(scrollThickness, scrollThickness);
          QScreen *appScreen = QGuiApplication::screenAt(QCursor::pos());
          if(appScreen == nullptr) {
            appScreen = QGuiApplication::primaryScreen();
          }
          const QRect ag = appScreen ? appScreen->availableGeometry() : QRect();
          // since the window isn't decorated yet, we have to assume a max thickness for its frame
          QSize maxFrame = QSize(50, 100);
          if(newSize.width() > ag.width() - maxFrame.width()
             || newSize.height() > ag.height() - maxFrame.height()) {
            newSize.scale(ag.width() - maxFrame.width(), ag.height() - maxFrame.height(), Qt::KeepAspectRatio);
          }
          newSize = newSize.expandedTo(QSize(400, 400)); // a minimum size of 400x400 is good
          move(ag.x() + (ag.width() - newSize.width())/2,
               ag.y() + (ag.height() - newSize.height())/2);
          resize(newSize);
          hide(); // hide it to show it again later, at onImageLoaded()
          setAttribute(Qt::WA_DontShowOnScreen, false);
        }
      }
    }
    // TODO: update status bar, show current index in the folder
  }
  else {
    title = tr("[*]Image Viewer");
    ui.statusBar->setText();
  }
  setWindowTitle(title);
  setWindowModified(imageModified_);
}

// Load the specified image file asynchronously in a worker thread.
// When the loading is finished, onImageLoaded() will be called.
void MainWindow::loadImage(const Fm::FilePath & filePath, QModelIndex index) {
  // cancel loading of current image
  if(loadJob_) {
    loadJob_->cancel(); // the job object will be freed automatically later
    loadJob_ = nullptr;
  }
  if(imageModified_) {
    // TODO: ask the user to save the modified image?
    // this should be made optional
    setModified(false);
  }

  currentIndex_ = index;
  currentFile_ = filePath;
  // clear current image, but do not update the view now to prevent flickers
  image_ = QImage();

  const Fm::CStrPtr basename = currentFile_.baseName();
  char* mime_type = g_content_type_guess(basename.get(), nullptr, 0, nullptr);
  QString mimeType;
  if (mime_type) {
    mimeType = QString::fromUtf8(mime_type);
    g_free(mime_type);
  }
  if(mimeType == QLatin1String("image/gif")
     || mimeType == QLatin1String("image/svg+xml") || mimeType == QLatin1String("image/svg+xml-compressed")) {
    if(!currentIndex_.isValid()) {
      // since onImageLoaded is not called here,
      // currentIndex_ should be set
      currentIndex_ = indexFromPath(currentFile_);
    }
    const Fm::CStrPtr file_name = currentFile_.toString();

    // set image zoom, like in onImageLoaded()
    if(static_cast<Application*>(qApp)->settings().forceZoomFit()) {
      ui.actionZoomFit->setChecked(true);
    }
    ui.view->setAutoZoomFit(ui.actionZoomFit->isChecked());
    if(ui.actionOriginalSize->isChecked()) {
      ui.view->zoomOriginal();
    }

    if(mimeType == QLatin1String("image/gif"))
      ui.view->setGifAnimation(QString::fromUtf8(file_name.get()));
    else
      ui.view->setSVG(QString::fromUtf8(file_name.get()));
    image_ = ui.view->image();
    updateUI();
    if(!isVisible()) {
      if(startMaximized_) {
        setWindowState(windowState() | Qt::WindowMaximized);
      }
      showAndRaise();
    }
  }
  else {
    // start a new gio job to load the specified image
    loadJob_ = new LoadImageJob(currentFile_);
    connect(loadJob_, &Fm::Job::finished, this, &MainWindow::onImageLoaded);
    connect(loadJob_, &Fm::Job::error, this
        , [] (const Fm::GErrorPtr & err, Fm::Job::ErrorSeverity /*severity*/, Fm::Job::ErrorAction & /*response*/)
          {
            // TODO: show a info bar?
            qWarning().noquote() << "lximage-qt:" << err.message();
          }
        , Qt::BlockingQueuedConnection);
    loadJob_->runAsync();

    updateUI();
  }
}

void MainWindow::saveImage(const Fm::FilePath & filePath) {
  if(saveJob_) // do not launch a new job if the current one is still in progress
    return;
  // start a new gio job to save current image to the specified path
  saveJob_ = new SaveImageJob(ui.view->image(), filePath);
  connect(saveJob_, &Fm::Job::finished, this, &MainWindow::onImageSaved);
  connect(saveJob_, &Fm::Job::error, this
        , [this] (const Fm::GErrorPtr & err, Fm::Job::ErrorSeverity severity, Fm::Job::ErrorAction & /*response*/)
        {
          // TODO: show a info bar?
          if(severity > Fm::Job::ErrorSeverity::MODERATE) {
            QMessageBox::critical(this, QObject::tr("Error"), err.message());
          }
          else {
            qWarning().noquote() << "lximage-qt:" << err.message();
          }
        }
      , Qt::BlockingQueuedConnection);
  saveJob_->runAsync();
  // FIXME: add a cancel button to the UI? update status bar?
}

void MainWindow::on_actionRotateClockwise_triggered() {
  if(!image_.isNull()) {
    ui.view->rotateImage(true);
    image_ = ui.view->image();
    setModified(true);
  }
}

void MainWindow::on_actionRotateCounterclockwise_triggered() {
  if(!image_.isNull()) {
    ui.view->rotateImage(false);
    image_ = ui.view->image();
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

void MainWindow::on_actionCopyPath_triggered() {
  if(currentFile_) {
    const Fm::CStrPtr dispName = currentFile_.displayName();
    QApplication::clipboard()->setText(QString::fromUtf8(dispName.get()));
  }
}

void MainWindow::on_actionRenameFile_triggered() {
  // rename inline if the thumbnail bar is shown; otherwise, show the rename dialog
  if(!currentIndex_.isValid()) {
    return;
  }
  if(thumbnailsView_ && thumbnailsView_->isVisible()) {
    QAbstractItemView* view = thumbnailsView_->childView();
    view->scrollTo(currentIndex_);
    view->edit(currentIndex_);
  }
  else if(const auto file = proxyModel_->fileInfoFromIndex(currentIndex_)) {
    Fm::renameFile(file, this);
  }
}

void MainWindow::on_actionPaste_triggered() {
  QClipboard *clipboard = QApplication::clipboard();
  QImage image = clipboard->image();
  if(!image.isNull()) {
    pasteImage(image);
  }
}

void MainWindow::on_actionUpload_triggered()
{
    if(currentFile_.isValid()) {
      UploadDialog(this, QString::fromUtf8(currentFile_.localPath().get())).exec();
    }
    // if there is no open file, save the image to "/tmp" and delete it after upload
    else if(!ui.view->image().isNull()) {
      QString tmpFileName;
      QString tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
      if(!tmp.isEmpty()) {
        QDir tmpDir(tmp);
        if(tmpDir.exists()) {
          const QString curTime = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
          tmpFileName = tmp + QStringLiteral("/lximage-") + curTime + QStringLiteral(".png");
        }
      }
      if(!tmpFileName.isEmpty() && saveJob_ == nullptr) {
        const Fm::FilePath filePath = Fm::FilePath::fromPathStr(qPrintable(tmpFileName));
        saveJob_ = new SaveImageJob(ui.view->image(), filePath);
        connect(saveJob_, &Fm::Job::finished, this, [this, tmpFileName] {
          saveJob_ = nullptr;
          UploadDialog(this, tmpFileName).exec();
          QFile::remove(tmpFileName);
        });
        saveJob_->runAsync();
      }
    }
}

void MainWindow::on_actionFlipVertical_triggered() {
  if(!image_.isNull()) {
    ui.view->flipImage(false);
    image_ = ui.view->image();
    setModified(true);
  }
}

void MainWindow::on_actionFlipHorizontal_triggered() {
  if(!image_.isNull()) {
    ui.view->flipImage(true);
    image_ = ui.view->image();
    setModified(true);
  }
}

void MainWindow::on_actionResize_triggered() {
  if(image_.isNull()) {
    return;
  }
  ResizeImageDialog *dialog = new ResizeImageDialog(this);
  dialog->setOriginalSize(image_.size());
  if(dialog->exec() == QDialog::Accepted) {
    QSize newSize = dialog->scaledSize();
    if(ui.view->resizeImage(newSize)) {
      image_ = ui.view->image();
      setModified(true);
    }
  }
  dialog->deleteLater();
}

void MainWindow::setModified(bool modified) {
  imageModified_ = modified;
  updateUI(); // should be done even if imageModified_ is not changed (because of transformations)
}

void MainWindow::applySettings() {
  Application* app = static_cast<Application*>(qApp);
  Settings& settings = app->settings();
  if(isFullScreen())
    ui.view->setBackgroundBrush(QBrush(settings.fullScreenBgColor()));
  else
    ui.view->setBackgroundBrush(QBrush(settings.bgColor()));
  ui.view->updateOutline();
  ui.view->setSmoothOnZoom(settings.smoothOnZoom());
  ui.menuRecently_Opened_Files->setMaxItems(settings.maxRecentFiles());

  // also, update shortcuts
  setShortcuts(true);
}

// Sets or updates shortcuts.
void MainWindow::setShortcuts(bool update) {
  Application* app = static_cast<Application*>(qApp);
  const auto actions = findChildren<QAction*>();

  // get default shortcuts if this is the first window
  if(app->defaultShortcuts().isEmpty()) {
    QHash<QString, Application::ShortcutDescription> defaultShortcuts;
    for(const auto& action : actions) {
      if(action->objectName().isEmpty() || action->text().isEmpty()) {
        continue;
      }
      QKeySequence seq = action->shortcut();
      Application::ShortcutDescription s;
      s.displayText = action->text().remove(QLatin1Char('&')); // without mnemonics
      s.shortcut = seq;
      defaultShortcuts.insert(action->objectName(), s);
    }
    app->setDefaultShortcuts(defaultShortcuts);
  }

  auto hardCodedShortcuts = hardCodedShortcuts_;

  // set custom shortcuts
  QHash<QString, QString> ca = app->settings().customShortcutActions();
  for(const auto& action : actions) {
    const QString objectName = action->objectName();
    if(ca.contains(objectName)) {
      // custom shortcuts are saved in the PortableText format
      auto keySeq = QKeySequence(ca.take(objectName), QKeySequence::PortableText);
      action->setShortcut(keySeq);
      if(!hardCodedShortcuts.isEmpty()) {
        for(int i = 0; i < keySeq.count(); ++i) {
          if(hardCodedShortcuts.contains(keySeq[i])) { // would be ambiguous
            hardCodedShortcuts.take(keySeq[i])->setKey(QKeySequence());
          }
        }
      }
    }
    else if (update) { // restore default shortcuts
      action->setShortcut(app->defaultShortcuts().value(objectName).shortcut);
    }
    if(!update && ca.isEmpty()) {
      break;
    }
  }

  // set unambiguous hard-coded shortcuts too
  // but force Home and End keys if they are not action shortcuts
  if(hardCodedShortcuts.contains(Qt::Key_Home) && ui.actionFirst->shortcut() == QKeySequence(Qt::Key_Home)) {
    hardCodedShortcuts.take(Qt::Key_Home)->setKey(QKeySequence());
  }
  if(hardCodedShortcuts.contains(Qt::Key_End) && ui.actionLast->shortcut() == QKeySequence(Qt::Key_End)) {
    hardCodedShortcuts.take(Qt::Key_End)->setKey(QKeySequence());
  }
  QMap<int, QShortcut*>::const_iterator it = hardCodedShortcuts.constBegin();
  while (it != hardCodedShortcuts.constEnd()) {
    it.value()->setKey(QKeySequence(it.key()));
    ++it;
  }
}

void MainWindow::on_actionPrint_triggered() {
  // QPrinter printer(QPrinter::HighResolution);
  QPrinter printer;
  QPrintDialog dlg(&printer);
  if(dlg.exec() == QDialog::Accepted) {
    QPainter painter;
    painter.begin(&printer);

    // fit the target rectangle into the viewport if needed and center it
    const QRectF viewportRect = painter.viewport();
    QRectF targetRect = image_.rect();
    if(viewportRect.width() < targetRect.width()) {
      targetRect.setSize(QSize(viewportRect.width(), targetRect.height() * (viewportRect.width() / targetRect.width())));
    }
    if(viewportRect.height() < targetRect.height()) {
      targetRect.setSize(QSize(targetRect.width() * (viewportRect.height() / targetRect.height()), viewportRect.height()));
    }
    targetRect.moveCenter(viewportRect.center());

    // set the viewport and window of the painter and paint the image
    painter.setViewport(targetRect.toRect());
    painter.setWindow(image_.rect());
    painter.drawImage(0, 0, image_);

    painter.end();

    // FIXME: The following code divides the image into columns and could be used later as an option.
    /*QRect pageRect = printer.pageRect();
    int cols = (image_.width() / pageRect.width()) + (image_.width() % pageRect.width() ? 1 : 0);
    int rows = (image_.height() / pageRect.height()) + (image_.height() % pageRect.height() ? 1 : 0);
    for(int row = 0; row < rows; ++row) {
      for(int col = 0; col < cols; ++col) {
        QRect srcRect(pageRect.width() * col, pageRect.height() * row, pageRect.width(), pageRect.height());
        painter.drawImage(QPoint(0, 0), image_, srcRect);
        if(col + 1 == cols && row + 1 == rows) // this is the last page
          break;
        printer.newPage();
      }
    }
    painter.end();*/
  }
}

// TODO: This can later be used for doing slide show
void MainWindow::on_actionFullScreen_triggered(bool /*checked*/) {
  setWindowState(windowState() ^ Qt::WindowFullScreen);
}

void MainWindow::on_actionSlideShow_triggered(bool checked) {
  if(checked) {
    if(!slideShowTimer_) {
      slideShowTimer_ = new QTimer();
      // switch to the next image when timeout
      connect(slideShowTimer_, &QTimer::timeout, this, &MainWindow::on_actionNext_triggered);
    }
    Application* app = static_cast<Application*>(qApp);
    slideShowTimer_->start(app->settings().slideShowInterval() * 1000);
    // showFullScreen();
    ui.actionSlideShow->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
  }
  else {
    if(slideShowTimer_) {
      delete slideShowTimer_;
      slideShowTimer_ = nullptr;
      ui.actionSlideShow->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    }
  }
}

void MainWindow::on_actionShowThumbnails_triggered(bool checked) {
  setShowThumbnails(checked);
}

void MainWindow::on_actionShowOutline_triggered(bool checked) {
  ui.view->showOutline(checked);
}

void MainWindow::on_actionShowExifData_triggered(bool checked) {
  setShowExifData(checked);
  if(checked && exifDataDock_) {
    exifDataDock_->show(); // needed in the full-screen state
  }
}

void MainWindow::setShowThumbnails(bool show) {
  Settings& settings = static_cast<Application*>(qApp)->settings();

  if(show) {
    if(!thumbnailsDock_) {
      thumbnailsDock_ = new QDockWidget(this);
      thumbnailsDock_->setFeatures(QDockWidget::NoDockWidgetFeatures); // FIXME: should use DockWidgetClosable
      thumbnailsDock_->setWindowTitle(tr("Thumbnails"));
      thumbnailsView_ = new Fm::FolderView(Fm::FolderView::IconMode);
      thumbnailsView_->setIconSize(Fm::FolderView::IconMode, QSize(settings.thumbnailSize(), settings.thumbnailSize()));
      thumbnailsView_->setAutoSelectionDelay(0);
      thumbnailsDock_->setWidget(thumbnailsView_);
      addDockWidget(settings.thumbnailsPosition(), thumbnailsDock_);
      QListView* listView = static_cast<QListView*>(thumbnailsView_->childView());
      listView->setSelectionMode(QAbstractItemView::SingleSelection);
      Fm::FolderItemDelegate* delegate = static_cast<Fm::FolderItemDelegate*>(listView->itemDelegateForColumn(Fm::FolderModel::ColumnFileName));
      int frameWidth = thumbnailsView_->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, thumbnailsView_);
      int scrollBarExtent = thumbnailsView_->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, thumbnailsView_) ?
                            0 : thumbnailsView_->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
      switch(settings.thumbnailsPosition()) {
        case Qt::LeftDockWidgetArea:
        case Qt::RightDockWidgetArea:
          listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
          listView->setFlow(QListView::LeftToRight);
          if(delegate) {
            thumbnailsView_->setFixedWidth(delegate->itemSize().width() + 2 * frameWidth + scrollBarExtent);
          }
          break;
        case Qt::TopDockWidgetArea:
        case Qt::BottomDockWidgetArea:
        default:
          listView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
          listView->setFlow(QListView::TopToBottom);
          if(delegate) {
            thumbnailsView_->setFixedHeight(delegate->itemSize().height() + 2 * frameWidth + scrollBarExtent);
          }
          break;
      }
      thumbnailsView_->setModel(proxyModel_);
      proxyModel_->setShowThumbnails(true);
      if (currentFile_) { // select the loaded image
        currentIndex_ = indexFromPath(currentFile_);
        listView->setCurrentIndex(currentIndex_);
        // wait to center the selection
        QCoreApplication::processEvents();
        listView->scrollTo(currentIndex_, QAbstractItemView::PositionAtCenter);
      }
      connect(thumbnailsView_->selectionModel(), &QItemSelectionModel::selectionChanged,
              this, &MainWindow::onThumbnailSelChanged);
    }
    else if (!thumbnailsDock_->isVisible()) {
      thumbnailsDock_->show();
      ui.actionShowThumbnails->setChecked(true);
    }
  }
  else {
    if(thumbnailsDock_) {
      delete thumbnailsView_;
      thumbnailsView_ = nullptr;
      delete thumbnailsDock_;
      thumbnailsDock_ = nullptr;
    }
    proxyModel_->setShowThumbnails(false);
  }
}

void MainWindow::updateThumbnails() {
  if(thumbnailsView_ == nullptr) {
    return;
  }
  int thumbSize = static_cast<Application*>(qApp)->settings().thumbnailSize();
  QSize newSize(thumbSize, thumbSize);
  if(thumbnailsView_->iconSize(Fm::FolderView::IconMode) == newSize) {
    return;
  }

  thumbnailsView_->setIconSize(Fm::FolderView::IconMode, newSize);
  QListView* listView = static_cast<QListView*>(thumbnailsView_->childView());
  if(Fm::FolderItemDelegate* delegate = static_cast<Fm::FolderItemDelegate*>(listView->itemDelegateForColumn(Fm::FolderModel::ColumnFileName))) {
    int frameWidth = thumbnailsView_->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, thumbnailsView_);
    int scrollBarExtent = thumbnailsView_->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, thumbnailsView_) ?
                          0 : thumbnailsView_->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    if(listView->flow() == QListView::LeftToRight) {
      thumbnailsView_->setFixedWidth(delegate->itemSize().width() + 2 * frameWidth + scrollBarExtent);
    }
    else {
      thumbnailsView_->setFixedHeight(delegate->itemSize().height() + 2 * frameWidth + scrollBarExtent);
    }
  }
}

void MainWindow::setShowExifData(bool show) {
  Settings& settings = static_cast<Application*>(qApp)->settings();
  // Close the dock if it exists and show is false
  if(exifDataDock_ && !show) {
    settings.setExifDatakWidth(exifDataDock_->width());
    delete exifDataDock_;
    exifDataDock_ = nullptr;
  }

  // Be sure the dock was created before rendering content to it
  if(show && !exifDataDock_) {
    exifDataDock_ = new QDockWidget(tr("EXIF Data"), this);
    exifDataDock_->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::RightDockWidgetArea, exifDataDock_);
    resizeDocks({exifDataDock_}, {settings.exifDatakWidth()}, Qt::Horizontal);
  }

  // Render the content to the dock
  if(show) {
    QWidget* exifDataDockView_ = new QWidget();

    QVBoxLayout* exifDataDockViewContent_ = new QVBoxLayout();
    QTableWidget* exifDataContentTable_ = new QTableWidget();

    // Table setup
    exifDataContentTable_->setColumnCount(2);
    exifDataContentTable_->setShowGrid(false);
    exifDataContentTable_->horizontalHeader()->hide();
    exifDataContentTable_->verticalHeader()->hide();

    // The table is not editable
    exifDataContentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Write the EXIF Data to the table
    const auto keys =exifData_.keys();
    for(const QString& key : keys) {
      int rowCount = exifDataContentTable_->rowCount();

      exifDataContentTable_->insertRow(rowCount);
      exifDataContentTable_->setItem(rowCount,0, new QTableWidgetItem(key));
      exifDataContentTable_->setItem(rowCount,1, new QTableWidgetItem(exifData_.value(key)));
    }

    // Table setup after content was added
    exifDataContentTable_->resizeColumnsToContents();
    exifDataContentTable_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    exifDataDockViewContent_->addWidget(exifDataContentTable_);
    exifDataDockView_->setLayout(exifDataDockViewContent_);
    exifDataDock_->setWidget(exifDataDockView_);
  }
}

void MainWindow::changeEvent(QEvent* event) {
  // TODO: hide menu/toolbars in full screen mode and make the background black.
  if(event->type() == QEvent::WindowStateChange) {
    Settings& settings = static_cast<Application*>(qApp)->settings();
    if(isFullScreen()) { // changed to fullscreen mode
      ui.view->setFrameStyle(QFrame::NoFrame);
      ui.view->setBackgroundBrush(QBrush(settings.fullScreenBgColor()));
      ui.view->updateOutline();
      ui.toolBar->hide();
      ui.annotationsToolBar->hide();
      ui.statusBar->hide();
      // It's logical to hide the thumbnail dock on full-screening. The user could show it
      // in the full-screen mode explicitly.
      if(thumbnailsDock_) {
        thumbnailsDock_->hide();
        ui.actionShowThumbnails->setChecked(false);
      }
      if(exifDataDock_) {
        settings.setExifDatakWidth(exifDataDock_->width()); // the user may have resized it
        exifDataDock_->hide();
        ui.actionShowExifData->setChecked(false);
      }
      // menubar is hidden in fullscreen mode but we need its menu shortcuts
      on_actionMenubar_triggered(false);
      ui.view->hideCursor(true);
    }
    else { // restore to normal window mode
      ui.view->setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
      ui.view->setBackgroundBrush(QBrush(settings.bgColor()));
      ui.view->updateOutline();
      if(ui.actionMenubar->isChecked()) {
        on_actionMenubar_triggered(true);
      }
      if(ui.actionToolbar->isChecked()){
        ui.toolBar->show();
      }
      if(ui.actionAnnotations->isChecked()){
        ui.annotationsToolBar->show();
      }
      ui.statusBar->show();
      if(thumbnailsDock_) {
        // The thumbnail dock exists but was hidden on full-screening. So, it should be restored.
        thumbnailsDock_->show();
        ui.actionShowThumbnails->setChecked(true);
      }
      if(exifDataDock_) {
        // The exif data dock exists but was hidden on full-screening. So, it should be restored.
        exifDataDock_->show();
        ui.actionShowExifData->setChecked(true);
      }
      ui.view->hideCursor(false);
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
  if(exifDataDock_) {
    settings.setExifDatakWidth(exifDataDock_->width());
  }
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

void MainWindow::onKeyboardEscape() {
  if(isFullScreen())
    ui.actionFullScreen->trigger(); // will also "uncheck" the menu entry
  else
    on_actionClose_triggered();
}

void MainWindow::onThumbnailSelChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/) {
  // the selected item of thumbnail view is changed
  if(!selected.isEmpty()) {
    QModelIndex index = selected.indexes().first();
    if(index.isValid()) {
      // WARNING: Adding the condition index != currentIndex_ would be wrong because currentIndex_ may not be updated yet
      const auto file = proxyModel_->fileInfoFromIndex(index);
      if(file) {
        loadImage(file->path(), index);
        return;
      }
    }
  }
  // no image to show; reload to show a blank view and update variables
  on_actionReload_triggered();
}

void MainWindow::onFilesRemoved(const Fm::FileInfoList& files) {
  if(thumbnailsView_) {
    return; // onThumbnailSelChanged() will do the job
  }
  for(auto& file : files) {
    if(file->path() == currentFile_) {
      if(proxyModel_->rowCount() >= 1 && currentIndex_.isValid()) {
        QModelIndex index;
        if(currentIndex_.row() < proxyModel_->rowCount()) {
          index = currentIndex_;
        }
        else {
          index = proxyModel_->index(proxyModel_->rowCount() - 1, 0);
        }
        const auto info = proxyModel_->fileInfoFromIndex(index);
        if(info) {
          loadImage(info->path(), index);
          return;
        }
      }
      // no image to show; reload to show a blank view
      on_actionReload_triggered();
      return;
    }
  }
}

void MainWindow::onFileDropped(const QString path) {
    openImageFile(path);
}

void MainWindow::fileMenuAboutToShow() {
  // the "Open With..." submenu of Fm::FileMenu is shown
  // only if there is a file with a valid mime type
  if(currentIndex_.isValid()) {
    if(const auto file = proxyModel_->fileInfoFromIndex(currentIndex_)) {
      if(file->mimeType()) {
        ui.openWithMenu->setEnabled(true);
        return;
      }
    }
  }
  ui.openWithMenu->setEnabled(false);
}

void MainWindow::createOpenWithMenu() {
  if(currentIndex_.isValid()) {
    if(const auto file = proxyModel_->fileInfoFromIndex(currentIndex_)) {
      if(file->mimeType()) {
        // We want the "Open With..." submenu. It will be deleted alongside
        // fileMenu_ when openWithMenu hides (-> deleteOpenWithMenu)
        Fm::FileInfoList files;
        files.push_back(file);
        fileMenu_ = new Fm::FileMenu(files, file, Fm::FilePath());
        if(QMenu* menu = fileMenu_->openWithMenuAction()->menu()) {
          ui.openWithMenu->addActions(menu->actions());
        }
      }
    }
  }
}

void MainWindow::deleteOpenWithMenu() {
  if(fileMenu_) {
    fileMenu_->deleteLater();
  }
}

void MainWindow::showAndRaise() {
    if(showFullScreen_) {
      showFullScreen();
      ui.actionFullScreen->setChecked(true);
    }
    else {
      show();
    }
    raise();
    activateWindow();
    QTimer::singleShot (100, this, [this]() { // steal the focus forcefully
      if(QWindow *win = windowHandle()){
        win->requestActivate();
      }
    });
}
