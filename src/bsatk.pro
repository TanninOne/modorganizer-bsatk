#-------------------------------------------------
#
# Project created by QtCreator 2012-02-25T19:21:29
#
#-------------------------------------------------

QT       -= core gui

TARGET = bsatk
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    filehash.cpp \
    bsafile.cpp \
    bsaexception.cpp \
    bsafolder.cpp \
		bsaarchive.cpp \
    bsatypes.cpp \
    bsaarchive.cpp

HEADERS += \
    filehash.h \
    errorcodes.h \
    bsatypes.h \
    bsafile.h \
    bsafolder.h \
    bsaexception.h \
    bsaarchive.h


#QMAKE_CXXFLAGS+=-std=c++0x
INCLUDEPATH += "$(ZLIBPATH)" "$(ZLIBPATH)/build" "$(BOOSTPATH)"
LIBS +=  -L"$(ZLIBPATH)/build/Release" -lzlibstatic

DEFINES +=  BOOST_LIB_DIAGNOSTIC
