TARGET = "phase three"
TEMPLATE = app

MY_SRC = "src/main file.cpp" extra.cpp
include(common.pri)

QT += core gui widgets
QT -= gui

CONFIG *= c++17 c++17 console
SOURCES += $$MY_SRC \
    generated.cpp
HEADERS += mainwindow.h
FORMS += forms/main.ui
RESOURCES += resources/app.qrc
TRANSLATIONS += i18n/app_en.ts
INCLUDEPATH += "include dir" $$PWD/include
DEPENDPATH += deps
LIBS += -L"lib dir" -framework Cocoa customlib
QMAKE_CXXFLAGS += -Wall -Wextra
QMAKE_LFLAGS += -pthread
INSTALLS += target

win32:SOURCES += "win path/win.cpp"
else:DEFINES += NOT_WIN

macx {
    SOURCES += mac.mm
    LIBS += -framework AppKit
}

contains(QT, widgets):DEFINES += HAS_WIDGETS
message(hello from fixture)
