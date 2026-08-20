#-------------------------------------------------
#
# Project created by QtCreator 2013-11-26T17:11:11
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = q2c
CONFIG   += console
CONFIG   -= app_bundle
DEFINES += Q2C_VERSION=\\\"0.1.0\\\"

TEMPLATE = app


SOURCES += main.cpp \
    terminalparser.cpp \
    configuration.cpp \
    project.cpp \
    logs.cpp \
    generic.cpp \
    buildmodel.cpp \
    qmakeparser.cpp \
    cmakeparser.cpp \
    cmakegenerator.cpp \
    qmakegenerator.cpp

HEADERS += \
    terminalparser.h \
    configuration.h \
    project.h \
    logs.h \
    generic.h \
    buildmodel.h \
    qmakeparser.h \
    cmakeparser.h \
    cmakegenerator.h \
    qmakegenerator.h
