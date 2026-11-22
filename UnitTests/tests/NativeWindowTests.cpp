/**
 * @file NativeWindowTests.cpp
 * @brief Verifies the SDL window-handle extraction in `Kaldi/KldNativeWindow.hpp`.
 *
 * No context is created here - that needs a live display. What is tested is the
 * logic that decides *what* a backend would be given, which is the part that used
 * to be a hard-coded X11 assumption inside the GL backend.
 */
#include <catch2/catch_test_macros.hpp>
#include <Kaldi/KldNativeWindow.hpp>
#include <KaldiGLCommon/GlContext.hpp>
#include <cstdint>
#include <string_view>

TEST_CASE("Unsupported SDL subsystems are reported instead of being used", "[nativewindow]")
{
	SDL_SysWMinfo info{};
	info.subsystem = SDL_SYSWM_UNKNOWN;

	const Kld::NativeWindowHandles handles = Kld::nativeWindowHandlesFrom(info);
	CHECK_FALSE(handles.usable());
	CHECK(handles.system == Kld::WindowSystem::Unknown);
	CHECK(handles.display == nullptr);
	CHECK(handles.window == nullptr);
	CHECK_FALSE(handles.unsupportedReason.empty());
}

#if defined(SDL_VIDEO_DRIVER_X11)
TEST_CASE("X11 windows expose the X11 display, window and no EGL override", "[nativewindow]")
{
	SDL_SysWMinfo info{};
	info.subsystem = SDL_SYSWM_X11;
	info.info.x11.display = reinterpret_cast<::Display*>(static_cast<uintptr_t>(0x1234));
	info.info.x11.window = static_cast<::Window>(0x5678);

	const Kld::NativeWindowHandles handles = Kld::nativeWindowHandlesFrom(info);
	CHECK(handles.usable());
	CHECK(handles.system == Kld::WindowSystem::X11);
	CHECK(handles.display == reinterpret_cast<void*>(static_cast<uintptr_t>(0x1234)));
	CHECK(handles.window == reinterpret_cast<void*>(static_cast<uintptr_t>(0x5678)));
	CHECK(handles.eglWindow == nullptr);
	CHECK(handles.eglDrawable() == handles.window);
	CHECK(handles.unsupportedReason.empty());
}
#endif

#if defined(SDL_VIDEO_DRIVER_WAYLAND)
TEST_CASE("Wayland windows expose the surface and the wl_egl_window separately", "[nativewindow]")
{
	SDL_SysWMinfo info{};
	info.subsystem = SDL_SYSWM_WAYLAND;
	info.info.wl.display = nullptr;
	info.info.wl.surface = reinterpret_cast<::wl_surface*>(static_cast<uintptr_t>(0x1111));
	info.info.wl.egl_window = reinterpret_cast<::wl_egl_window*>(static_cast<uintptr_t>(0x2222));

	const Kld::NativeWindowHandles handles = Kld::nativeWindowHandlesFrom(info);
	CHECK(handles.usable());
	CHECK(handles.system == Kld::WindowSystem::Wayland);
	CHECK(handles.window == reinterpret_cast<void*>(static_cast<uintptr_t>(0x1111)));
	CHECK(handles.eglWindow == reinterpret_cast<void*>(static_cast<uintptr_t>(0x2222)));
	// EGL must consume the wrapper, Vulkan the raw surface.
	CHECK(handles.eglDrawable() == handles.eglWindow);
}
#endif

TEST_CASE("Window system names are stable identifiers", "[nativewindow]")
{
	CHECK(Kld::windowSystemName(Kld::WindowSystem::X11) == std::string_view("X11"));
	CHECK(Kld::windowSystemName(Kld::WindowSystem::Wayland) == std::string_view("Wayland"));
	CHECK(Kld::windowSystemName(Kld::WindowSystem::Windows) == std::string_view("Win32"));
	CHECK(Kld::windowSystemName(Kld::WindowSystem::Android) == std::string_view("Android"));
	CHECK(Kld::windowSystemName(Kld::WindowSystem::Cocoa) == std::string_view("Cocoa"));
	CHECK(Kld::windowSystemName(Kld::WindowSystem::Unknown) == std::string_view("unknown"));
}

TEST_CASE("The default GL context config matches the desktop profile", "[glcontext]")
{
	const GL::ContextConfig config = GL::defaultContextConfig();
	CHECK(config.api == GL::ContextApi::Auto);
	CHECK(config.versionMajor == 4);
	CHECK(config.versionMinor == 3);
	CHECK(config.coreProfile);
	CHECK(config.forwardCompatible);
	CHECK(config.doubleBuffered);
	CHECK(config.depthBits == 24);
	CHECK(config.stencilBits == 8);
	CHECK(config.screen == -1);
	CHECK_FALSE(GL::wantsMultisampling(config));

	GL::ContextConfig multisampled = config;
	multisampled.sampleCount = 4;
	CHECK(GL::wantsMultisampling(multisampled));
}

TEST_CASE("Context API names are stable identifiers", "[glcontext]")
{
	CHECK(GL::contextApiName(GL::ContextApi::Auto) == std::string_view("auto"));
	CHECK(GL::contextApiName(GL::ContextApi::EGL) == std::string_view("EGL"));
	CHECK(GL::contextApiName(GL::ContextApi::GLX) == std::string_view("GLX"));
	CHECK(GL::contextApiName(GL::ContextApi::WGL) == std::string_view("WGL"));
}
