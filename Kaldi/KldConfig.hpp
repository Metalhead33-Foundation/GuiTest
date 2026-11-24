#ifndef KLDCONFIG_HPP
#define KLDCONFIG_HPP
/**
 * @file KldConfig.hpp
 * @brief Backend configuration contract: common settings plus per-backend fields.
 *
 * A Kaldi backend needs two very different kinds of configuration:
 *
 *  - **Common settings** every backend understands: resolution, frames in flight,
 *    present mode/vsync, sRGB, texture quality. These live in `DeviceConfig`.
 *  - **Backend-specific settings** that only make sense for one API: EGL vs GLX on
 *    Linux, the Vulkan device name, the Direct3D feature level, ... Those are
 *    described as `ConfigField` entries and applied by the backend itself.
 *
 * The engine drives the conversation in both directions:
 *
 * 1. it asks the backend for the defaults it recommends for a purpose
 *    (`description()`),
 * 2. it asks which fields the backend exposes and in which range (`fields()`),
 * 3. it persists the merged result (for example as an `.ini` file through
 *    `Euph::Conf::Configuration`, which is what
 *    `Lotte/Conf/LteBackendConfig.hpp` does),
 * 4. on the next start it parses the file into a `ConfigReader` and calls
 *    `apply()`, which clamps every value to the range from step 2.
 *
 * Key names follow Euphemy's typed-key convention so the same strings can be
 * written to the `.ini` file verbatim: `b` bool, `i` integer, `f` float,
 * `s` string (unsigned values are `i` fields with `minimum >= 0`).
 *
 * The whole header is inline: Kaldi is consumed header-only, so there is no
 * library to link for the configuration vocabulary.
 */
#include <Kaldi/KaldiOperation.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace Kld {

// ─────────────────────────────────────────────────────────────────────────────
// Purpose and value types
// ─────────────────────────────────────────────────────────────────────────────

/** @brief What the engine is creating the device/context for. */
enum class ContextPurpose : uint8_t {
	/** @brief The window the game is played in. */
	MainWindow,
	/** @brief A render target that is never presented (thumbnail, reflection, ...). */
	Offscreen,
	/** @brief A debugging overlay/preview window. */
	DebugView
};

/** @brief Stable name of a purpose ("main", "offscreen", "debug"). */
constexpr std::string_view contextPurposeName(ContextPurpose purpose) noexcept
{
	switch (purpose) {
		case ContextPurpose::MainWindow: return "main";
		case ContextPurpose::Offscreen: return "offscreen";
		case ContextPurpose::DebugView: return "debug";
	}
	return "main";
}

/**
 * @brief Type of a configuration value, mirroring Euphemy's typed keys.
 *
 * @note The enumerators carry a `Value` suffix because several platform SDKs
 * define `Bool` (X11's `Xdefs.h`), and `Integer`/`Float`/`String` are equally
 * attractive names for someone else's typedef. Kaldi's headers are included next
 * to Xlib in the GLX and Vulkan backends, so the enumerators have to survive that.
 */
enum class ConfigValueType : uint8_t {
	/** @brief Bool (`b` prefix). */
	BoolValue,
	/** @brief Signed integer (`i` prefix); use a non-negative range for unsigned values. */
	IntegerValue,
	/** @brief Floating point (`f` prefix). */
	FloatValue,
	/** @brief Free-form string (`s` prefix). */
	StringValue
};

/** @brief Stable name of a value type ("bool", "integer", "float", "string"). */
constexpr std::string_view configValueTypeName(ConfigValueType type) noexcept
{
	switch (type) {
		case ConfigValueType::BoolValue: return "bool";
		case ConfigValueType::IntegerValue: return "integer";
		case ConfigValueType::FloatValue: return "float";
		case ConfigValueType::StringValue: return "string";
	}
	return "string";
}

/** @brief Euphemy typed-key prefix for a value type. */
constexpr char configKeyPrefix(ConfigValueType type) noexcept
{
	switch (type) {
		case ConfigValueType::BoolValue: return 'b';
		case ConfigValueType::IntegerValue: return 'i';
		case ConfigValueType::FloatValue: return 'f';
		case ConfigValueType::StringValue: return 's';
	}
	return 's';
}

