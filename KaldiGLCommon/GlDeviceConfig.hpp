#ifndef KALDIGLDEVICECONFIG_HPP
#define KALDIGLDEVICECONFIG_HPP
/**
 * @file GlDeviceConfig.hpp
 * @brief Configuration for a GL/GLES backend: context creation plus command execution.
 *
 * Unreal Tournament keeps every renderer knob in one section of `Default.ini`
 * (`[OpenGLDrv.OpenGLRenderDevice]`: `SwapInterval`, `NumAASamples`, `NoFiltering`,
 * `UseTrilinear`, `LODBias`, `MaxAnisotropy`, `UseOpenGLDebug`, `DetailMax`, ...).
 * Kaldi already had the *context* half of that in `ContextConfig`; this header
 * adds the *execution* half so a single `graphics` section configures both.
 *
 * The split is deliberate:
 *
 *  - @ref ContextConfig (unchanged) describes what the platform must create -
 *    version, profile, MSAA, buffer sizes. It is consumed by `createContext()`.
 *  - @ref ExecutorConfig describes how the command executor interprets commands -
 *    debug labels, filtering policy, LOD bias, texture size limit. It is consumed
 *    by `CommandExecutor`.
 *  - @ref DeviceConfig bundles both, and @ref DeviceConfigProvider exposes them
 *    through the same `Kld::ConfigProvider` contract the engine already uses
 *    (`Lotte/Conf/LteBackendConfig.hpp` persists it as `.ini`).
 *
 * @section kaldigl_deviceconfig_ut Unreal Tournament keys and their Kaldi names
 *
 * | UT `Default.ini` | Kaldi key | Effect here |
 * |---|---|---|
 * | `UseOpenGLDebug` | `bGlDebugLabels` | forward debug labels through `GL_KHR_debug` |
 * | `NoFiltering` | `bGlForceNearestFiltering` | override sampler filtering to nearest |
 * | `UseTrilinear` | `bGlForceTrilinear` | upgrade mipmapped samplers to trilinear |
 * | `LODBias` | `fGlTextureLodBias` | `GL_TEXTURE_LOD_BIAS` for every texture |
 * | `MaxAnisotropy` | `fGlMaxAnisotropy` | cap `OpCreateSampler::maxAnisotropy` |
 * | `MaxLogTextureSize` | `iGlMaxTextureSize` | reject oversized textures (absolute, not log2) |
 * | `NumAASamples`, `SwapInterval` | context/present config | context and swap chain |
 *
 * UT's `UseMultiDrawArrays`, `UsePersistentBuffers`, `UseBufferInvalidation`,
 * `UseSSE*`, `BufferTileQuads` and the visual toggles (`Coronas`, `ShinySurfaces`,
 * `VolumetricLighting`, ...) are *renderer features above Kaldi*, not command
 * execution policy: a Quake 3/Unreal-shaped renderer decides them and compiles
 * the result into commands. They therefore have no key here - deliberately.
 */
#include "GlContext.hpp"
#include <Kaldi/KldConfig.hpp>
#include <cstdint>
#include <span>

namespace KALDIGL_NS {

/**
 * @brief How the OpenGL command executor should interpret commands.
 *
 * These are *policy* knobs, not capability switches: an unsupported value is
 * clamped or ignored by the executor instead of failing submission.
 */
struct ExecutorConfig {
	/** @brief Forward `DebugLabelBegin`/`End`/`Marker` through `GL_KHR_debug`. */
	bool debugLabels = true;
	/** @brief Ignore sampler filtering and sample nearest-neighbour (UT `NoFiltering`). */
	bool forceNearestFiltering = false;
	/** @brief Upgrade mipmapped samplers to trilinear filtering (UT `UseTrilinear`). */
	bool forceTrilinear = false;
	/** @brief Constant LOD bias applied to every sampled texture (UT `LODBias`). */
	float textureLodBias = 0.0f;
	/** @brief Maximum anisotropy the executor applies; `0` keeps command values. */
	float maxAnisotropy = 0.0f;
	/** @brief Largest accepted texture extent; `0` means no limit. */
	std::uint32_t maxTextureSize = 0;
};

/** @brief Everything a GL/GLES device needs: platform context plus executor policy. */
struct DeviceConfig {
	/** @brief Requested context version/profile/buffers/MSAA. */
	ContextConfig context;
	/** @brief Command executor policy. */
	ExecutorConfig executor;
};

/**
 * @brief Returns the dialect's default device configuration.
 *
 * The executor defaults are deliberately conservative: debug labels follow the
 * `GL_KHR_debug` availability, filtering and LOD bias are left to the command
 * stream, and no artificial size limit is imposed.
 */
constexpr DeviceConfig defaultDeviceConfig() noexcept
{
	DeviceConfig config;
	config.context = defaultContextConfig();
	return config;
}

/**
 * @brief `Kld::ConfigProvider` describing and applying a @ref DeviceConfig.
 *
 * The field list is the existing context field set (`sGlApi`, `iGlVersionMajor`,
 * ...) followed by the executor keys, so one `.ini` section configures both
 * halves. `apply()` clamps every value through `Kld::readConfig*`.
 */
class DeviceConfigProvider final : public Kld::ConfigProvider {
public:
	/**
	 * @brief Creates a provider writing into @p config.
	 * @param config The object to describe and update; must outlive the provider.
	 */
	explicit DeviceConfigProvider(DeviceConfig& config) noexcept;

	/** @brief The object this provider reads and writes. */
	DeviceConfig& target() noexcept { return config_; }
	/** @brief The object this provider reads and writes. */
	const DeviceConfig& target() const noexcept { return config_; }

	/** @copydoc Kld::ConfigProvider::description */
	Kld::DeviceConfig description(Kld::ContextPurpose purpose) const override;

	/** @copydoc Kld::ConfigProvider::fields */
	std::span<const Kld::ConfigField> fields(Kld::ContextPurpose purpose) const override;

	/** @copydoc Kld::ConfigProvider::apply */
	bool apply(const Kld::ConfigReader& reader) override;

private:
	DeviceConfig& config_;
};

}

#endif // KALDIGLDEVICECONFIG_HPP
