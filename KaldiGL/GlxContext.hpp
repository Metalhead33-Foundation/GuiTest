#ifndef GLXCONTEXT_HPP
#define GLXCONTEXT_HPP
#include <glad/glx.h>
#include <SDL2/SDL_syswm.h>
#include <GlContext.hpp>

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
public:
	Context(const SDL_SysWMinfo& syswminfo, int screen = 0);
	~Context();
	void makeCurrent();
	void swapBuffers();
};

class RenderingContext : public Context {
public:
	RenderingContext(const SDL_SysWMinfo& syswminfo, int screen = 0);
	~RenderingContext();
};

}

#endif // GLXCONTEXT_HPP
