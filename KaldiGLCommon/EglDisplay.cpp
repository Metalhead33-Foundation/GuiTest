/**
 * @file EglDisplay.cpp
 * @brief Implements EglDisplay.hpp.
 */
#include "EglDisplay.hpp"
#include <cstring>
#include <iterator>
#include <map>
#include <mutex>
#include <stdexcept>

namespace KALDIGL_NS {

namespace {

/** @brief Loads libEGL's entry points once, before the first EGL call. */
void ensureEglLoaded()
{
	static std::once_flag loadOnce;
	std::call_once(loadOnce, [] {
		if (gladLoaderLoadEGL(nullptr) == 0) {
			throw std::runtime_error("EglDisplay: libEGL could not be loaded");
		}
	});
}

/** @brief Space-delimited EGL extension check. */
bool hasExtension(const char* extensions, const char* name)
{
	if (extensions == nullptr || name == nullptr) {
		return false;
	}
	const std::size_t length = std::strlen(name);
	for (const char* position = extensions; (position = std::strstr(position, name)) != nullptr; position += length) {
		if (position[length] == ' ' || position[length] == '\0') {
			return true;
		}
	}
	return false;
}

/** @brief Builds the exception message for a failed EGL call. */
[[noreturn]] void throwEglFailure(const char* step, EGLint error)
{
	std::string message = "EglDisplay: ";
	message += step;
	message += " failed (";
	message += eglErrorName(error);
	message += ")";
	throw std::runtime_error(message);
}

} // namespace

const char* eglErrorName(EGLint error) noexcept
{
	switch (error) {
		case EGL_SUCCESS: return "EGL_SUCCESS";
		case EGL_NOT_INITIALIZED: return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS: return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC: return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE: return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONTEXT: return "EGL_BAD_CONTEXT";
		case EGL_BAD_CONFIG: return "EGL_BAD_CONFIG";
		case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY: return "EGL_BAD_DISPLAY";
		case EGL_BAD_SURFACE: return "EGL_BAD_SURFACE";
		case EGL_BAD_MATCH: return "EGL_BAD_MATCH";
		case EGL_BAD_PARAMETER: return "EGL_BAD_PARAMETER";
		case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
		case EGL_CONTEXT_LOST: return "EGL_CONTEXT_LOST";
		default: return "EGL_<unknown>";
	}
}

EglDisplay::EglDisplay(EGLNativeDisplayType nativeDisplay)
{
	ensureEglLoaded();

	display_ = eglGetDisplay(nativeDisplay);
	if (display_ == EGL_NO_DISPLAY) {
		throwEglFailure("eglGetDisplay", eglGetError());
	}
	if (!eglInitialize(display_, &majorVersion_, &minorVersion_)) {
		const EGLint error = eglGetError();
		display_ = EGL_NO_DISPLAY;
		throwEglFailure("eglInitialize", error);
	}
	// The bootstrap in ensureEglLoaded() ran before any display was initialized, so
	// glad could only probe the bare EGL 1.0 entry points (it queries EGL_VERSION
	// of an uninitialized display, gets null and falls back to 1.0). Re-run the
	// loader with the initialized display so the 1.4/1.5 entry points - notably
	// eglGetCurrentContext, which the context destructor uses - become available
	// as well. The initial 1.0 pointers are already present, so a failure here only
	// leaves them untouched.
	(void)gladLoaderLoadEGL(display_);
	supportsContextProfiles_ = (majorVersion_ > 1 || (majorVersion_ == 1 && minorVersion_ >= 5)) ||
							   hasExtension(eglQueryString(display_, EGL_EXTENSIONS), "EGL_KHR_create_context");
}

EglDisplay::EglDisplay(EglDisplay&& other) noexcept
	: display_(other.display_)
	, majorVersion_(other.majorVersion_)
	, minorVersion_(other.minorVersion_)
	, supportsContextProfiles_(other.supportsContextProfiles_)
{
	other.display_ = EGL_NO_DISPLAY;
	other.majorVersion_ = 0;
	other.minorVersion_ = 0;
	other.supportsContextProfiles_ = false;
}

EglDisplay& EglDisplay::operator=(EglDisplay&& other) noexcept
{
	if (this != &other) {
		terminate();
		display_ = other.display_;
		majorVersion_ = other.majorVersion_;
		minorVersion_ = other.minorVersion_;
		supportsContextProfiles_ = other.supportsContextProfiles_;
		other.display_ = EGL_NO_DISPLAY;
		other.majorVersion_ = 0;
		other.minorVersion_ = 0;
		other.supportsContextProfiles_ = false;
	}
	return *this;
}

EglDisplay::~EglDisplay()
{
	terminate();
}

void EglDisplay::terminate() noexcept
{
	if (display_ != EGL_NO_DISPLAY) {
		eglTerminate(display_);
		display_ = EGL_NO_DISPLAY;
	}
	supportsContextProfiles_ = false;
	majorVersion_ = 0;
	minorVersion_ = 0;
}

std::shared_ptr<EglDisplay> EglDisplay::acquire(EGLNativeDisplayType nativeDisplay)
{
	ensureEglLoaded();

	const EGLDisplay key = eglGetDisplay(nativeDisplay);
	if (key == EGL_NO_DISPLAY) {
		throwEglFailure("eglGetDisplay", eglGetError());
	}

	static std::mutex mutex;
	static std::map<EGLDisplay, std::weak_ptr<EglDisplay>> cache;
	const std::lock_guard<std::mutex> lock(mutex);

	for (auto it = cache.begin(); it != cache.end();) {
		it = it->second.expired() ? cache.erase(it) : std::next(it);
	}
	if (const auto it = cache.find(key); it != cache.end()) {
		if (std::shared_ptr<EglDisplay> existing = it->second.lock()) {
			return existing;
		}
	}

	std::shared_ptr<EglDisplay> created = std::make_shared<EglDisplay>(nativeDisplay);
	cache[key] = created;
	return created;
}

std::string EglDisplay::description() const
{
	if (!isValid()) {
		return "EGL (uninitialized)";
	}
	std::string text = "EGL ";
	text += std::to_string(majorVersion_);
	text += ".";
	text += std::to_string(minorVersion_);
	if (const char* vendor = eglQueryString(display_, EGL_VENDOR); vendor != nullptr) {
		text += " (";
		text += vendor;
		text += ")";
	}
	return text;
}

}
