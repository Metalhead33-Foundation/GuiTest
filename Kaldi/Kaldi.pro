TEMPLATE = lib
CONFIG += shared c++2a
TARGET = Kaldi 
CONFIG -= qt

DEFINES += MH_KALDI_EXPORT

include(Kaldi.pri)

# Avoid build errors with no files:
HEADERS += \
    KaldiLib.h \
    KaldiOperation.hpp \
    KldCommandBuffer.hpp \
    KldDevice.hpp \
    KldDispatch.hpp \
    KldHal.hpp \
    KldNativeWindow.hpp \
    KldVertexFormat.hpp

DISTFILES += \
    ../docs/graphics/KALDI_TUTORIAL.md \
    ../docs/graphics/SHADERS.md \
    ../docs/graphics/index.md \
    Kaldi.pri \
    README.md

SOURCES +=
