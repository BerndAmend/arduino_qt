TEMPLATE = app

CONFIG += C++11

QT += qml quick widgets websockets

SOURCES += main.cpp \
    Arduino.cpp

!android {
	DEFINES += HAS_QSERIALPORT
	QT += serialport
}

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    Arduino.hpp
