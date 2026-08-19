TARGET = complex_app
TEMPLATE = app

QT += core gui widgets network
QT -= gui
CONFIG += c++17 testcase plugin

include(shared.pri)

APP_SOURCES = src/main.cpp src/window.cpp "src/file with space.cpp"
APP_HEADERS = include/window.h include/appconfig.h

SOURCES += $$APP_SOURCES
HEADERS += $$APP_HEADERS
FORMS += ui/mainwindow.ui ui/preferences.ui
RESOURCES += resources/app.qrc resources/icons.qrc
TRANSLATIONS += i18n/complex_en.ts i18n/complex_de.ts
DEFINES += COMPLEX_APP VERSION=\\\"1.2.3\\\"
INCLUDEPATH += include $$PWD/generated
DEPENDPATH += generated
LIBS += -Llib -lssl -lcrypto -framework Security
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
QMAKE_LFLAGS += -pthread
INSTALLS += target translations

win32: SOURCES += platform/win.cpp
win32: DEFINES += PLATFORM_WINDOWS

unix {
    SOURCES += platform/unix.cpp
    DEFINES += PLATFORM_UNIX
    LIBS += -ldl
}

macx {
    SOURCES += platform/mac.mm
    LIBS += -framework Cocoa
}

contains(QT, network): DEFINES += HAS_NETWORK_MODULE

