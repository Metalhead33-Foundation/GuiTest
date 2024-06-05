#ifndef EGLCONTEXT_HPP
#define EGLCONTEXT_HPP
#include <GFX/GL/EglDisplay.hpp>
#include <functional>
#include <GFX/GL/GlContext.hpp>
#include <GFX/GL/GlResourceFactory.hpp>
namespace EGL {
/*! An object that encapsulates an EGL context. */
class Context : public GL::Context
{
public:
	/*! Traits of the EGLSurface pointer.*/
	typedef std::pointer_traits<EGLSurface> SurfaceTraits;
	/*! EGL surface type.*/
	typedef SurfaceTraits::element_type surface_t;
	/*! Unique pointer to an EGL surface.*/
	typedef std::unique_ptr<surface_t,std::function<void(surface_t*)>> uSurface;
	/*! Traits of the EGLContext pointer.*/
	typedef std::pointer_traits<EGLContext> ContextTraits;
	/*! EGL context type.*/
	typedef ContextTraits::element_type context_t;
	/*! Unique pointer to an EGL context.*/
	typedef std::unique_ptr<context_t,std::function<void(context_t*)>> uContext;
private:
	/*! EGL configuration structure. */
	EGLConfig config;
	/*! Number of configurations */
	EGLint num_config;
	/*! Pointer to the EGL context itself. */
	uContext context;
	/*! Pointer to the EGL surface. */
	uSurface surface;
	/*! Pointer to the EGL display. */
	Display display;
public:
	//! Constructor.
	/*!
	  \param nativeDisplay A pointer to the native display.
	  \param nativeWindow A pointer to the native window.
	*/
	Context(EGLNativeDisplayType nativeDisplay, EGLNativeWindowType nativeWindow);
	/*! Makes the context the current context for the thread it is called for, for use with OpenGL(ES) functions. */
	void makeCurrent();
	//! Returns a pointer to the EGL surface associated with the context, for use in functions that require the raw pointer.
	/*!
	  \return The pointer to the EGL surface associated with the context.
	*/
	const surface_t* getSurfacePtr() const;
	//! Returns a pointer to the EGL context, for use in functions that require the raw pointer.
	/*!
	  \return The pointer to the EGL context.
	*/
	const context_t* getContextPtr() const;
	//! Returns a pointer to the EGL surface associated with the context, for use in functions that require the raw pointer.
	/*!
	  \return The pointer to the EGL surface associated with the context.
	*/
	surface_t* getSurfacePtr();
	//! Returns a pointer to the EGL context, for use in functions that require the raw pointer.
	/*!
	  \return The pointer to the EGL context.
	*/
	context_t* getContextPtr();
	//! Returns a pointer to the EGL configuration associated with the context, for use in functions that require the raw pointer.
	/*!
	  \return The pointer to the EGL configuration.
	*/
	EGLConfig getConfig() const;
	//! Returns a reference to the EGL display associated with the context.
	/*!
	  \return A reference to the EGL display.
	*/
	const Display& getDisplay() const;
	//! Returns a reference to the EGL display associated with the context.
	/*!
	  \return A reference to the EGL display.
	*/
	Display& getDisplay();
	/*! Swaps buffers. To be called every time OpenGL(ES) has finished rendering a frame. */
	void swapBuffers();
};
class RenderingContext : public GL::ResourceFactory {
private:
	Context context;
public:
	RenderingContext(const SDL_SysWMinfo& syswminfo);
	~RenderingContext();
	Context& getContext() override;
};

}

#endif // EGLCONTEXT_HPP
