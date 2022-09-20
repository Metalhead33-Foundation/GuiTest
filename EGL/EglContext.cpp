#include "EglContext.hpp"
#include <sstream>

namespace EGL {

enum EglErrorType {
	CONFIG,
	SURFACE,
	CONTEXT
};
class EglError : public std::exception {
private:
	std::string str;
public:
	EglError(EglErrorType errType) {
		std::stringstream strm;
		switch (errType) {
			case CONFIG: strm << "Failed to choose config (eglError: " << std::hex << eglGetError() << ")."; break;
			case SURFACE: strm << "Failed to create surface (eglError: " << std::hex << eglGetError() << ")."; break;
			case CONTEXT: strm << "Failed to create context (eglError: " << std::hex << eglGetError() << ")."; break;
		}
		str = strm.str();
	}
	const char* what() const noexcept override { return str.c_str(); }
};

static EGLint attr[] = {
	EGL_BUFFER_SIZE, 16,
	EGL_RENDERABLE_TYPE,
	EGL_OPENGL_ES2_BIT,
	EGL_NONE
};

static EGLint ctxattr[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

EGLConfig Context::getConfig() const
{
	return config;
}

const Display& Context::getDisplay() const
{
	return display;
}

Display& Context::getDisplay()
{
	return display;
}

void Context::swapBuffers()
{
	eglSwapBuffers(display.getDisplayPtr(),surface.get());
}

Context::Context(EGLNativeDisplayType nativeDisplay, EGLNativeWindowType nativeWindow)
	: context(nullptr), surface(nullptr), display(nativeDisplay)
{
	if (!eglChooseConfig(display.getDisplayPtr(), attr, &config, 1, &num_config)) {
		throw EglError(CONFIG);
	}
	auto my_destroyContext = [this](context_t* t) { eglDestroyContext(display.getDisplayPtr(), t); };
	auto my_destroySurface = [this](surface_t* t) { eglDestroySurface(display.getDisplayPtr(), t); };
	this->surface = uSurface(eglCreateWindowSurface(display.getDisplayPtr(), config, nativeWindow, nullptr),my_destroySurface);
	if(!this->surface)
	{
		throw EglError(SURFACE);
	}
	this->context = uContext(eglCreateContext(display.getDisplayPtr(), config, nullptr, ctxattr),my_destroyContext);
	if(!this->context)
	{
		throw EglError(CONTEXT);
	}
}

void Context::makeCurrent()
{
	eglMakeCurrent(display.getDisplayPtr(), surface.get(), surface.get(), context.get());
}

const Context::surface_t* Context::getSurfacePtr() const
{
	return surface.get();
}

const Context::context_t* Context::getContextPtr() const
{
	return context.get();
}

Context::surface_t* Context::getSurfacePtr()
{
	return surface.get();
}

Context::context_t* Context::getContextPtr()
{
	return context.get();
}

}
