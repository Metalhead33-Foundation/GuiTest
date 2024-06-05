#if defined(DESKTOP_GL) && (WINSYS_X)
#include "GlxContext.hpp"

namespace GLX {

// Get a matching FB config
static int visual_attribs[] =
  {
	GLX_X_RENDERABLE    , True,
	GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
	GLX_RENDER_TYPE     , GLX_RGBA_BIT,
	GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
	GLX_RED_SIZE        , 8,
	GLX_GREEN_SIZE      , 8,
	GLX_BLUE_SIZE       , 8,
	GLX_ALPHA_SIZE      , 8,
	GLX_DEPTH_SIZE      , 24,
	GLX_STENCIL_SIZE    , 8,
	GLX_DOUBLEBUFFER    , True,
	//GLX_SAMPLE_BUFFERS  , 1,
	//GLX_SAMPLES         , 4,
	None
  };

static int context_attribs[] =
{
  GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
  GLX_CONTEXT_MINOR_VERSION_ARB, 3,
  GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
  None
};

const GLXFBConfig& Context::getBestFbc() const
{
	// Pick the FB config/visual with the most samples per pixel
	printf( "Getting XVisualInfos\n" );
	int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

	int i;
	for (i=0; i<fbcount; ++i)
	{
	  XVisualInfo *vi = glXGetVisualFromFBConfig( display, fbc[i] );
	  if ( vi )
	  {
		int samp_buf, samples;
		glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
		glXGetFBConfigAttrib( display, fbc[i], GLX_SAMPLES       , &samples  );

		if ( best_fbc < 0 || samp_buf && samples > best_num_samp )
		  best_fbc = i, best_num_samp = samples;
		if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
		  worst_fbc = i, worst_num_samp = samples;
	  }
	  XFree( vi );
	}
	return fbc[ best_fbc ];
}

Context::Context(const SDL_SysWMinfo& syswminfo, int screen)
	: version(0), display(syswminfo.info.x11.display), win(syswminfo.info.x11.window), fbc(nullptr), fbcount(0), ctx(nullptr)
{
	version = gladLoaderLoadGLX(display,screen);
	if(!version) {
		throw std::runtime_error("Failed to load OpenGL!");
	}
	fbc = glXChooseFBConfig(display, screen, visual_attribs, &fbcount);
	// Pick the FB config/visual with the most samples per pixel
	ctx = glXCreateContextAttribsARB(display,getBestFbc(),nullptr,True,context_attribs);
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

