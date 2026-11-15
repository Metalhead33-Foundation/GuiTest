# KaldiGLCommon - shared OpenGL / OpenGL ES backend support code.
#
# This is *not* a standalone qmake project. The sources are compiled directly
# into each consuming backend library, because a translation unit can only be
# built for one dialect at a time:
#
#   * KaldiGL.pro   defines GL_IMPLEMENTATION   -> namespace GL   (OpenGL 4.3 core)
#   * KaldiGLES.pro defines GLES_IMPLEMENTATION -> namespace GLES (OpenGL ES 3.2)
#
# Consumers must include ../Kaldi/Kaldi.pri (for the Kaldi include path) and add:
#
#   include(../KaldiGLCommon/KaldiGLCommon.pri)
#   SOURCES += $$KALDIGL_COMMON_SOURCES
#   HEADERS += $$KALDIGL_COMMON_HEADERS
#
INCLUDEPATH += $$quote($$PWD)

# Context layer: the abstract contract in GlContext.cpp, its EGL implementation,
# and the display the EGL contexts share. GlxContext lives in KaldiGL because GLX
# is desktop-only.
KALDIGL_COMMON_SOURCES += \
    $$PWD/GlContext.cpp \
    $$PWD/EglDisplay.cpp \
    $$PWD/EglContext.cpp \
    $$PWD/GlSpirvShader.cpp \
    $$PWD/GlUtil.cpp

KALDIGL_COMMON_HEADERS += \
    $$PWD/GlBackend.hpp \
    $$PWD/GlContext.hpp \
    $$PWD/EglDisplay.hpp \
    $$PWD/EglContext.hpp \
    $$PWD/GlSpirvShader.hpp \
    $$PWD/GlUtil.hpp

