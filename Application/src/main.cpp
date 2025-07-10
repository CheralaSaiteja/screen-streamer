#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtGui/QGuiApplication>
#include <stdio.h>

int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);

  // Create an engine to load QML files
  QQmlApplicationEngine engine;

  engine.addImportPath(":/");

  engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

  if (engine.rootObjects().isEmpty())
    return -1;

  fprintf(stdout, "started application\n");

  return app.exec();
}