#ifndef EGLCONTEXT_HPP
#define EGLCONTEXT_HPP
#include <EGL/EglDisplay.hpp>
#include <functional>
namespace EGL {

class Context
{
public:
	typedef std::pointer_traits<EGLSurface> SurfaceTraits;
	typedef SurfaceTraits::element_type surface_t;
	typedef std::unique_ptr<surface_t,std::function<void(surface_t*)>> uSurface;
	typedef std::pointer_traits<EGLContext> ContextTraits;
	typedef ContextTraits::element_type context_t;
	typedef std::unique_ptr<context_t,std::function<void(context_t*)>> uContext;
private:
	EGLConfig config;
	EGLint num_config;
	uContext context;
	uSurface surface;
	Display display;
public:
	Context(EGLNativeDisplayType nativeDisplay, EGLNativeWindowType nativeWindow);
	void makeCurrent();
	const surface_t* getSurfacePtr() const;
	const context_t* getContextPtr() const;
	surface_t* getSurfacePtr();
	context_t* getContextPtr();
	EGLConfig getConfig() const;
	const Display& getDisplay() const;
	Display& getDisplay();
	void swapBuffers();
};

}

#endif // EGLCONTEXT_HPP
