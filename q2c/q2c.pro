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

TEMPLATE = app


SOURCES += main.cpp \
    terminalparser.cpp \
    configuration.cpp \
    project.cpp \
    logs.cpp \
    generic.cpp

HEADERS += \
    terminalparser.h \
    configuration.h \
    project.h \
    logs.h \
    generic.h
