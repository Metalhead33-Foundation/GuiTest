/**
 * @file EglContext.cpp
 * @brief Implements EglContext.hpp.
 *
 * The attribute lists are built from @ref ContextConfig, so the only hard-coded
 * policy left is "a window surface with a renderable type for this dialect". The
 * legacy implementation had a fixed `EGL_BUFFER_SIZE, 16` plus `EGL_NONE`, which
 * ignored every caller request and rejected RGBA8 windows on some drivers.
 */
#include "EglContext.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace KALDIGL_NS {

namespace {

/** @brief Builds the exception message for a failed EGL call. */
[[noreturn]] void throwEglFailure(const char* step, EGLint error)
{
	std::string message = "EglContext: ";
	message += step;
	message += " failed (";
	message += eglErrorName(error);
	message += ")";
	throw std::runtime_error(message);
}

/** @brief Appends a size attribute unless the caller disabled that buffer. */
void appendSize(std::vector<EGLint>& attribs, EGLint name, int bits)
{
	if (bits > 0) {
		attribs.push_back(name);
		attribs.push_back(static_cast<EGLint>(bits));
	}
}

/** @brief Builds the `eglChooseConfig` attribute list for @p config. */
std::vector<EGLint> buildConfigAttributes(const ContextConfig& config)
{
	std::vector<EGLint> attribs;
	attribs.push_back(EGL_SURFACE_TYPE);
	attribs.push_back(EGL_WINDOW_BIT);
	attribs.push_back(EGL_RENDERABLE_TYPE);
#if KALDIGL_IS_GLES
	attribs.push_back(EGL_OPENGL_ES3_BIT);
#else
	attribs.push_back(EGL_OPENGL_BIT);
#endif
	appendSize(attribs, EGL_RED_SIZE, config.redBits);
	appendSize(attribs, EGL_GREEN_SIZE, config.greenBits);
	appendSize(attribs, EGL_BLUE_SIZE, config.blueBits);
	appendSize(attribs, EGL_ALPHA_SIZE, config.alphaBits);
	appendSize(attribs, EGL_DEPTH_SIZE, config.depthBits);
	appendSize(attribs, EGL_STENCIL_SIZE, config.stencilBits);
	// EGL has no double-buffer config attribute: window surfaces are always
	// back-buffered, EGL_RENDER_BUFFER is a per-surface attribute instead.
	if (wantsMultisampling(config)) {
		attribs.push_back(EGL_SAMPLE_BUFFERS);
		attribs.push_back(1);
		attribs.push_back(EGL_SAMPLES);
		attribs.push_back(static_cast<EGLint>(config.sampleCount));
	}
	attribs.push_back(EGL_NONE);
	return attribs;
}

/**
 * @brief Builds the `eglCreateContext` attribute list for @p config.
 * @param useProfiles True when the display understands EGL 1.5 /
 *        `EGL_KHR_create_context` version and profile attributes.
 */
std::vector<EGLint> buildContextAttributes(const ContextConfig& config, bool useProfiles)
{
	std::vector<EGLint> attribs;
	if (!useProfiles) {
		// Oldest portable form: only the client (major) version can be requested.
		attribs.push_back(EGL_CONTEXT_CLIENT_VERSION);
		attribs.push_back(static_cast<EGLint>(config.versionMajor));
		attribs.push_back(EGL_NONE);
		return attribs;
	}

	attribs.push_back(EGL_CONTEXT_MAJOR_VERSION);
	attribs.push_back(static_cast<EGLint>(config.versionMajor));
	attribs.push_back(EGL_CONTEXT_MINOR_VERSION);
	attribs.push_back(static_cast<EGLint>(config.versionMinor));

	EGLint flags = 0;
#if !KALDIGL_IS_GLES
	if (config.coreProfile) {
		attribs.push_back(EGL_CONTEXT_OPENGL_PROFILE_MASK);
		attribs.push_back(EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT);
	}
	if (config.forwardCompatible) {
		flags |= EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;
	}
#endif
	if (config.debugContext) {
		flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
	}
	if (flags != 0) {
		attribs.push_back(EGL_CONTEXT_FLAGS_KHR);
		attribs.push_back(flags);
	}
	attribs.push_back(EGL_NONE);
	return attribs;
}

} // namespace