/** @brief Backend-independent settings every backend can honour. */
struct DeviceConfig {
	/** @brief Swapchain/render-target width in pixels. */
	uint16_t width = 1280;
	/** @brief Swapchain/render-target height in pixels. */
	uint16_t height = 720;
	/** @brief Frames the backend may keep in flight. */
	uint8_t framesInFlight = 2;
	/** @brief Preferred presentation mode. */
	PresentMode presentMode = PresentMode::Fifo;
	/** @brief Present with vsync. */
	bool vsync = true;
	/** @brief Prefer an sRGB swapchain/render target when the API offers one. */
	bool srgbFramebuffer = true;
	/** @brief Maximum texture anisotropy (`0` disables anisotropic filtering). */
	float maxAnisotropy = 0.0f;
	/** @brief Generate mip chains for uploaded textures unless the asset provides them. */
	bool generateMipmaps = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// Field description
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief One editable setting a backend exposes to the configuration file.
 *
 * `key` already contains the Euphemy type prefix, so it is written to the `.ini`
 * file as-is and parsed back without translation. Numeric bounds are ignored for
 * `Bool` and `String`; `choices` (a `|`-separated list) restricts `String` values.
 */
struct ConfigField {
	/** @brief Typed key, for example `"bGlDebugContext"` or `"iGlVersionMajor"`. */
	const char* key;
	/** @brief Value type, and therefore the meaning of `key[0]`. */
	ConfigValueType type;
	/** @brief Inclusive minimum for numeric types. */
	double minimum;
	/** @brief Inclusive maximum for numeric types. */
	double maximum;
	/** @brief Textual default value written to a fresh configuration file. */
	const char* defaultValue;
	/** @brief Optional `|`-separated list of accepted strings; @c nullptr for free-form. */
	const char* choices;
	/** @brief One-line help text for the generated file and tooling. */
	const char* help;
};

/**
 * @brief Read-only, typed access to the values of one configuration section.
 *
 * Implemented by whoever owns persistence; in this repository that is
 * `Lotte::Conf::ConfigurationReader` over `Euph::Conf::Configuration`.
 */
class ConfigReader {
public:
	/** @brief Virtual destructor for implementations. */
	virtual ~ConfigReader() = default;

	/** @brief True when the section contains @p key at all. */
	virtual bool hasValue(std::string_view key) const = 0;
	/** @brief Reads a bool, or @p fallback when the key is missing. */
	virtual bool getBool(std::string_view key, bool fallback) const = 0;
	/** @brief Reads an integer, or @p fallback when the key is missing. */
	virtual long long getInteger(std::string_view key, long long fallback) const = 0;
	/** @brief Reads a float, or @p fallback when the key is missing. */
	virtual double getFloat(std::string_view key, double fallback) const = 0;
	/** @brief Reads a string, or @p fallback when the key is missing. */
	virtual std::string_view getString(std::string_view key, std::string_view fallback) const = 0;
};

/**
 * @brief A backend's answer to "what would you like, and what may I edit?".
 *
 * Implementations keep a reference to the concrete configuration object (for
 * example the GL layer's `ContextConfig`), so `apply()` writes straight into the
 * object the backend will use.
 */
class ConfigProvider {
public:
	/** @brief Virtual destructor for implementations. */
	virtual ~ConfigProvider() = default;

	/** @brief Backend-neutral defaults the backend recommends for @p purpose. */
	virtual DeviceConfig description(ContextPurpose purpose) const = 0;

	/** @brief Backend-specific fields exposed for @p purpose. */
	virtual std::span<const ConfigField> fields(ContextPurpose purpose) const = 0;

