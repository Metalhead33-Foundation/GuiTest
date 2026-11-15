TEMPLATE = lib
TARGET = KaldiD3D12
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../extern/DirectX-Headers/include
DEFINES += MH_KALDI_IMPL_EXPORT
include(../Kaldi/Kaldi.pri)

# D3DUtil (DXGI tables) and D3DSPirvShader (SPIR-V -> HLSL) are shared with
# KaldiD3D11; only the DXGI dependent part is Windows-only.
include(../KaldiD3DCommon/KaldiD3DCommon.pri)

# Shader cross-compilation support (SPIR-V -> HLSL) is a backend concern.
LIBS += -lspirv-cross-core -lspirv-cross-hlsl

# Avoid build errors with no files:
SOURCES +=  \
    KaldiD3D12Device.cpp \
    $$KALDID3D_COMMON_SOURCES
HEADERS += \
    KaldiD3D12Device.hpp \
    KldImplLib.h \
    $$KALDID3D_COMMON_HEADERS

# D3D12Util includes <directx/d3d12.h>, which only exists on Windows hosts.
win32 {
    SOURCES += D3D12Util.cpp
    HEADERS += D3D12Util.hpp
}

DISTFILES += \
    README.md
