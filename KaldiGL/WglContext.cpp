/**
 * @file WglContext.cpp
 * @brief Implements WglContext.hpp.
 *
 * @warning This translation unit only builds on Windows and is not covered by the
 * repository's CI (which runs on Linux). It is written against the documented
 * WGL API and `WGL_ARB_create_context`/`WGL_ARB_pixel_format` behaviour.
 */
#include "WglContext.hpp"
#include <stdexcept>
#include <string>
#include <vector>

namespace KALDIGL_NS {

namespace {

/** @brief Builds a PIXELFORMATDESCRIPTOR matching @p config. */
PIXELFORMATDESCRIPTOR makePixelFormat(const ContextConfig& config)
{
	PIXELFORMATDESCRIPTOR descriptor{};
	descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	descriptor.nVersion = 1;
	descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	if (config.doubleBuffered) {
		descriptor.dwFlags |= PFD_DOUBLEBUFFER;
	}
	descriptor.iPixelType = PFD_TYPE_RGBA;
	descriptor.cColorBits = static_cast<BYTE>(config.redBits + config.greenBits + config.blueBits);
	descriptor.cRedBits = static_cast<BYTE>(config.redBits);
	descriptor.cGreenBits = static_cast<BYTE>(config.greenBits);
	descriptor.cBlueBits = static_cast<BYTE>(config.blueBits);
	descriptor.cAlphaBits = static_cast<BYTE>(config.alphaBits);
	descriptor.cDepthBits = static_cast<BYTE>(config.depthBits);
	descriptor.cStencilBits = static_cast<BYTE>(config.stencilBits);
	descriptor.iLayerType = PFD_MAIN_PLANE;
	return descriptor;
}

/** @brief Builds the `wglCreateContextAttribsARB` attribute list for @p config. */
std::vector<int> contextAttributes(const ContextConfig& config)
{
	std::vector<int> attribs = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, config.versionMajor,
		WGL_CONTEXT_MINOR_VERSION_ARB, config.versionMinor
	};
	if (config.coreProfile) {
		attribs.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
		attribs.push_back(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);
	}
	int flags = 0;
	if (config.forwardCompatible) {
		flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
	}
	if (config.debugContext) {
		flags |= WGL_CONTEXT_DEBUG_BIT_ARB;
	}
	if (flags != 0) {
		attribs.push_back(WGL_CONTEXT_FLAGS_ARB);
		attribs.push_back(flags);
	}
	attribs.push_back(0);
	return attribs;
}

} // namespace

WglContext::WglContext(HWND window, const ContextConfig& config)
	: config_(config)
	, window_(window)
{
	if (window_ == nullptr) {
		throw std::invalid_argument("WglContext: no Win32 window was supplied");
	}

	dc_ = GetDC(window_);
	if (dc_ == nullptr) {
		throw std::runtime_error("WglContext: GetDC failed for the window");
	}

	const PIXELFORMATDESCRIPTOR descriptor = makePixelFormat(config_);
	pixelFormat_ = ChoosePixelFormat(dc_, &descriptor);
	if (pixelFormat_ == 0) {
		ReleaseDC(window_, dc_);
		dc_ = nullptr;
		throw std::runtime_error("WglContext: ChoosePixelFormat found no matching pixel format");
	}
	if (SetPixelFormat(dc_, pixelFormat_, &descriptor) == FALSE) {
		ReleaseDC(window_, dc_);
		dc_ = nullptr;
		throw std::runtime_error("WglContext: SetPixelFormat failed");
	}

	wglVersion_ = gladLoaderLoadWGL(dc_);
	if (wglVersion_ == 0) {
		// Still try the legacy path; only the WGL extension helpers are missing.
		wglVersion_ = 1;
	}

	if (wglCreateContextAttribsARB != nullptr) {
		const std::vector<int> attribs = contextAttributes(config_);
		context_ = wglCreateContextAttribsARB(dc_, nullptr, attribs.data());
	}
	if (context_ == nullptr) {
		// Legacy path: a compatibility 1.x context. The caller's GL loader will
		// report the version mismatch if the requested profile is unavailable.
		context_ = wglCreateContext(dc_);
	}
	if (context_ == nullptr) {
		gladLoaderUnloadWGL();
		ReleaseDC(window_, dc_);
		dc_ = nullptr;
		throw std::runtime_error("WglContext: wglCreateContext failed");
	}
}

WglContext::~WglContext()
{
	if (dc_ != nullptr && context_ != nullptr && wglGetCurrentContext() == context_) {
		wglMakeCurrent(nullptr, nullptr);
	}
	if (context_ != nullptr) {
		wglDeleteContext(context_);
		context_ = nullptr;
	}
	if (dc_ != nullptr) {
		ReleaseDC(window_, dc_);
		dc_ = nullptr;
	}
	if (wglVersion_ > 1) {
		gladLoaderUnloadWGL();
	}
	wglVersion_ = 0;
}

void WglContext::makeCurrent()
{
	if (wglMakeCurrent(dc_, context_) == FALSE) {
		throw std::runtime_error("WglContext: wglMakeCurrent failed");
	}
}

void WglContext::releaseCurrent() noexcept
{
	if (dc_ != nullptr) {
		wglMakeCurrent(nullptr, nullptr);
	}
}

void WglContext::swapBuffers()
{
	SwapBuffers(dc_);
}

std::unique_ptr<Context> createWglContext(const Kld::NativeWindowHandles& handles, const ContextConfig& config)
{
	if (handles.system != Kld::WindowSystem::Windows || handles.window == nullptr) {
		throw std::invalid_argument(std::string("WglContext: WGL requires a Win32 window, got ") +
									std::string(Kld::windowSystemName(handles.system)));
	}
	return std::make_unique<WglContext>(static_cast<HWND>(handles.window), config);
}

}
