#ifndef GLWRAP_H
#define GLWRAP_H

#ifdef DESKTOP_GL
#include <NativeGfxApi/glad/gl.h>
#else
#include <NativeGfxApi/glad/gles2.h>
#endif

#endif // GLWRAP_H
