/**
 * @file GlContext.cpp
 * @brief Implements the window-system independent parts of GlContext.hpp.
 *
 * The dialect switch picks the glad loader that loadGlFunctions() drives; the
 * factory dispatches to the EGL or GLX implementation, both of which live in
 * their own translation units.
 */
#include "GlContext.hpp"
#if KALDIGL_IS_GLES
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include <stdexcept>
#include <string>

namespace KALDIGL_NS {

int loadGlFunctions() noexcept
{
#if KALDIGL_IS_GLES
	return gladLoaderLoadGLES2();
#else
	return gladLoaderLoadGL();
#endif
}

void unloadGlFunctions() noexcept
{
#if KALDIGL_IS_GLES
	gladLoaderUnloadGLES2();
#else
	gladLoaderUnloadGL();
#endif
}

void applyDefaultState()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

std::unique_ptr<Context> createContext(const NativeWindowHandles& handles, const ContextConfig& config)
{
	if (!handles.usable()) {
		std::string message = "GlContext: cannot create a context: ";
		if (handles.unsupportedReason.empty()) {
			message += "the window has no usable native drawable";
		} else {
			message.append(handles.unsupportedReason);
		}
		throw std::invalid_argument(message);
	}

	const ContextApi api = (config.api == ContextApi::Auto) ? handles.suggestedApi : config.api;
	switch (api) {
		case ContextApi::GLX:
#if KALDIGL_IS_GLES
			throw std::runtime_error("GlContext: GLX is not available in the OpenGL ES backend");
#else
			return createGlxContext(handles, config);
#endif
		case ContextApi::EGL:
		case ContextApi::Auto:
			break;
	}
	return createEglContext(handles, config);
}

std::unique_ptr<Context> createContext(const SDL_SysWMinfo& info, const ContextConfig& config)
{
	return createContext(nativeHandlesFrom(info), config);
}

}
