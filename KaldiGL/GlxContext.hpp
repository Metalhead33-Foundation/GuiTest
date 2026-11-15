#ifndef KALDIGLXCONTEXT_HPP
#define KALDIGLXCONTEXT_HPP
/**
 * @file GlxContext.hpp
 * @brief GLX implementation of the portable @ref Context contract (desktop OpenGL, X11).
 *
 * GLX is the native X11 binding for desktop OpenGL and is what
 * @ref NativeWindowHandles suggests for `SDL_SYSWM_X11` windows in the desktop
 * build. It is only compiled into `KaldiGL`; the OpenGL ES backend has no GLX
 * path at all, and Wayland/Windows/Android must use the EGL implementation.
 *
 * Unlike the legacy class this replaces, the framebuffer configuration is chosen
 * from @ref ContextConfig (channel sizes, depth/stencil, double buffering), the
 * `GLXFBConfig` array is freed with `XFree`, and every failure is reported with
 * a message that names the failing GLX call.
 */
#include "GlContext.hpp"
#include <glad/glx.h>

namespace KALDIGL_NS {

/**
 * @brief A GLX context for one X11 window.
 *
 * The context does not own the X11 display or the window: both belong to the
 * windowing layer (SDL) and must outlive this object.
 *
 * @note Not copyable and not movable, for the same reasons as `EglContext`.
 */
class GlxContext final : public Context {
public:
	/**
	 * @brief Creates a GLX context for @p window.
	 * @param display X11 display owning the window.
	 * @param window X11 window the drawable is bound to.
	 * @param config Requested version, profile, buffer sizes; @ref ContextConfig::screen
	 *        selects the X11 screen (-1 means the display's default screen).
	 * @throws std::invalid_argument When @p display or @p window is null.
	 * @throws std::runtime_error When libGL/GLX, the framebuffer configuration or
	 *         the context itself cannot be obtained.
	 */
	GlxContext(::Display* display, ::Window window, const ContextConfig& config);

	/** @brief Releases the context and the GLX entry points it loaded. */
	~GlxContext() override;

	GlxContext(const GlxContext&) = delete;
	GlxContext& operator=(const GlxContext&) = delete;
	GlxContext(GlxContext&&) = delete;
	GlxContext& operator=(GlxContext&&) = delete;

	/** @brief `glXMakeCurrent(display, window, context)`; throws on failure. */
	void makeCurrent() override;

	/** @brief Unbinds the context from the calling thread (best effort, never throws). */
	void releaseCurrent() noexcept override;

	/** @brief `glXSwapBuffers`; requires a current context. */
	void swapBuffers() override;

	/** @brief Always `ContextApi::GLX`. */
	ContextApi api() const noexcept override { return ContextApi::GLX; }
	/** @brief Always "GLX". */
	std::string_view apiName() const noexcept override { return contextApiName(ContextApi::GLX); }
	/** @brief The configuration this context was created with. */
	const ContextConfig& config() const noexcept override { return config_; }
	/** @brief True when a native GLX context exists. */
	bool isValid() const noexcept override { return context_ != nullptr; }

	/** @brief The raw `GLXContext`. */
	GLXContext handle() const noexcept { return context_; }
	/** @brief The `GLXFBConfig` the context was created from. */
	GLXFBConfig fbConfig() const noexcept { return fbConfig_; }
	/** @brief The X11 display (borrowed, not owned). */
	::Display* nativeDisplay() const noexcept { return display_; }
	/** @brief The X11 window (borrowed, not owned). */
	::Window nativeWindow() const noexcept { return window_; }

private:
	ContextConfig config_;
	::Display* display_ = nullptr;
	::Window window_ = 0;
	GLXFBConfig fbConfig_ = nullptr;
	GLXContext context_ = nullptr;
	int glxVersion_ = 0;
};

}

#endif // KALDIGLXCONTEXT_HPP
