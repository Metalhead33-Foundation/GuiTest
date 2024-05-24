#ifndef EGLDISPLAY_HPP
#define EGLDISPLAY_HPP
#include <NativeGfxApi/glad/egl.h>
#include <NativeGfxApi/EGL//eglplatform.h>
#include <SDL2/SDL_syswm.h>
#include <memory>
namespace EGL {

/*! An object that encapsulates an EGL display. */
class Display
{
public:
	/*! Traits of the EGLDisplay pointer.*/
	typedef std::pointer_traits<EGLDisplay> DisplayTraits;
	/*! EGL display type.*/
	typedef DisplayTraits::element_type display_t;
	/*! Unique pointer to an EGL display.*/
	typedef std::unique_ptr<display_t,decltype(eglTerminate)> uDisplay;
private:
	/*! A pointer to the underlying raw EGL display. */
	uDisplay display;
	/*! EGL major version. (e.g. 1 for 1.3) */
	EGLint majorVersion;
	/*! EGL minor version. (e.g. 3 for 1.3) */
	EGLint minorVersion;
	Display(const Display& cpy) = delete;
	Display& operator=(const Display& cpy) = delete;
public:
	//! Constructor.
	/*!
	  \param nativeDisplay A pointer to the native display.
	*/
	explicit Display(EGLNativeDisplayType nativeDisplay);
	//! Constructor.
	/*!
	  \param mov A unique pointer to the native display.
	*/
	explicit Display(uDisplay&& mov);
	//! Move-constructor.
	/*!
	  \param mov An RValue reference to an instance of Dispaly to be moved.
	*/
	Display(Display&& mov);
	//! Move-assignment operator.
	/*!
	  \param mov An RValue reference to an instance of Dispaly to be moved.
	  \return The Display that was assigned to.
	*/
	Display& operator=(Display&& mov);
	//! Returns a pointer to the EGL display, for use in functions that require the raw pointer.
	/*!
	  \return The pointer to the EGL display.
	*/
	const display_t* getDisplayPtr() const;
	//! Returns a pointer to the EGL display, for use in functions that require the raw pointer.
	/*!
	  \return The pointer to the EGL display.
	*/
	display_t* getDisplayPtr();
	//! Returns the EGL major version. (e.g. 1 for 1.3)
	/*!
	  \return EGL major version (e.g. 1 for 1.3)
	*/
	EGLint getMajorVersion() const;
	//! Returns the EGL minor version. (e.g. 3 for 1.3)
	/*!
	  \return EGL minor version (e.g. 3 for 1.3)
	*/
	EGLint getMinorVersion() const;
};
/*! A shared pointer to an object that encapsulates an EGL display. */
typedef std::shared_ptr<Display> sDisplay;

}

#endif // EGLDISPLAY_HPP
