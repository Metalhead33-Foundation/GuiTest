TEMPLATE = lib
TARGET = KaldiD3D11 
CONFIG += shared c++2a
CONFIG -= qt

DEFINES += MH_KALDI_IMPL_EXPORT

LIBS += -L$$OUT_PWD/../Kaldi -lKaldi
# Avoid build errors with no files:
SOURCES += 
HEADERS += \
    KldImplLib.hpp

DISTFILES += \
    README.md
