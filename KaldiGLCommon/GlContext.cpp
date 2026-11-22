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
#include <atomic>
#include <stdexcept>
#include <string>

namespace KALDIGL_NS {

namespace {

/**
 * @brief Number of live users of the process-wide GL entry points.
 *
 * glad's function pointers are global, so unloading them while a second device
 * still exists would leave that device with null pointers. The counter makes
 * loadGlFunctions()/unloadGlFunctions() a balanced pair per device.
 */
std::atomic<int> glLoadUsers { 0 };

} // namespace

int loadGlFunctions() noexcept
{
#if KALDIGL_IS_GLES
	const int version = gladLoaderLoadGLES2();
#else
	const int version = gladLoaderLoadGL();
#endif
	if (version != 0) {
		glLoadUsers.fetch_add(1, std::memory_order_relaxed);
	}
	return version;
}

void unloadGlFunctions() noexcept
{
	if (glLoadUsers.fetch_sub(1, std::memory_order_acq_rel) > 1) {
		return;
	}
	glLoadUsers.store(0, std::memory_order_relaxed);
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

std::unique_ptr<Context> createContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config)
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

#if KALDIGL_IS_GLES
	// The ES backend has exactly one path; an explicit GLX/WGL request is a caller
	// error, not something to silently reinterpret.
	if (config.api != ContextApi::Auto && config.api != ContextApi::EGL) {
		throw std::runtime_error("GlContext: the OpenGL ES backend only supports EGL");
	}
	return createEglContext(handles, config);
#else
	ContextApi api = config.api;
	if (api == ContextApi::Auto) {
		switch (handles.system) {
			case Kld::WindowSystem::X11: api = ContextApi::GLX; break;
			case Kld::WindowSystem::Windows: api = ContextApi::WGL; break;
			default: api = ContextApi::EGL; break;
		}
	}
	switch (api) {
		case ContextApi::EGL:
		case ContextApi::Auto:
			return createEglContext(handles, config);
		case ContextApi::GLX:
			return createGlxContext(handles, config);
		case ContextApi::WGL:
#if defined(_WIN32)
			return createWglContext(handles, config);
#else
			throw std::runtime_error("GlContext: WGL is only available on Windows hosts");
#endif
	}
	throw std::runtime_error("GlContext: no context implementation is available for the selected API");
#endif
}

std::unique_ptr<Context> createContext(const SDL_SysWMinfo& info, const ContextConfig& config)
{
	return createContext(Kld::nativeWindowHandlesFrom(info), config);
}

}
