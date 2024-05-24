#ifndef GLXCONTEXT_HPP
#define GLXCONTEXT_HPP
#if defined(DESKTOP_GL) && (WINSYS_X)
#include <NativeGfxApi/glad/glx.h>
#include <SDL2/SDL_syswm.h>
#include <GFX/GL/GlContext.hpp>

namespace GLX {
class Context : public GL::Context
{
private:
	int version;
	Display* display;
	Window win;
	GLXFBConfig* fbc;
	int fbcount;
	GLXContext ctx;
	const GLXFBConfig& getBestFbc() const;
public:
	Context(const SDL_SysWMinfo& syswminfo, int screen = 0);
	~Context();
	void makeCurrent();
	void swapBuffers();
};
}

#endif
#endif // GLXCONTEXT_HPP
