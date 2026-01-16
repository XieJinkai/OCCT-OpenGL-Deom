#include <QApplication>

#include "App/MainWindow.h"

int main(int argc, char* argv[])
{
  QApplication app(argc, argv);

  MainWindow w;
  w.resize(1200, 800);
  w.show();

  return app.exec();
}

