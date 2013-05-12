#include <QApplication>
#include <QIcon>
#include "lximage-qt.h"
#include "mainwindow.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  
  // FIXME: read icon theme name from config
  QIcon::setThemeName("oxygen");
  
  LxImage::MainWindow mainWindow;
  mainWindow.show();
  return app.exec();
}
