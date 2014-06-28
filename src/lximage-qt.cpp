#include <QApplication>
#include <QIcon>
#include "application.h"

int main(int argc, char** argv) {
  LxImage::Application app(argc, argv);
  if(!app.init(argc, argv))
    return 0;
  return app.exec();
}
