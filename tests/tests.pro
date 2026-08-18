QT += core
QT -= gui

TARGET = q2c_tests
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../q2c
DEFINES += TEST_FIXTURE_DIR=\\\"$$PWD/fixtures\\\"

SOURCES += main.cpp \
    ../q2c/buildmodel.cpp \
    ../q2c/cmakeparser.cpp \
    ../q2c/cmakegenerator.cpp \
    ../q2c/generic.cpp \
    ../q2c/logs.cpp \
    ../q2c/qmakegenerator.cpp \
    ../q2c/qmakeparser.cpp \
    ../q2c/configuration.cpp

HEADERS += \
    ../q2c/buildmodel.h \
    ../q2c/cmakeparser.h \
    ../q2c/cmakegenerator.h \
    ../q2c/generic.h \
    ../q2c/logs.h \
    ../q2c/qmakegenerator.h \
    ../q2c/qmakeparser.h \
    ../q2c/configuration.h
