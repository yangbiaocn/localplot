#-------------------------------------------------
#
# Project created by QtCreator 2015-12-17T21:05:53
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = localplot
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    hpgl_obj.cpp

HEADERS  += mainwindow.h \
    hpgl_obj.h

FORMS    += mainwindow.ui