	/**
	 * @brief Reads every exposed field out of @p reader and stores it.
	 *
	 * Implementations clamp numeric values to the range they advertised in
	 * fields() and fall back to the default when a value is unusable.
	 *
	 * @return @c false when a required value could not be interpreted at all.
	 */
	virtual bool apply(const ConfigReader& reader) = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Reading helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Common fields, described once so every backend exposes the same keys.
 *
 * Backends append their own fields after these in `fields()`.
 */
inline std::span<const ConfigField> commonConfigFields() noexcept
{
	static constexpr ConfigField fields[] = {
		{ "iWidth", ConfigValueType::IntegerValue, 1, 16384, "1280", nullptr,
		  "Swapchain/render-target width in pixels." },
		{ "iHeight", ConfigValueType::IntegerValue, 1, 16384, "720", nullptr,
		  "Swapchain/render-target height in pixels." },
		{ "iFramesInFlight", ConfigValueType::IntegerValue, 1, 8, "2", nullptr,
		  "How many frames the backend may keep in flight." },
		{ "sPresentMode", ConfigValueType::StringValue, 0, 0, "fifo", "immediate|fifo|mailbox",
		  "Presentation mode; fifo implies vsync." },
		{ "bVSync", ConfigValueType::BoolValue, 0, 0, "true", nullptr,
		  "Present with vsync." },
		{ "bSrgbFramebuffer", ConfigValueType::BoolValue, 0, 0, "true", nullptr,
		  "Use an sRGB swapchain/render target when available." },
		{ "fMaxAnisotropy", ConfigValueType::FloatValue, 0, 16, "0", nullptr,
		  "Maximum texture anisotropy; 0 disables anisotropic filtering." },
		{ "bGenerateMipmaps", ConfigValueType::BoolValue, 0, 0, "true", nullptr,
		  "Generate mip chains for uploaded textures when the asset has none." }
	};
	return std::span<const ConfigField>(fields, sizeof(fields) / sizeof(fields[0]));
}

/** @brief True when @p value is one of @ref ConfigField::choices (or choices is null). */
inline bool configFieldAllows(const ConfigField& field, std::string_view value) noexcept
{
	if (field.choices == nullptr) {
		return true;
	}
	std::string_view remaining(field.choices);
	while (!remaining.empty()) {
		const std::size_t separator = remaining.find('|');
		if (remaining.substr(0, separator) == value) {
			return true;
		}
		if (separator == std::string_view::npos) {
			break;
		}
		remaining.remove_prefix(separator + 1);
	}
	return false;
}

/** @brief Parses the textual default of a numeric field. */
inline double configFieldDefaultNumber(const ConfigField& field) noexcept
{
	if (field.defaultValue == nullptr) {
		return 0.0;
	}
	switch (field.type) {
		case ConfigValueType::IntegerValue: return static_cast<double>(std::strtoll(field.defaultValue, nullptr, 10));
		case ConfigValueType::FloatValue: return std::strtod(field.defaultValue, nullptr);
		default: return 0.0;
	}
}

/** @brief Reads an integer field, falling back to its default and clamping to its range. */
inline long long readConfigInteger(const ConfigField& field, const ConfigReader& reader) noexcept
{
	const long long fallback = static_cast<long long>(configFieldDefaultNumber(field));
	long long value = reader.getInteger(field.key, fallback);
	if (field.maximum >= field.minimum) {
		const long long minimum = static_cast<long long>(field.minimum);
		const long long maximum = static_cast<long long>(field.maximum);
		if (value < minimum) {
			value = minimum;
		} else if (value > maximum) {
			value = maximum;
		}
	}
	return value;
}

/** @brief Reads a float field, falling back to its default and clamping to its range. */
inline double readConfigFloat(const ConfigField& field, const ConfigReader& reader) noexcept
{
	double value = reader.getFloat(field.key, configFieldDefaultNumber(field));
	if (field.maximum >= field.minimum) {
		if (value < field.minimum) {
			value = field.minimum;
		} else if (value > field.maximum) {
			value = field.maximum;
		}
	}
	return value;
}

/** @brief Reads a bool field, falling back to its textual default ("true"/"1"). */
inline bool readConfigBool(const ConfigField& field, const ConfigReader& reader) noexcept
{
	const std::string_view fallback = field.defaultValue != nullptr ? std::string_view(field.defaultValue)
																   : std::string_view("false");
	const bool defaultBool = fallback == "true" || fallback == "1";
	return reader.getBool(field.key, defaultBool);
}

/** @brief Reads a string field, falling back to the default when @ref ConfigField::choices rejects it. */
inline std::string readConfigString(const ConfigField& field, const ConfigReader& reader)
{
	const std::string_view fallback = field.defaultValue != nullptr ? std::string_view(field.defaultValue)
																   : std::string_view();
	std::string value(reader.getString(field.key, fallback));
	if (!configFieldAllows(field, value)) {
		return std::string(fallback);
	}
	return value;
}

}
#endif // KLDCONFIG_HPP
