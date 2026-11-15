# KaldiD3DCommon - shared Direct3D 11 / Direct3D 12 backend support code.
#
# Like KaldiGLCommon this is *not* a standalone qmake project: the sources are
# compiled into the consuming backend.
#
#   KALDID3D_COMMON_SOURCES        - portable SPIR-V -> HLSL translation
#                                    (SPIRV-Cross only, compiles on any host)
#   KALDID3D_COMMON_WIN32_SOURCES  - DXGI format tables, needs <dxgiformat.h>
#                                    from the Windows SDK or DirectX-Headers
#
# Consumers must add:
#
#   include(../KaldiD3DCommon/KaldiD3DCommon.pri)
#   SOURCES += $$KALDID3D_COMMON_SOURCES
#   win32: SOURCES += $$KALDID3D_COMMON_WIN32_SOURCES
#
INCLUDEPATH += $$quote($$PWD)

KALDID3D_COMMON_SOURCES += \
    $$PWD/D3DSPirvShader.cpp

KALDID3D_COMMON_WIN32_SOURCES += \
    $$PWD/D3DUtil.cpp

KALDID3D_COMMON_HEADERS += \
    $$PWD/D3DSPirvShader.hpp \
    $$PWD/D3DUtil.hpp
