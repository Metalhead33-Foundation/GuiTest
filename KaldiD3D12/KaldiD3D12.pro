TEMPLATE = lib
TARGET = KaldiD3D12
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi.pri)

# Avoid build errors with no files:
SOURCES +=  \
    KaldiD3D12Device.cpp
HEADERS += \
    KaldiD3D12Device.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
