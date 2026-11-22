#ifndef KALDIGLEGLDISPLAY_HPP
#define KALDIGLEGLDISPLAY_HPP
/**
 * @file EglDisplay.hpp
 * @brief Reference-counted `EGLDisplay` owner shared by every EGL context.
 *
 * EGL ties a context and a surface to an initialized display, and initializing
 * the same native display twice is only safe as long as it is terminated twice.
 * `EglDisplay` therefore owns exactly one `eglInitialize`/`eglTerminate` pair,
 * and EGL contexts share it through `std::shared_ptr`. `EglDisplay::acquire()`
 * hands out the process-wide instance for a native display, so a second window
 * (or a share context) reuses the display instead of re-initializing it.
 */
#include "GlBackend.hpp"
#include <glad/egl.h>
#include <memory>
#include <string>

namespace KALDIGL_NS {

/** @brief Symbolic name of an `eglGetError()` code ("EGL_SUCCESS", "EGL_BAD_CONFIG", ...). */
const char* eglErrorName(EGLint error) noexcept;

/**
 * @brief Owns an initialized EGL display and terminates it on destruction.
 *
 * @note Move-only. Copies would double-terminate the display.
 */
class EglDisplay {
public:
	/**
	 * @brief Gets the display for @p nativeDisplay and initializes it.
	 * @param nativeDisplay Platform display handle; `EGL_DEFAULT_DISPLAY` for the default.
	 * @throws std::runtime_error When libEGL cannot be loaded, or `eglGetDisplay`
	 *         or `eglInitialize` fails; the message carries `eglErrorName()`.
	 */
	explicit EglDisplay(EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY);

	/** @brief Takes over another display; the source is left uninitialized. */
	EglDisplay(EglDisplay&& other) noexcept;
	/** @brief Terminates the current display and takes over @p other. */
	EglDisplay& operator=(EglDisplay&& other) noexcept;

	EglDisplay(const EglDisplay&) = delete;
	EglDisplay& operator=(const EglDisplay&) = delete;

	/** @brief Terminates the display when it is still initialized. */
	~EglDisplay();

	/**
	 * @brief Returns the shared process-wide display for @p nativeDisplay.
	 *
	 * The instance is created on first use and destroyed with the last owner, so
	 * contexts may be created and destroyed in any order. Thread-safe.
	 *
	 * @throws std::runtime_error When libEGL cannot be loaded or initialization fails.
	 */
	static std::shared_ptr<EglDisplay> acquire(EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY);

	/** @brief The raw `EGLDisplay` (`EGL_NO_DISPLAY` when uninitialized). */
	EGLDisplay handle() const noexcept { return display_; }
	/** @brief Major EGL version reported by `eglInitialize`. */
	EGLint majorVersion() const noexcept { return majorVersion_; }
	/** @brief Minor EGL version reported by `eglInitialize`. */
	EGLint minorVersion() const noexcept { return minorVersion_; }
	/** @brief True when the display is initialized. */
	bool isValid() const noexcept { return display_ != EGL_NO_DISPLAY; }

	/**
	 * @brief True when the context version/profile attributes are understood.
	 *
	 * That is the case for EGL 1.5 and for `EGL_KHR_create_context`; without it a
	 * context can only be requested through `EGL_CONTEXT_CLIENT_VERSION`.
	 */
	bool supportsContextProfiles() const noexcept { return supportsContextProfiles_; }

	/** @brief "EGL 1.5 (vendor)" style description for logs and error messages. */
	std::string description() const;

private:
	/** @brief Terminates the display if it is initialized. */
	void terminate() noexcept;

	EGLDisplay display_ = EGL_NO_DISPLAY;
	EGLint majorVersion_ = 0;
	EGLint minorVersion_ = 0;
	bool supportsContextProfiles_ = false;
};

}

#endif // KALDIGLEGLDISPLAY_HPP
