#ifndef EGLDISPLAY_HPP
#define EGLDISPLAY_HPP
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <SDL2/SDL_syswm.h>
#include <memory>
namespace EGL {

class Display
{
public:
	typedef std::pointer_traits<EGLDisplay> DisplayTraits;
	typedef DisplayTraits::element_type display_t;
	typedef std::unique_ptr<display_t,decltype(eglTerminate)> uDisplay;
private:
	uDisplay display;
	EGLint majorVersion;
	EGLint minorVersion;
	Display(const Display& cpy) = delete;
	Display& operator=(const Display& cpy) = delete;
public:
	explicit Display(EGLNativeDisplayType nativeDisplay);
	explicit Display(uDisplay&& mov);
	Display(Display&& mov);
	Display& operator=(Display&& mov);
	const display_t* getDisplayPtr() const;
	display_t* getDisplayPtr();
	EGLint getMajorVersion() const;
	EGLint getMinorVersion() const;
};
typedef std::shared_ptr<Display> sDisplay;

}

#endif // EGLDISPLAY_HPP
