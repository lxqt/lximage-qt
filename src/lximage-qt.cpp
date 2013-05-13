#include <QApplication>
#include <QIcon>
#include "application.h"

int main(int argc, char** argv) {
  LxImage::Application app(argc, argv);
  if(!app.init())
    return 1;
  return app.exec();
}
