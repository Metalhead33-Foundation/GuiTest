#if defined(DESKTOP_GL) && (WINSYS_X)
#include "GlxContext.hpp"

namespace GLX {

// Get a matching FB config
static const int visualAttribs[] = {
	GLX_X_RENDERABLE, True,
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_RENDER_TYPE, GLX_RGBA_BIT,
	GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
	/*GLX_RED_SIZE, 8,
	GLX_GREEN_SIZE, 8,
	GLX_BLUE_SIZE, 8,
	GLX_ALPHA_SIZE, 8,*/
	GLX_DEPTH_SIZE, 24,
	GLX_STENCIL_SIZE, 8,
	GLX_DOUBLEBUFFER, True,
	None
};

static const int contextAttribs[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
	GLX_CONTEXT_MINOR_VERSION_ARB, 1,
	GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
	GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
	None
};


Context::Context(const SDL_SysWMinfo& syswminfo, int screen)
	: version(0), display(syswminfo.info.x11.display), win(syswminfo.info.x11.window), fbc(nullptr), fbcount(0), ctx(nullptr)
{
	version = gladLoaderLoadGLX(display,screen);
	if(!version) {
		throw std::runtime_error("Failed to load OpenGL!");
	}
	fbc = glXChooseFBConfig(display, screen ? screen : DefaultScreen(syswminfo.info.x11.display), visualAttribs, &fbcount);
	ctx = glXCreateContextAttribsARB(display,fbc[0],nullptr,True,contextAttribs);
	if(!ctx) {
		throw std::runtime_error("Failed to create OpenGL context!");
	}
}

Context::~Context()
{
	if(ctx) glXDestroyContext(display, ctx);
	if(version) gladLoaderUnloadGLX();
}

void Context::makeCurrent()
{
	glXMakeCurrent( display, win, ctx );
}

void Context::swapBuffers()
{
	glXSwapBuffers ( display, win );
}

RenderingContext::RenderingContext(const SDL_SysWMinfo& syswminfo, int screen)
	: context(syswminfo,screen)
{
	context.makeCurrent();
	int version = gladLoaderLoadGL();
	if(!version) {
		throw std::runtime_error("Failed to load OpenGL!");
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

RenderingContext::~RenderingContext()
{
	gladLoaderUnloadGL();
}

Context& RenderingContext::getContext()
{
	return context;
}

}
#endif

