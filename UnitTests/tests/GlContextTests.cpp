/**
 * @file GlContextTests.cpp
 * @brief Verifies the window-handle extraction and the default context config.
 *
 * No GL context is created here - that needs a live display. What is tested is the
 * logic that decides *what* would be created, which is exactly the part that used
 * to be a hard-coded X11 assumption in the salvaged code.
 *
 * The test binary is built without `GL_IMPLEMENTATION`/`GLES_IMPLEMENTATION`, so
 * `KALDIGL_NS` resolves to `GL` and the desktop defaults apply.
 */
#include <catch2/catch_test_macros.hpp>
#include <KaldiGLCommon/GlContext.hpp>
#include <cstdint>
#include <string_view>

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
}

TEST_CASE("Unsupported SDL subsystems are reported instead of being used", "[glcontext]")
{
	SDL_SysWMinfo info{};
	info.subsystem = SDL_SYSWM_UNKNOWN;

	const GL::NativeWindowHandles handles = GL::nativeHandlesFrom(info);
	CHECK_FALSE(handles.usable());
	CHECK(handles.window == nullptr);
	CHECK(handles.suggestedApi == GL::ContextApi::Auto);
	CHECK_FALSE(handles.unsupportedReason.empty());
}

#if defined(SDL_VIDEO_DRIVER_X11)
TEST_CASE("X11 windows expose the X11 display and suggest GLX", "[glcontext]")
{
	SDL_SysWMinfo info{};
	info.subsystem = SDL_SYSWM_X11;
	info.info.x11.display = reinterpret_cast<::Display*>(static_cast<uintptr_t>(0x1234));
	info.info.x11.window = static_cast<::Window>(0x5678);

	const GL::NativeWindowHandles handles = GL::nativeHandlesFrom(info);
	CHECK(handles.usable());
	CHECK(handles.display == reinterpret_cast<void*>(static_cast<uintptr_t>(0x1234)));
	CHECK(handles.window == reinterpret_cast<void*>(static_cast<uintptr_t>(0x5678)));
	CHECK(handles.suggestedApi == GL::ContextApi::GLX);
	CHECK(handles.unsupportedReason.empty());
}
#endif

#if defined(SDL_VIDEO_DRIVER_WAYLAND)
TEST_CASE("Wayland windows need a wl_egl_window and suggest EGL", "[glcontext]")
{
	SDL_SysWMinfo info{};
	info.subsystem = SDL_SYSWM_WAYLAND;
	info.info.wl.display = nullptr;
	info.info.wl.egl_window = nullptr;

	GL::NativeWindowHandles handles = GL::nativeHandlesFrom(info);
	CHECK(handles.suggestedApi == GL::ContextApi::EGL);
	CHECK_FALSE(handles.usable());
	CHECK_FALSE(handles.unsupportedReason.empty());

	info.info.wl.egl_window = reinterpret_cast<::wl_egl_window*>(static_cast<uintptr_t>(0x9abc));
	handles = GL::nativeHandlesFrom(info);
	CHECK(handles.usable());
	CHECK(handles.window == reinterpret_cast<void*>(static_cast<uintptr_t>(0x9abc)));
}
#endif
