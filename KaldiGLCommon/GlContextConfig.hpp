#ifndef KALDIGLCONTEXTCONFIG_HPP
#define KALDIGLCONTEXTCONFIG_HPP
/**
 * @file GlContextConfig.hpp
 * @brief `Kld::ConfigProvider` for the GL/GLES context layer.
 *
 * This is the concrete half of `Kaldi/KldConfig.hpp`: it exposes the fields of
 * `ContextConfig` (API choice, version, profile, MSAA, buffer sizes, X11 screen)
 * with their valid ranges and defaults, and writes the values a `Kld::ConfigReader`
 * provides straight back into the `ContextConfig` the backend will pass to
 * `createContext()`.
 *
 * Typical flow:
 *
 * @code
 * GL::ContextConfig config = GL::defaultContextConfig();
 * GL::ContextConfigProvider provider(config);
 * // ... write provider.fields() defaults into an .ini file ...
 * // ... next start: provider.apply(reader) clamps what the file said ...
 * std::unique_ptr<GL::Context> context = GL::createContext(handles, config);
 * @endcode
 */
#include "GlContext.hpp"
#include <Kaldi/KldConfig.hpp>
#include <span>
#include <string_view>

namespace KALDIGL_NS {

/**
 * @brief Exposes `ContextConfig` as a Kaldi configuration provider.
 *
 * The provider does not own the configuration it edits: it holds a reference, so
 * the same object can be kept by the backend and handed to createContext().
 */
class ContextConfigProvider final : public Kld::ConfigProvider {
public:
	/**
	 * @brief Creates a provider writing into @p config.
	 * @param config The configuration object to describe and update; must outlive the provider.
	 */
	explicit ContextConfigProvider(ContextConfig& config) noexcept;

	/** @brief The object this provider reads and writes. */
	ContextConfig& target() noexcept { return config_; }
	/** @brief The object this provider reads and writes. */
	const ContextConfig& target() const noexcept { return config_; }

	/**
	 * @brief Recommends backend-neutral defaults for @p purpose.
	 *
	 * Main window: 1280x720 with vsync; offscreen targets: 1024x1024 without
	 * vsync; debug views: 640x360 without vsync or sRGB decoration.
	 */
	Kld::DeviceConfig description(Kld::ContextPurpose purpose) const override;

	/**
	 * @brief Common fields plus the GL-specific ones.
	 *
	 * The list is identical for every purpose; the GL context layer has no
	 * purpose-dependent settings, only purpose-dependent defaults.
	 */
	std::span<const Kld::ConfigField> fields(Kld::ContextPurpose purpose) const override;

	/**
	 * @brief Reads every field and stores it in `target()`, clamped to its range.
	 * @return Always @c true; unknown API names fall back to `ContextApi::Auto`.
	 */
	bool apply(const Kld::ConfigReader& reader) override;

private:
	ContextConfig& config_;
};

/** @brief Lower-case configuration name of a context API ("auto", "egl", "glx", "wgl"). */
std::string_view contextApiConfigName(ContextApi api) noexcept;

/** @brief Maps a configuration name back to `ContextApi`; unknown names give `Auto`. */
ContextApi contextApiFromConfigName(std::string_view name) noexcept;

}

#endif // KALDIGLCONTEXTCONFIG_HPP
