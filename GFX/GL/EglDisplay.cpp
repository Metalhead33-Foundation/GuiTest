#include "EglDisplay.hpp"
#include <stdexcept>

namespace EGL {

Display::Display(EGLNativeDisplayType nativeDisplay)
	: display(eglGetDisplay(nativeDisplay) ,eglTerminate)
{
	if(!display) throw std::runtime_error("Unable to get display!");
	gladLoaderLoadEGL(display.get());
	if(!eglInitialize(display.get(),&majorVersion,&minorVersion)) throw std::runtime_error("Unable to initialize display!");
}

Display::Display(uDisplay&& mov)
	: display(std::move(mov))
{
	if(!eglInitialize(this->display.get(),&majorVersion,&minorVersion)) throw std::runtime_error("Unable to initialize display!");
}

EGLint Display::getMajorVersion() const
{
	return majorVersion;
}

EGLint Display::getMinorVersion() const
{
	return minorVersion;
}

Display::Display(Display&& mov)
	: display(std::move(mov.display)), majorVersion(mov.majorVersion), minorVersion(mov.minorVersion)
{
	mov.majorVersion = 0;
	mov.minorVersion = 0;
}

Display& Display::operator=(Display&& mov)
{
	this->display = std::move(mov.display);
	this->majorVersion = mov.majorVersion;
	this->minorVersion = mov.minorVersion;
	mov.majorVersion = 0;
	mov.minorVersion = 0;
	return *this;
}

const Display::display_t* Display::getDisplayPtr() const
{
	return display.get();
}

Display::display_t* Display::getDisplayPtr()
{
	return display.get();
}

}
