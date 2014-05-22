#-------------------------------------------------
#
# Project created by QtCreator 2014-05-15T22:44:35
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = showCameraInWidget
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    IMGWidget.cpp \
    funcForThread.cpp

HEADERS  += mainwindow.h \
    IMGWidget.h \
    funcForThread.h

FORMS    += mainwindow.ui

INCLUDEPATH += /usr/local/include \
                /usr/local/include/opencv \
                /usr/local/include/opencv2

LIBS += /usr/lib/libopencv_highgui.so \
        /usr/lib/libopencv_core.so    \
        /usr/lib/libopencv_imgproc.so
