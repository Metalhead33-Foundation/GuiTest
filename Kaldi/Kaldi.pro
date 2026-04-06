TEMPLATE = lib
CONFIG += shared c++2a
TARGET = Kaldi 
CONFIG -= qt

DEFINES += MH_KALDI_EXPORT

include(Kaldi.pri)

# Avoid build errors with no files:
HEADERS += \
    KaldiLib.hpp \
    KaldiOperation.hpp \
    KldDevice.hpp \
    KldVertexFormat.hpp

DISTFILES += \
    Kaldi.pri \
    README.md

SOURCES += \
    KldDevice.cpp
