#ifndef GLWRAP_H
#define GLWRAP_H

#ifdef DESKTOP_GL
#include <NativeGfxApi/glad/gl.h>
#else
#include <NativeGfxApi/glad/gles2.h>
#endif
inline GLbitfield generateGlMappedBitfield(bool needsToRead, bool entireBuffer) {
	if(needsToRead) {
		return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
	} else {
		if(entireBuffer) {
			return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
		} else {
			return GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
		}
	}
}

#endif // GLWRAP_H
