#ifndef LTEBACKENDCONFIG_HPP
#define LTEBACKENDCONFIG_HPP
/**
 * @file LteBackendConfig.hpp
 * @brief Bridges `Kld::ConfigProvider` and `Euph::Conf::Configuration` (`.ini` files).
 *
 * Kaldi describes *what* a backend wants (`Kld::ConfigProvider`), Euphemy owns the
 * text format (`Euph::Conf::Configuration`), and Lotte - which already depends on
 * both - is the glue. This header is the whole persistence story in five
 * functions:
 *
 * @code
 * GL::ContextConfig glConfig = GL::defaultContextConfig();
 * GL::ContextConfigProvider provider(glConfig);
 *
 * // First run: write the defaults so the user has something to edit.
 * Euph::Conf::Configuration ini;
 * Lotte::Conf::writeBackendDefaults(ini, provider, Kld::ContextPurpose::MainWindow);
 * Lotte::Conf::saveConfiguration(iosys, "/config/kaldi.ini", ini);
 *
 * // Later runs: read the file back; apply() clamps every value to its range.
 * Euph::Conf::Configuration loaded = Lotte::Conf::loadConfiguration(iosys, "/config/kaldi.ini");
 * Lotte::Conf::applyBackendConfig(loaded, provider);
 * @endcode
 *
 * The section name defaults to `graphics`; the key names are the provider's own
 * typed keys (`iWidth`, `sGlApi`, ...), so nothing has to be translated.
 */
#include <Euphemy/Config/EuphConfiguration.hpp>
#include <Kaldi/KldConfig.hpp>
#include <Lotte/LotteLib.hpp>
#include <Elvavena/Io/ElvIoSystem.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace Lotte {
namespace Conf {

/**
 * @brief `Kld::ConfigReader` over a `Euph::Conf::ConfigurationSection`.
 *
 * Euphemy's getters take the typed key and return the default when the key is
 * missing, which is exactly the `Kld::ConfigReader` contract; the adapter only
 * forwards.
 */
class MH_LOTTE_API ConfigurationReader final : public Kld::ConfigReader {
public:
	/**
	 * @brief Reads from @p section of @p config.
	 * @param config Configuration owning the section; must outlive the reader.
	 * @param section Section name.
	 */
	ConfigurationReader(Euph::Conf::Configuration& config, std::string_view section);

	/** @brief Reads from an already resolved section. */
	explicit ConfigurationReader(Euph::Conf::ConfigurationSection& section) noexcept;

	/** @copydoc Kld::ConfigReader::hasValue */
	bool hasValue(std::string_view key) const override;
	/** @copydoc Kld::ConfigReader::getBool */
	bool getBool(std::string_view key, bool fallback) const override;
	/** @copydoc Kld::ConfigReader::getInteger */
	long long getInteger(std::string_view key, long long fallback) const override;
	/** @copydoc Kld::ConfigReader::getFloat */
	double getFloat(std::string_view key, double fallback) const override;
	/** @copydoc Kld::ConfigReader::getString */
	std::string_view getString(std::string_view key, std::string_view fallback) const override;

private:
	Euph::Conf::ConfigurationSection* section_;
};

/**
 * @brief Writes every field of @p provider into @p config, using its defaults.
 *
 * Existing values are overwritten, so this is the "write a fresh configuration"
 * step. The section is created when it does not exist yet.
 *
 * @return The section that was written.
 */
MH_LOTTE_API std::string_view writeBackendDefaults(Euph::Conf::Configuration& config,
												   const Kld::ConfigProvider& provider,
												   Kld::ContextPurpose purpose,
												   std::string_view section = "graphics");

/**
 * @brief Reads @p section back through @p provider, which clamps every value.
 * @return `false` when the section does not exist (the provider keeps its defaults).
 */
MH_LOTTE_API bool applyBackendConfig(Euph::Conf::Configuration& config,
									 Kld::ConfigProvider& provider,
									 std::string_view section = "graphics");

/** @brief Serializes @p config into the `.ini` text format. */
MH_LOTTE_API std::string serializeConfiguration(const Euph::Conf::Configuration& config);

/** @brief Parses `.ini` text produced by serializeConfiguration(). */
MH_LOTTE_API Euph::Conf::Configuration parseConfiguration(std::string_view text);

/** @brief Writes @p config to @p path, truncating it. Throws when the file cannot be opened. */
MH_LOTTE_API void saveConfiguration(Elv::Io::System& iosys, const char* path, const Euph::Conf::Configuration& config);

/** @brief Reads @p path; a missing file yields an empty configuration. */
MH_LOTTE_API Euph::Conf::Configuration loadConfiguration(Elv::Io::System& iosys, const char* path);

}
}

#endif // LTEBACKENDCONFIG_HPP
