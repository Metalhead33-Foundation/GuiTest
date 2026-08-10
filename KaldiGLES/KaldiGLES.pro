TEMPLATE = lib
TARGET = KaldiGLES
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi.pri)

# Avoid build errors with no files:
SOURCES += \
    KaldiGLESDevice.cpp
HEADERS += \
    KaldiGLESDevice.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
