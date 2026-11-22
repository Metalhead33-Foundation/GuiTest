#ifndef KLDNATIVEWINDOW_HPP
#define KLDNATIVEWINDOW_HPP
/**
 * @file KldNativeWindow.hpp
 * @brief Backend-neutral extraction of native window handles from SDL.
 *
 * Every backend faces the same first problem: the engine hands over an
 * `SDL_SysWMinfo`, but EGL wants an `EGLNativeDisplayType`/`EGLNativeWindowType`,
 * GLX wants a `Display*`/`Window`, WGL an `HWND`, Direct3D an `HWND` plus an
 * `HINSTANCE`, and Vulkan a platform surface create-info. Doing that switch once,
 * here, keeps it out of the individual backends.
 *
 * The handles are `void*` on purpose: this header stays free of Xlib, Wayland,
 * Windows and EGL types, so it can be included from any backend translation unit
 * (and from tests). Casting back is the caller's job, and `WindowSystem` tells it
 * what the pointers really are.
 *
 * @note SDL's `SDL_SysWMinfo` only contains the subsystem structs SDL itself was
 * built with, so every branch is guarded by the matching `SDL_VIDEO_DRIVER_*`
 * macro. A host without a given backend simply reports it as unsupported.
 */
#include <cstdint>
#include <string_view>
#include <SDL2/SDL_syswm.h>

namespace Kld {

/** @brief Windowing system SDL reported for a window. */
enum class WindowSystem : uint8_t {
	/** @brief SDL could not tell, or the subsystem has no GL/Vulkan path here. */
	Unknown,
	/** @brief X11: `display` is `Display*`, `window` is `Window`. */
	X11,
	/** @brief Wayland: `display` is `wl_display*`, `window` is `wl_surface*`. */
	Wayland,
	/** @brief Win32: `display` is an `HINSTANCE` (may be null), `window` is an `HWND`. */
	Windows,
	/** @brief Android: `window` is an `ANativeWindow*`. */
	Android,
	/** @brief macOS/Cocoa: `window` is an `NSWindow*` (no backend path yet). */
	Cocoa
};

/** @brief Stable, human-readable name of a window system. */
constexpr std::string_view windowSystemName(WindowSystem system) noexcept
{
	switch (system) {
		case WindowSystem::Unknown: return "unknown";
		case WindowSystem::X11: return "X11";
		case WindowSystem::Wayland: return "Wayland";
		case WindowSystem::Windows: return "Win32";
		case WindowSystem::Android: return "Android";
		case WindowSystem::Cocoa: return "Cocoa";
	}
	return "unknown";
}

/**
 * @brief Native handles of one window, tagged with the system that produced them.
 *
 * `display` is the *connection* handle (X11 display, Wayland display, HINSTANCE),
 * `window` the drawable. Both are borrowed: SDL owns them, and they must outlive
 * whatever backend object is created from them.
 */
struct NativeWindowHandles {
	/** @brief Windowing system the handles belong to. */
	WindowSystem system = WindowSystem::Unknown;
	/** @brief Connection/display handle, or @c nullptr when the platform has none. */
	void* display = nullptr;
	/** @brief Drawable handle; @c nullptr when extraction failed. */
	void* window = nullptr;
	/**
	 * @brief Alternate drawable for EGL, when the platform distinguishes them.
	 *
	 * Only Wayland does: Vulkan wants the `wl_surface`, EGL wants a
	 * `wl_egl_window`. Null everywhere else, and null on Wayland when SDL did not
	 * expose one.
	 */
	void* eglWindow = nullptr;
	/** @brief Non-empty when the window cannot be used (carries the reason). */
	std::string_view unsupportedReason;

	/** @brief True when a drawable was extracted and no platform gap was hit. */
	bool usable() const noexcept { return window != nullptr && unsupportedReason.empty(); }

	/** @brief The drawable an EGL implementation should use (falls back to window()). */
	void* eglDrawable() const noexcept { return eglWindow != nullptr ? eglWindow : window; }
};

/**
 * @brief Extracts the native display/window handles from SDL window-manager info.
 *
 * The caller must have filled @p info through `SDL_GetWindowWMInfo()`; this
 * function only interprets it. No backend is selected here - the same handles
 * feed EGL, GLX, WGL, Direct3D and Vulkan.
 */
inline NativeWindowHandles nativeWindowHandlesFrom(const SDL_SysWMinfo& info) noexcept
{
	NativeWindowHandles handles;
	switch (info.subsystem) {
#if defined(SDL_VIDEO_DRIVER_X11)
		case SDL_SYSWM_X11:
			handles.system = WindowSystem::X11;
			handles.display = info.info.x11.display;
			handles.window = reinterpret_cast<void*>(static_cast<uintptr_t>(info.info.x11.window));
			break;
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
		case SDL_SYSWM_WAYLAND:
			handles.system = WindowSystem::Wayland;
			handles.display = info.info.wl.display;
			// Vulkan (and Wayland's wl_surface semantics) want the surface itself;
			// EGL additionally needs SDL's wl_egl_window wrapper.
			handles.window = info.info.wl.surface;
			handles.eglWindow = info.info.wl.egl_window;
			break;
#endif
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
		case SDL_SYSWM_WINDOWS:
			handles.system = WindowSystem::Windows;
			handles.display = info.info.win.hinstance;
			handles.window = info.info.win.window;
			break;
#endif
#if defined(SDL_VIDEO_DRIVER_ANDROID)
		case SDL_SYSWM_ANDROID:
			handles.system = WindowSystem::Android;
			handles.window = info.info.android.window;
			break;
#endif
#if defined(SDL_VIDEO_DRIVER_COCOA)
		case SDL_SYSWM_COCOA:
			handles.system = WindowSystem::Cocoa;
			handles.window = info.info.cocoa.window;
			handles.unsupportedReason = "no OpenGL/Vulkan/Direct3D backend path is implemented for Cocoa yet";
			break;
#endif
		default:
			handles.system = WindowSystem::Unknown;
			handles.unsupportedReason = "unsupported SDL window subsystem";
			break;
	}
	return handles;
}

}

#endif // KLDNATIVEWINDOW_HPP
