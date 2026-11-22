TEMPLATE = lib
TARGET = KaldiGL 
CONFIG += shared c++2a
CONFIG -= qt

INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../glad/include
DEFINES += MH_KALDI_IMPL_EXPORT
DEFINES += GL_IMPLEMENTATION
include(../Kaldi/Kaldi.pri)

# GlUtil and GlSpirvShader are shared with KaldiGLES. They are compiled into
# each backend separately so the GLES_IMPLEMENTATION dialect switch works.
include(../KaldiGLCommon/KaldiGLCommon.pri)

# Shader cross-compilation support (SPIR-V -> GLSL) is a backend concern.
LIBS += -lspirv-cross-core -lspirv-cross-glsl

# GlxContext calls Xlib directly (XGetWindowAttributes, XVisualIDFromVisual,
# XFree, ...), so the desktop backend has to link libX11 explicitly. EGL is
# loaded at runtime through glad.
unix:!macx: LIBS += -lX11

# Avoid build errors with no files:
SOURCES +=  \
    ../glad/src/egl.c \
    ../glad/src/gl.c \
    ../glad/src/glx.c \
    ../glad/src/wgl.c \
    GlxContext.cpp \
    KaldiGLDevice.cpp \
    $$KALDIGL_COMMON_SOURCES
HEADERS += \
    GlxContext.hpp \
    KaldiGLDevice.hpp \
    KldImplLib.h \
    $$KALDIGL_COMMON_HEADERS

# WGL is the desktop-GL binding on Win32 and pulls in <glad/wgl.h> (windows.h).
win32 {
    SOURCES += WglContext.cpp
    HEADERS += WglContext.hpp
}

DISTFILES += \
    README.md
