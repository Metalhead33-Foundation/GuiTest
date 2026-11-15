#ifndef KALDIGLEGLCONTEXT_HPP
#define KALDIGLEGLCONTEXT_HPP
/**
 * @file EglContext.hpp
 * @brief EGL implementation of the portable @ref Context contract.
 *
 * Used by both GL-family backends: it is the only option for OpenGL ES, and it is
 * the portable option for desktop OpenGL (Wayland, Windows via ANGLE, Android,
 * embedded targets). Desktop-only X11 users may prefer `GlxContext`.
 *
 * The display is shared through @ref EglDisplay, so several windows of the same
 * process reuse one `EGLDisplay` instead of initializing it twice.
 */
#include "EglDisplay.hpp"
#include "GlContext.hpp"
#include <memory>

namespace KALDIGL_NS {

/**
 * @brief An EGL context plus its window surface.
 *
 * @note Not copyable and not movable: the destructor must run on a thread that
 * may legally release the context, and moving a live GL context between objects
 * buys nothing. Hold it through `std::unique_ptr<Context>`.
 */
class EglContext final : public Context {
public:
	/**
	 * @brief Creates a context and a window surface for @p handles.
	 * @param handles Native display/window, normally from nativeHandlesFrom().
	 * @param config Requested version, profile, buffer sizes and MSAA.
	 * @throws std::invalid_argument When @p handles has no native window.
	 * @throws std::runtime_error When EGL fails; the message names the failing call.
	 */
	EglContext(const NativeWindowHandles& handles, const ContextConfig& config);

	/**
	 * @brief Creates a context and a window surface on an existing display.
	 * @param display Initialized display, normally from `EglDisplay::acquire()`.
	 * @param window Platform window (`EGLNativeWindowType`).
	 * @param config Requested version, profile, buffer sizes and MSAA.
	 */
	EglContext(std::shared_ptr<EglDisplay> display, EGLNativeWindowType window, const ContextConfig& config);

	/** @brief Releases the surface and the context (and unbinds them when current). */
	~EglContext() override;

	EglContext(const EglContext&) = delete;
	EglContext& operator=(const EglContext&) = delete;
	EglContext(EglContext&&) = delete;
	EglContext& operator=(EglContext&&) = delete;

	/** @brief `eglMakeCurrent(display, surface, surface, context)`; throws on failure. */
	void makeCurrent() override;

	/** @brief Unbinds the context from the calling thread (best effort, never throws). */
	void releaseCurrent() noexcept override;

	/** @brief `eglSwapBuffers`; throws on failure. Requires a current context. */
	void swapBuffers() override;

	/** @brief Always `ContextApi::EGL`. */
	ContextApi api() const noexcept override { return ContextApi::EGL; }
	/** @brief Always "EGL". */
	std::string_view apiName() const noexcept override { return contextApiName(ContextApi::EGL); }
	/** @brief The configuration this context was created with. */
	const ContextConfig& config() const noexcept override { return config_; }
	/** @brief True when both the context and the surface exist. */
	bool isValid() const noexcept override { return context_ != EGL_NO_CONTEXT && surface_ != EGL_NO_SURFACE; }

	/** @brief The raw `EGLContext`. */
	EGLContext handle() const noexcept { return context_; }
	/** @brief The raw `EGLSurface` of the window. */
	EGLSurface surface() const noexcept { return surface_; }
	/** @brief The `EGLConfig` chosen for this context. */
	EGLConfig eglConfig() const noexcept { return eglConfig_; }
	/** @brief The display this context lives on. */
	const EglDisplay& display() const noexcept { return *display_; }

private:
	std::shared_ptr<EglDisplay> display_;
	EGLConfig eglConfig_ = nullptr;
	EGLContext context_ = EGL_NO_CONTEXT;
	EGLSurface surface_ = EGL_NO_SURFACE;
	ContextConfig config_;
};

}

#endif // KALDIGLEGLCONTEXT_HPP
