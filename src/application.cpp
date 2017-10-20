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


#include "application.h"
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QPixmapCache>
#include "applicationadaptor.h"
#include "screenshotdialog.h"
#include "preferencesdialog.h"
#include "mainwindow.h"

using namespace LxImage;

static const char* serviceName = "org.lxde.LxImage";
static const char* ifaceName = "org.lxde.LxImage.Application";

Application::Application(int& argc, char** argv):
  QApplication(argc, argv),
  libFm(),
  windowCount_(0) {
  setApplicationVersion(LXIMAGE_VERSION);
}

bool Application::init(int argc, char** argv) {
  Q_UNUSED(argc)
  Q_UNUSED(argv)

  setAttribute(Qt::AA_UseHighDpiPixmaps, true);

  // install the translations built-into Qt itself
  qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  installTranslator(&qtTranslator);

  // install libfm-qt translator
  installTranslator(libFm.translator());

  // install our own tranlations
  translator.load("lximage-qt_" + QLocale::system().name(), LXIMAGE_DATA_DIR "/translations");
  installTranslator(&translator);

  // initialize dbus
  QDBusConnection dbus = QDBusConnection::sessionBus();
  if(dbus.registerService(serviceName)) {
    settings_.load(); // load settings
    // we successfully registered the service
    isPrimaryInstance = true;
    setQuitOnLastWindowClosed(false); // do not quit even when there're no windows

    new ApplicationAdaptor(this);
    dbus.registerObject("/Application", this);

    connect(this, &Application::aboutToQuit, this, &Application::onAboutToQuit);

    if(settings_.useFallbackIconTheme())
      QIcon::setThemeName(settings_.fallbackIconTheme());
  }
  else {
    // an service of the same name is already registered.
    // we're not the first instance
    isPrimaryInstance = false;
  }

  QPixmapCache::setCacheLimit(1024); // avoid pixmap caching.

  return parseCommandLineArgs();
}

bool Application::parseCommandLineArgs() {
  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption screenshotOption(
    QStringList() << "s" << "screenshot",
    tr("Take a screenshot")
  );
  parser.addOption(screenshotOption);

  const QString files = tr("[FILE1, FILE2,...]");
  parser.addPositionalArgument("files", files, files);

  parser.process(*this);

  const QStringList args = parser.positionalArguments();
  const bool screenshotTool = parser.isSet(screenshotOption);

  QStringList paths;
  Q_FOREACH(QString arg, args) {
    QFileInfo info(arg);
    paths.push_back(info.absoluteFilePath());
  }

  bool keepRunning = false;
  if(isPrimaryInstance) {
    settings_.load();
    keepRunning = true;
    if(screenshotTool) {
      screenshot();
    }
    else {
      newWindow(paths);
    }
  }
  else {
    // we're not the primary instance.
    // call the primary instance via dbus to do operations
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusInterface iface(serviceName, "/Application", ifaceName, dbus, this);
    if(screenshotTool)
      iface.call("screenshot");
    else
      iface.call("newWindow", paths);
  }
  return keepRunning;
}

MainWindow* Application::createWindow() {
  LxImage::MainWindow* window;
  window = new LxImage::MainWindow();
  return window;
}

void Application::newWindow(QStringList files) {
  LxImage::MainWindow* window;
  if(files.empty()) {
    window = createWindow();

    window->resize(settings_.windowWidth(), settings_.windowHeight());
    if(settings_.windowMaximized())
      window->setWindowState(window->windowState() | Qt::WindowMaximized);

    window->show();
  }
  else {
    Q_FOREACH(QString fileName, files) {
      window = createWindow();
      window->openImageFile(fileName);

      window->resize(settings_.windowWidth(), settings_.windowHeight());
      if(settings_.windowMaximized())
        window->setWindowState(window->windowState() | Qt::WindowMaximized);

      /* when there's an image, we show the window AFTER resizing
         and centering it appropriately at MainWindow::updateUI() */
      //window->show();
    }
  }
}

void Application::applySettings() {
  Q_FOREACH(QWidget* window, topLevelWidgets()) {
    if(window->inherits("LxImage::MainWindow"))
      static_cast<MainWindow*>(window)->applySettings();
  }
}

void Application::screenshot() {
  ScreenshotDialog* dlg = new ScreenshotDialog();
  dlg->show();
}

void Application::editPreferences() {
  PreferencesDialog* dlg = new PreferencesDialog();
  dlg->show();
}

void Application::onAboutToQuit() {
  settings_.save();
}
