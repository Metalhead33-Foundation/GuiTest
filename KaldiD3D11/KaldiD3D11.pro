TEMPLATE = lib
TARGET = KaldiD3D11 
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi.pri)

# Avoid build errors with no files:
SOURCES +=  \
    KaldiD3D11Device.cpp
HEADERS += \
    KaldiD3D11Device.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
