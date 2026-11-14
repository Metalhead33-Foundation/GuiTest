TEMPLATE = lib
TARGET = KaldiD3D12
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi.pri)

# Shader cross-compilation support (SPIR-V -> GLSL/HLSL) is a backend concern.
LIBS += -lspirv-cross-core -lspirv-cross-glsl

# Avoid build errors with no files:
SOURCES +=  \
    KaldiD3D12Device.cpp
HEADERS += \
    KaldiD3D12Device.hpp \
    KldImplLib.h

DISTFILES += \
    README.md
