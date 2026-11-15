/**
 * @file GlxContext.cpp
 * @brief Implements GlxContext.hpp.
 */
#include "GlxContext.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace KALDIGL_NS {

namespace {

/** @brief Appends a visual attribute unless the caller disabled that buffer. */
void appendSize(std::vector<int>& attribs, int name, int bits)
{
	if (bits > 0) {
		attribs.push_back(name);
		attribs.push_back(bits);
	}
}

/** @brief Asks GLX for the first framebuffer configuration matching @p attribs. */
GLXFBConfig chooseFbConfig(::Display* display, int screen, const std::vector<int>& attribs)
{
	int count = 0;
	GLXFBConfig* configs = glXChooseFBConfig(display, screen, attribs.data(), &count);
	GLXFBConfig chosen = nullptr;
	if (configs != nullptr && count > 0) {
		chosen = configs[0];
	}
	if (configs != nullptr) {
		XFree(configs);
	}
	return chosen;
}

/** @brief Builds the detailed visual attribute list from @p config. */
std::vector<int> detailedVisualAttributes(const ContextConfig& config)
{
	std::vector<int> attribs = {
		GLX_X_RENDERABLE, True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR
	};
	appendSize(attribs, GLX_RED_SIZE, config.redBits);
	appendSize(attribs, GLX_GREEN_SIZE, config.greenBits);
	appendSize(attribs, GLX_BLUE_SIZE, config.blueBits);
	appendSize(attribs, GLX_ALPHA_SIZE, config.alphaBits);
	appendSize(attribs, GLX_DEPTH_SIZE, config.depthBits);
	appendSize(attribs, GLX_STENCIL_SIZE, config.stencilBits);
	attribs.push_back(GLX_DOUBLEBUFFER);
	attribs.push_back(config.doubleBuffered ? True : False);
	attribs.push_back(None);
	return attribs;
}

/**
 * @brief The minimal visual attribute list used as a fallback.
 *
 * Several drivers refuse an exact RGBA8 request for the default visual, so a
 * failed detailed query is retried with only the properties GLX requires.
 */
std::vector<int> minimalVisualAttributes()
{
	return { GLX_X_RENDERABLE, True,
			 GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
			 GLX_RENDER_TYPE, GLX_RGBA_BIT,
			 GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
			 None };
}

/** @brief Builds the `glXCreateContextAttribsARB` attribute list for @p config. */
std::vector<int> contextAttributes(const ContextConfig& config)
{
	std::vector<int> attribs = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, config.versionMajor,
		GLX_CONTEXT_MINOR_VERSION_ARB, config.versionMinor
	};
	if (config.coreProfile) {
		attribs.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
		attribs.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
	}
	int flags = 0;
	if (config.forwardCompatible) {
		flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
	}
	if (config.debugContext) {
		flags |= GLX_CONTEXT_DEBUG_BIT_ARB;
	}
	if (flags != 0) {
		attribs.push_back(GLX_CONTEXT_FLAGS_ARB);
		attribs.push_back(flags);
	}
	attribs.push_back(None);
	return attribs;
}

} // namespace

GlxContext::GlxContext(::Display* display, ::Window window, const ContextConfig& config)
	: config_(config)
	, display_(display)
	, window_(window)
{
	if (display_ == nullptr) {
		throw std::invalid_argument("GlxContext: no X11 display was supplied");
	}
	if (window_ == 0) {
		throw std::invalid_argument("GlxContext: no X11 window was supplied");
	}

	const int screen = config_.screen >= 0 ? config_.screen : DefaultScreen(display_);
	glxVersion_ = gladLoaderLoadGLX(display_, screen);
	if (glxVersion_ == 0) {
		throw std::runtime_error("GlxContext: GLX could not be loaded (is libGL installed?)");
	}

	fbConfig_ = chooseFbConfig(display_, screen, detailedVisualAttributes(config_));
	if (fbConfig_ == nullptr) {
		fbConfig_ = chooseFbConfig(display_, screen, minimalVisualAttributes());
	}
	if (fbConfig_ == nullptr) {
		throw std::runtime_error("GlxContext: glXChooseFBConfig found no usable framebuffer configuration");
	}

	if (glXCreateContextAttribsARB == nullptr) {
		throw std::runtime_error("GlxContext: GLX_ARB_create_context is not available on this driver");
	}
	const std::vector<int> attribs = contextAttributes(config_);
	context_ = glXCreateContextAttribsARB(display_, fbConfig_, nullptr, True, attribs.data());
	if (context_ == nullptr) {
		throw std::runtime_error("GlxContext: glXCreateContextAttribsARB failed for the requested version/profile");
	}
}

GlxContext::~GlxContext()
{
	if (display_ != nullptr && context_ != nullptr) {
		if (glXGetCurrentContext() == context_) {
			glXMakeCurrent(display_, None, nullptr);
		}
		glXDestroyContext(display_, context_);
		context_ = nullptr;
	}
	if (glxVersion_ != 0) {
		gladLoaderUnloadGLX();
		glxVersion_ = 0;
	}
}

void GlxContext::makeCurrent()
{
	if (glXMakeCurrent(display_, window_, context_) == False) {
		throw std::runtime_error("GlxContext: glXMakeCurrent failed");
	}
}

void GlxContext::releaseCurrent() noexcept
{
	glXMakeCurrent(display_, None, nullptr);
}

void GlxContext::swapBuffers()
{
	glXSwapBuffers(display_, window_);
}

std::unique_ptr<Context> createGlxContext(const NativeWindowHandles& handles, const ContextConfig& config)
{
	if (handles.display == nullptr || handles.window == nullptr) {
		throw std::invalid_argument("GlxContext: GLX requires an X11 display and window");
	}
	return std::make_unique<GlxContext>(reinterpret_cast<::Display*>(handles.display),
										static_cast<::Window>(reinterpret_cast<uintptr_t>(handles.window)),
										config);
}

}
