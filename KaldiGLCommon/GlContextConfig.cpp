/**
 * @file GlContextConfig.cpp
 * @brief Implements GlContextConfig.hpp.
 */
#include "GlContextConfig.hpp"
#include <string>
#include <vector>

namespace KALDIGL_NS {

namespace {

/** @brief Dialect-aware textual default of a version field. */
constexpr const char* defaultVersion(const char* desktop, const char* es) noexcept
{
#if KALDIGL_IS_GLES
	(void)desktop;
	return es;
#else
	(void)es;
	return desktop;
#endif
}

/** @brief Dialect-aware textual default of a profile flag. */
constexpr const char* defaultFlag(bool desktopValue) noexcept
{
#if KALDIGL_IS_GLES
	(void)desktopValue;
	return "false";
#else
	return desktopValue ? "true" : "false";
#endif
}

/** @brief Common fields followed by the GL-specific ones, built once. */
const std::vector<Kld::ConfigField>& allFields()
{
	static const std::vector<Kld::ConfigField> fields = [] {
		std::vector<Kld::ConfigField> result;
		const std::span<const Kld::ConfigField> common = Kld::commonConfigFields();
		result.assign(common.begin(), common.end());

		result.push_back({ "sGlApi", Kld::ConfigValueType::StringValue, 0, 0, "auto", "auto|egl|glx|wgl",
						   "Window-system binding for the GL context; auto picks GLX on X11, WGL on "
						   "Win32 and EGL elsewhere." });
		result.push_back({ "iGlVersionMajor", Kld::ConfigValueType::IntegerValue, 1, 5, defaultVersion("4", "3"), nullptr,
						   "Requested OpenGL/OpenGL ES major version." });
		result.push_back({ "iGlVersionMinor", Kld::ConfigValueType::IntegerValue, 0, 9, defaultVersion("3", "2"), nullptr,
						   "Requested OpenGL/OpenGL ES minor version." });
		result.push_back({ "bGlCoreProfile", Kld::ConfigValueType::BoolValue, 0, 0, defaultFlag(true), nullptr,
						   "Request a core profile (desktop OpenGL only)." });
		result.push_back({ "bGlForwardCompatible", Kld::ConfigValueType::BoolValue, 0, 0, defaultFlag(true), nullptr,
						   "Request a forward-compatible context (desktop OpenGL only)." });
		result.push_back({ "bGlDebugContext", Kld::ConfigValueType::BoolValue, 0, 0, "false", nullptr,
						   "Request a debug context when the driver offers one." });
		result.push_back({ "iGlSamples", Kld::ConfigValueType::IntegerValue, 0, 16, "0", nullptr,
						   "MSAA sample count; 0 or 1 disables multisampling." });
		result.push_back({ "iGlDepthBits", Kld::ConfigValueType::IntegerValue, 0, 32, "24", nullptr,
						   "Depth buffer size in bits; 0 disables the depth buffer." });
		result.push_back({ "iGlStencilBits", Kld::ConfigValueType::IntegerValue, 0, 8, "8", nullptr,
						   "Stencil buffer size in bits; 0 disables the stencil buffer." });
		result.push_back({ "iGlScreen", Kld::ConfigValueType::IntegerValue, -1, 15, "-1", nullptr,
						   "X11 screen index for GLX; -1 selects the default screen." });
		return result;
	}();
	return fields;
}

/** @brief Finds a field by key; the table is small, so a linear scan is fine. */
const Kld::ConfigField& findField(std::string_view key)
{
	for (const Kld::ConfigField& field : allFields()) {
		if (key == field.key) {
			return field;
		}
	}
	// Every caller below passes a literal that exists in allFields(); a missing key
	// would be a programming error, and returning the first field keeps it harmless.
	return allFields().front();
}

} // namespace

std::string_view contextApiConfigName(ContextApi api) noexcept
{
	switch (api) {
		case ContextApi::Auto: return "auto";
		case ContextApi::EGL: return "egl";
		case ContextApi::GLX: return "glx";
		case ContextApi::WGL: return "wgl";
	}
	return "auto";
}

ContextApi contextApiFromConfigName(std::string_view name) noexcept
{
	if (name == "egl" || name == "EGL") {
		return ContextApi::EGL;
	}
	if (name == "glx" || name == "GLX") {
		return ContextApi::GLX;
	}
	if (name == "wgl" || name == "WGL") {
		return ContextApi::WGL;
	}
	return ContextApi::Auto;
}

ContextConfigProvider::ContextConfigProvider(ContextConfig& config) noexcept
	: config_(config)
{
}

Kld::DeviceConfig ContextConfigProvider::description(Kld::ContextPurpose purpose) const
{
	Kld::DeviceConfig config;
	switch (purpose) {
		case Kld::ContextPurpose::MainWindow:
			config.width = 1280;
			config.height = 720;
			config.framesInFlight = 2;
			config.vsync = true;
			config.srgbFramebuffer = true;
			break;
		case Kld::ContextPurpose::Offscreen:
			config.width = 1024;
			config.height = 1024;
			config.framesInFlight = 1;
			config.vsync = false;
			config.srgbFramebuffer = false;
			break;
		case Kld::ContextPurpose::DebugView:
			config.width = 640;
			config.height = 360;
			config.framesInFlight = 2;
			config.vsync = false;
			config.srgbFramebuffer = false;
			break;
	}
	return config;
}

std::span<const Kld::ConfigField> ContextConfigProvider::fields(Kld::ContextPurpose /*purpose*/) const
{
	return allFields();
}

bool ContextConfigProvider::apply(const Kld::ConfigReader& reader)
{
	config_.api = contextApiFromConfigName(Kld::readConfigString(findField("sGlApi"), reader));
	config_.versionMajor = static_cast<int>(Kld::readConfigInteger(findField("iGlVersionMajor"), reader));
	config_.versionMinor = static_cast<int>(Kld::readConfigInteger(findField("iGlVersionMinor"), reader));
	config_.coreProfile = Kld::readConfigBool(findField("bGlCoreProfile"), reader);
	config_.forwardCompatible = Kld::readConfigBool(findField("bGlForwardCompatible"), reader);
	config_.debugContext = Kld::readConfigBool(findField("bGlDebugContext"), reader);
	config_.sampleCount = static_cast<int>(Kld::readConfigInteger(findField("iGlSamples"), reader));
	config_.depthBits = static_cast<int>(Kld::readConfigInteger(findField("iGlDepthBits"), reader));
	config_.stencilBits = static_cast<int>(Kld::readConfigInteger(findField("iGlStencilBits"), reader));
	config_.screen = static_cast<int>(Kld::readConfigInteger(findField("iGlScreen"), reader));
	return true;
}

}
