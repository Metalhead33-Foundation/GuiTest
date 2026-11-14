TEMPLATE = lib
TARGET = KaldiD3D11 
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi.pri)

# Shader cross-compilation support (SPIR-V -> GLSL/HLSL) is a backend concern.
LIBS += -lspirv-cross-core -lspirv-cross-glsl

# Avoid build errors with no files:
SOURCES +=  \
    KaldiD3D11Device.cpp
HEADERS += \
    KaldiD3D11Device.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
