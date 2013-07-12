#-------------------------------------------------
#
# Project created by QtCreator 2013-07-04T18:34:05
#
#-------------------------------------------------

QT       += core network script

QT       -= gui

TARGET = Mileina
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += src/main.cpp \
    src/qirc.cpp \
    src/timermanager.cpp

HEADERS += \
    src/qirc.h \
    src/timermanager.h

OTHER_FILES += \
    etc/config.json.dist \
    README.md
