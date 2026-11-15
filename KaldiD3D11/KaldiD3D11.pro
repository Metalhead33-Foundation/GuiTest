TEMPLATE = lib
TARGET = KaldiD3D11 
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../extern/DXSDK/Include
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi/Kaldi.pri)

# D3DUtil (DXGI tables) and D3DSPirvShader (SPIR-V -> HLSL) are shared with
# KaldiD3D12; only the DXGI dependent part is Windows-only.
include(../KaldiD3DCommon/KaldiD3DCommon.pri)

# Shader cross-compilation support (SPIR-V -> HLSL) is a backend concern.
LIBS += -lspirv-cross-core -lspirv-cross-hlsl

# Avoid build errors with no files:
SOURCES +=  \
    KaldiD3D11Device.cpp \
    $$KALDID3D_COMMON_SOURCES
HEADERS += \
    KaldiD3D11Device.hpp \
    KldImplLib.h \
    $$KALDID3D_COMMON_HEADERS

# D3D11Util includes <d3d11.h>, which only exists on Windows hosts.
win32 {
    SOURCES += D3D11Util.cpp D3D11Context.cpp
    HEADERS += D3D11Util.hpp D3D11Context.hpp
}

DISTFILES += \
    README.md
