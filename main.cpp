#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include "Arduino.hpp"


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	qmlRegisterType<Arduino>("Arduino", 1, 0, "Arduino");

	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

	return app.exec();
}
