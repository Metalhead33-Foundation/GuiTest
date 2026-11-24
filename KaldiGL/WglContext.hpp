#ifndef KALDIGLWCONTEXT_HPP
#define KALDIGLWCONTEXT_HPP
/**
 * @file WglContext.hpp
 * @brief WGL implementation of the portable `Context` contract (desktop OpenGL, Win32).
 *
 * WGL is the native Win32 binding for desktop OpenGL and is what
 * `createContext()` picks for `Kld::WindowSystem::Windows` in the desktop
 * build. It is only compiled into `KaldiGL`, and only on Windows hosts (`win32`
 * scope of `KaldiGL.pro`), so it cannot be verified on the Linux CI host - it is
 * written against the documented WGL/WGL_ARB_create_context API.
 *
 * @note Unlike GLX, WGL does not own a "display": the pixel format belongs to the
 * window's device context, so this class obtains (and releases) an `HDC` for the
 * `HWND` and keeps the chosen pixel format index for diagnostics.
 */
#include "GlContext.hpp"
#include <glad/wgl.h>

namespace KALDIGL_NS {

/**
 * @brief A WGL context for one Win32 window.
 *
 * The context does not own the window: it belongs to the windowing layer (SDL)
 * and must outlive this object.
 */
class WglContext final : public Context {
public:
	/**
	 * @brief Creates a WGL context for @p window.
	 * @param window Window to render into; a device context is obtained from it.
	 * @param config Requested version, profile and buffer sizes.
	 * @throws std::invalid_argument When @p window is null.
	 * @throws std::runtime_error When the pixel format, WGL extensions or the
	 *         context itself cannot be obtained.
	 */
	WglContext(HWND window, const ContextConfig& config);

	/** @brief Releases the context, the device context and the WGL entry points. */
	~WglContext() override;

	WglContext(const WglContext&) = delete;
	WglContext& operator=(const WglContext&) = delete;
	WglContext(WglContext&&) = delete;
	WglContext& operator=(WglContext&&) = delete;

	/** @brief `wglMakeCurrent(dc, context)`; throws on failure. */
	void makeCurrent() override;

	/** @brief Unbinds the context from the calling thread (best effort, never throws). */
	void releaseCurrent() noexcept override;

	/** @brief `SwapBuffers(dc)`; requires a current context. */
	void swapBuffers() override;

	/** @brief Always `ContextApi::WGL`. */
	ContextApi api() const noexcept override { return ContextApi::WGL; }
	/** @brief Always "WGL". */
	std::string_view apiName() const noexcept override { return contextApiName(ContextApi::WGL); }
	/** @brief The configuration this context was created with. */
	const ContextConfig& config() const noexcept override { return config_; }
	/** @brief True when a native WGL context exists. */
	bool isValid() const noexcept override { return context_ != nullptr; }

	/** @brief The raw `HGLRC`. */
	HGLRC handle() const noexcept { return context_; }
	/** @brief The device context the pixel format was set on. */
	HDC deviceContext() const noexcept { return dc_; }
	/** @brief The Win32 window (borrowed, not owned). */
	HWND nativeWindow() const noexcept { return window_; }
	/** @brief The pixel format index chosen for the window. */
	int pixelFormat() const noexcept { return pixelFormat_; }

private:
	ContextConfig config_;
	HWND window_ = nullptr;
	HDC dc_ = nullptr;
	HGLRC context_ = nullptr;
	int pixelFormat_ = 0;
	int wglVersion_ = 0;
};

}

#endif // KALDIGLWCONTEXT_HPP
