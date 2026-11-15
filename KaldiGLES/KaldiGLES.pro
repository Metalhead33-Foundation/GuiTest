TEMPLATE = lib
TARGET = KaldiGLES
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../glad/include
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi/Kaldi.pri)

# Shader cross-compilation support (SPIR-V -> GLSL/HLSL) is a backend concern.
LIBS += -lspirv-cross-core -lspirv-cross-glsl

# Avoid build errors with no files:
SOURCES += \
    ../glad/src/egl.c \
    ../glad/src/gles2.c \
    KaldiGLESDevice.cpp
HEADERS += \
    KaldiGLESDevice.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