EglContext::EglContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config)
	: EglContext(EglDisplay::acquire(reinterpret_cast<EGLNativeDisplayType>(handles.display)),
				 reinterpret_cast<EGLNativeWindowType>(handles.eglDrawable()),
				 config)
{
}

EglContext::EglContext(std::shared_ptr<EglDisplay> display, EGLNativeWindowType window, const ContextConfig& config)
	: display_(std::move(display))
	, config_(config)
{
	if (display_ == nullptr || !display_->isValid()) {
		throw std::invalid_argument("EglContext: the EGL display is not initialized");
	}
	if (!window) {
		throw std::invalid_argument("EglContext: no native window was supplied");
	}

	const EGLDisplay dpy = display_->handle();

	// EGL starts with the OpenGL ES client API bound. Desktop contexts have to
	// bind EGL_OPENGL_API before eglCreateContext, otherwise the GL-specific
	// context attributes (profile mask, forward-compatible flag) are rejected with
	// EGL_BAD_ATTRIBUTE even when the config came from EGL_OPENGL_BIT.
	if (eglBindAPI == nullptr) {
		throw std::runtime_error("EglContext: eglBindAPI is unavailable (EGL 1.2 is required)");
	}
#if KALDIGL_IS_GLES
	if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
#else
	if (eglBindAPI(EGL_OPENGL_API) != EGL_TRUE) {
#endif
		throwEglFailure("eglBindAPI", eglGetError());
	}

	const std::vector<EGLint> configAttribs = buildConfigAttributes(config_);
	EGLint configCount = 0;
	if (!eglChooseConfig(dpy, configAttribs.data(), &eglConfig_, 1, &configCount) || configCount < 1) {
		throwEglFailure("eglChooseConfig", eglGetError());
	}

	surface_ = eglCreateWindowSurface(dpy, eglConfig_, window, nullptr);
	if (surface_ == EGL_NO_SURFACE) {
		throwEglFailure("eglCreateWindowSurface", eglGetError());
	}

	const std::vector<EGLint> contextAttribs = buildContextAttributes(config_, display_->supportsContextProfiles());
	context_ = eglCreateContext(dpy, eglConfig_, EGL_NO_CONTEXT, contextAttribs.data());
	if (context_ == EGL_NO_CONTEXT) {
		const EGLint error = eglGetError();
		eglDestroySurface(dpy, surface_);
		surface_ = EGL_NO_SURFACE;
		throwEglFailure("eglCreateContext", error);
	}
}

EglContext::~EglContext()
{
	if (display_ == nullptr || !display_->isValid()) {
		return;
	}
	const EGLDisplay dpy = display_->handle();
	// eglGetCurrentContext is an EGL 1.4 entry point. A loader that could not
	// detect the display version leaves it null, and calling it here would turn
	// destruction into a crash; the context is then simply left unbound.
	if (eglGetCurrentContext != nullptr && eglGetCurrentContext() == context_) {
		eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	}
	if (surface_ != EGL_NO_SURFACE) {
		eglDestroySurface(dpy, surface_);
		surface_ = EGL_NO_SURFACE;
	}
	if (context_ != EGL_NO_CONTEXT) {
		eglDestroyContext(dpy, context_);
		context_ = EGL_NO_CONTEXT;
	}
}

void EglContext::makeCurrent()
{
	if (eglMakeCurrent(display_->handle(), surface_, surface_, context_) != EGL_TRUE) {
		throwEglFailure("eglMakeCurrent", eglGetError());
	}
}

void EglContext::releaseCurrent() noexcept
{
	if (display_ == nullptr || !display_->isValid()) {
		return;
	}
	// Unbinding cannot be recovered from usefully, and this is called from destructors.
	(void)eglMakeCurrent(display_->handle(), EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void EglContext::swapBuffers()
{
	if (eglSwapBuffers(display_->handle(), surface_) != EGL_TRUE) {
		throwEglFailure("eglSwapBuffers", eglGetError());
	}
}

std::unique_ptr<Context> createEglContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config)
{
	return std::make_unique<EglContext>(handles, config);
}

}
