#-------------------------------------------------
#
# Project created by QtCreator 2012-02-25T19:21:29
#
#-------------------------------------------------

QT       -= core gui

TARGET = bsatk
TEMPLATE = lib
CONFIG += staticlib

!include(../LocalPaths.pri) {
  message("paths to required libraries need to be set up in LocalPaths.pri")
}

SOURCES += \
    filehash.cpp \
    bsafile.cpp \
    bsaexception.cpp \
    bsafolder.cpp \
    bsaarchive.cpp \
    bsatypes.cpp

HEADERS += \
    filehash.h \
    errorcodes.h \
    bsatypes.h \
    bsafile.h \
    bsafolder.h \
    bsaexception.h \
    bsaarchive.h


INCLUDEPATH += "$${ZLIBPATH}" "$${ZLIBPATH}/build" "$${BOOSTPATH}"
LIBS += -L"$${ZLIBPATH}/build" -lzlibstatic

DEFINES += BOOST_LIB_DIAGNOSTIC NOMINMAX
