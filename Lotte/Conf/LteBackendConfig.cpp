/**
 * @file LteBackendConfig.cpp
 * @brief Implements LteBackendConfig.hpp.
 */
#include "LteBackendConfig.hpp"
#include <sstream>
#include <stdexcept>

namespace Lotte {
namespace Conf {

namespace {

/** @brief Configuration name of a presentation mode. */
std::string_view presentModeName(Kld::PresentMode mode) noexcept
{
	switch (mode) {
		case Kld::PresentMode::Immediate: return "immediate";
		case Kld::PresentMode::Fifo: return "fifo";
		case Kld::PresentMode::Mailbox: return "mailbox";
	}
	return "fifo";
}

/** @brief Section lookup that never inserts, or @c nullptr when missing. */
Euph::Conf::ConfigurationSection* findSection(Euph::Conf::Configuration& config, std::string_view name)
{
	auto& sections = config.getSections();
	const auto it = sections.find(std::string(name));
	return it == sections.end() ? nullptr : &it->second;
}

} // namespace

ConfigurationReader::ConfigurationReader(Euph::Conf::Configuration& config, std::string_view section)
	: section_(nullptr)
{
	// `getSections()[name]` would insert an empty section; creating it is what we
	// want here, because reading a missing section should yield the defaults.
	auto& sections = config.getSections();
	section_ = &sections[std::string(section)];
}

ConfigurationReader::ConfigurationReader(Euph::Conf::ConfigurationSection& section) noexcept
	: section_(&section)
{
}

bool ConfigurationReader::hasValue(std::string_view key) const
{
	return section_ != nullptr && section_->getContainer().find(std::string(key)) != section_->getContainer().end();
}

bool ConfigurationReader::getBool(std::string_view key, bool fallback) const
{
	return section_ != nullptr ? section_->getBool(key, fallback) : fallback;
}

long long ConfigurationReader::getInteger(std::string_view key, long long fallback) const
{
	return section_ != nullptr ? static_cast<long long>(section_->getInteger(key, static_cast<Euph::Conf::ConfInt>(fallback)))
							   : fallback;
}

double ConfigurationReader::getFloat(std::string_view key, double fallback) const
{
	return section_ != nullptr ? static_cast<double>(section_->getFloat(key, static_cast<Euph::Conf::ConfFloat>(fallback)))
							   : fallback;
}

std::string_view ConfigurationReader::getString(std::string_view key, std::string_view fallback) const
{
	if (section_ == nullptr) {
		return fallback;
	}
	return section_->getString(key, fallback);
}

std::string_view writeBackendDefaults(Euph::Conf::Configuration& config,
									  const Kld::ConfigProvider& provider,
									  Kld::ContextPurpose purpose,
									  std::string_view section)
{
	Euph::Conf::ConfigurationSection& target = config.getSections()[std::string(section)];

	// Common settings are derived from the provider's recommendation, with the
	// field descriptors supplying the key names and the ranges.
	const Kld::DeviceConfig common = provider.description(purpose);
	target.setInt("iWidth", static_cast<Euph::Conf::ConfInt>(common.width));
	target.setInt("iHeight", static_cast<Euph::Conf::ConfInt>(common.height));
	target.setInt("iFramesInFlight", static_cast<Euph::Conf::ConfInt>(common.framesInFlight));
	target.setString("sPresentMode", presentModeName(common.presentMode));
	target.setBool("bVSync", common.vsync ? 1u : 0u);
	target.setBool("bSrgbFramebuffer", common.srgbFramebuffer ? 1u : 0u);
	target.setFloat("fMaxAnisotropy", static_cast<Euph::Conf::ConfFloat>(common.maxAnisotropy));
	target.setBool("bGenerateMipmaps", common.generateMipmaps ? 1u : 0u);

	// Backend-specific fields: write each field's own default, but skip the common
	// keys we just filled in from description().
	for (const Kld::ConfigField& field : provider.fields(purpose)) {
		const std::string key(field.key);
		if (key == "iWidth" || key == "iHeight" || key == "iFramesInFlight" || key == "sPresentMode" ||
			key == "bVSync" || key == "bSrgbFramebuffer" || key == "fMaxAnisotropy" || key == "bGenerateMipmaps") {
			continue;
		}
		const std::string_view value = field.defaultValue != nullptr ? std::string_view(field.defaultValue)
																	 : std::string_view();
		switch (field.type) {
			case Kld::ConfigValueType::BoolValue:
				target.setBool(key, (value == "true" || value == "1") ? 1u : 0u);
				break;
			case Kld::ConfigValueType::IntegerValue:
				target.setInt(key, static_cast<Euph::Conf::ConfInt>(Kld::configFieldDefaultNumber(field)));
				break;
			case Kld::ConfigValueType::FloatValue:
				target.setFloat(key, static_cast<Euph::Conf::ConfFloat>(Kld::configFieldDefaultNumber(field)));
				break;
			case Kld::ConfigValueType::StringValue:
				target.setString(key, value.empty() ? std::string_view("") : value);
				break;
		}
	}
	return section;
}

bool applyBackendConfig(Euph::Conf::Configuration& config,
						Kld::ConfigProvider& provider,
						std::string_view section)
{
	if (findSection(config, section) == nullptr) {
		return false;
	}
	ConfigurationReader reader(config, section);
	return provider.apply(reader);
}

std::string serializeConfiguration(Euph::Conf::Configuration& config)
{
	std::ostringstream stream;
	stream << config;
	return stream.str();
}

Euph::Conf::Configuration parseConfiguration(std::string_view text)
{
	Euph::Conf::Configuration config;
	config.fromString(std::string(text));
	return config;
}

void saveConfiguration(Elv::Io::System& iosys, const char* path, Euph::Conf::Configuration& config)
{
	auto device = iosys.openUnique(path, Elv::Io::Mode::WRITE);
	if (!device) {
		throw std::runtime_error(std::string("Lotte::Conf: cannot write the configuration file '") + path + "'");
	}
	const std::string text = serializeConfiguration(config);
	(void)device->write(std::span<const char>(text.data(), text.size()));
}

Euph::Conf::Configuration loadConfiguration(Elv::Io::System& iosys, const char* path)
{
	Euph::Conf::Configuration config;
	if (!iosys.exists(path)) {
		return config;
	}
	auto device = iosys.openUnique(path, Elv::Io::Mode::READ);
	if (!device) {
		return config;
	}
	const std::vector<char> bytes = device->readAllAsVector<char>();
	config.fromString(std::string(bytes.begin(), bytes.end()));
	return config;
}

}
}
