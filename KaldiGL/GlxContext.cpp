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

/** @brief Visual id of @p window, or @c 0 when it cannot be queried. */
VisualID windowVisualId(::Display* display, ::Window window)
{
	XWindowAttributes attributes {};
	if (XGetWindowAttributes(display, window, &attributes) == 0 || attributes.visual == nullptr) {
		return 0;
	}
	return XVisualIDFromVisual(attributes.visual);
}

/** @brief Reads one integer attribute of @p config, or -1 when the query fails. */
int fbConfigAttribute(::Display* display, GLXFBConfig config, int attribute)
{
	int value = 0;
	return glXGetFBConfigAttrib(display, config, attribute, &value) == Success ? value : -1;
}

/**
 * @brief Picks the framebuffer configuration that belongs to the window's visual.
 *
 * A GLX drawable must use the same visual as its FBConfig: `glXMakeCurrent`
 * rejects anything else with an X `BadMatch`, and Xlib's default error handler
 * aborts the process on that error. A caller-owned window (SDL creates its window
 * from its own GL setup) therefore selects the configuration, not the other way
 * round. `glXChooseFBConfig` is not usable for this: it filters by the requested
 * channel/depth/stencil sizes, and the window's visual is a single fixed layout
 * that rarely satisfies a generic request (a window without a stencil buffer has
 * no matching configuration then, even though GLX can render into it).
 *
 * `ContextConfig` buffer sizes and MSAA are therefore advisory for GLX: the
 * window's visual already fixes the framebuffer layout. @p wantedSamples is only
 * used to prefer a matching configuration with at least that many samples.
 *
 * @return A window-capable RGBA configuration with the window's visual, or
 *         @c nullptr when the X server offers none (the window is then not a GLX
 *         drawable and the caller gets a clear error instead of an X abort).
 */
GLXFBConfig chooseFbConfig(::Display* display, int screen, ::Window window, int wantedSamples)
{
	const VisualID visual = windowVisualId(display, window);
	if (visual == 0) {
		return nullptr;
	}
	int count = 0;
	GLXFBConfig* configs = glXGetFBConfigs(display, screen, &count);
	GLXFBConfig chosen = nullptr;
	GLXFBConfig fallback = nullptr;
	for (int index = 0; index < count; ++index) {
		const GLXFBConfig config = configs[index];
		if (static_cast<VisualID>(fbConfigAttribute(display, config, GLX_VISUAL_ID)) != visual) {
			continue;
		}
		const int drawableType = fbConfigAttribute(display, config, GLX_DRAWABLE_TYPE);
		const int renderType = fbConfigAttribute(display, config, GLX_RENDER_TYPE);
		if ((drawableType & GLX_WINDOW_BIT) == 0 || (renderType & GLX_RGBA_BIT) == 0) {
			continue;
		}
		if (fallback == nullptr) {
			fallback = config;
		}
		if (wantedSamples <= 1 || fbConfigAttribute(display, config, GLX_SAMPLES) >= wantedSamples) {
			chosen = config;
			break;
		}
	}
	if (configs != nullptr) {
		XFree(configs);
	}
	return chosen != nullptr ? chosen : fallback;
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

	fbConfig_ = chooseFbConfig(display_, screen, window_, wantsMultisampling(config_) ? config_.sampleCount : 0);
	if (fbConfig_ == nullptr) {
		throw std::runtime_error("GlxContext: the window's visual has no window-capable GLX framebuffer configuration");
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

std::unique_ptr<Context> createGlxContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config)
{
	if (handles.system != Kld::WindowSystem::X11 || handles.display == nullptr || handles.window == nullptr) {
		throw std::invalid_argument(std::string("GlxContext: GLX requires an X11 display and window, got ") +
									std::string(Kld::windowSystemName(handles.system)));
	}
	return std::make_unique<GlxContext>(reinterpret_cast<::Display*>(handles.display),
										static_cast<::Window>(reinterpret_cast<uintptr_t>(handles.window)),
										config);
}

}
