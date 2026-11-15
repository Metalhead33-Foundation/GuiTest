TEMPLATE = lib
TARGET = KaldiGLES
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../glad/include
DEFINES += MH_KALDI_IMPL_EXPORT
DEFINES += GLES_IMPLEMENTATION
include(../Kaldi/Kaldi.pri)

# GlUtil and GlSpirvShader are shared with KaldiGL. They are compiled into
# each backend separately so the GLES_IMPLEMENTATION dialect switch works.
include(../KaldiGLCommon/KaldiGLCommon.pri)

# Shader cross-compilation support (SPIR-V -> GLSL ES) is a backend concern.
LIBS += -lspirv-cross-core -lspirv-cross-glsl

# Avoid build errors with no files:
SOURCES += \
    ../glad/src/egl.c \
    ../glad/src/gles2.c \
    KaldiGLESDevice.cpp \
    $$KALDIGL_COMMON_SOURCES
HEADERS += \
    KaldiGLESDevice.hpp \
    KldImplLib.h \
    $$KALDIGL_COMMON_HEADERS

DISTFILES += \
    README.md
