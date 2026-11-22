/**
 * @file GlDeviceConfig.cpp
 * @brief Implements GlDeviceConfig.hpp.
 *
 * The context half is delegated to `ContextConfigProvider` (so the existing key
 * set and clamping rules stay in one place); this unit only owns the executor
 * keys that were added on top.
 */
#include "GlDeviceConfig.hpp"
#include "GlContextConfig.hpp"
#include <string_view>
#include <vector>

namespace KALDIGL_NS {

namespace {

/** @brief Executor policy fields, appended after the context fields. */
std::span<const Kld::ConfigField> executorFields() noexcept
{
	static constexpr Kld::ConfigField fields[] = {
		{ "bGlDebugLabels", Kld::ConfigValueType::BoolValue, 0, 0, "true", nullptr,
		  "Forward debug label/marker commands through GL_KHR_debug when available (UT: UseOpenGLDebug)." },
		{ "bGlForceNearestFiltering", Kld::ConfigValueType::BoolValue, 0, 0, "false", nullptr,
		  "Ignore sampler filtering and use nearest-neighbour sampling (UT: NoFiltering)." },
		{ "bGlForceTrilinear", Kld::ConfigValueType::BoolValue, 0, 0, "false", nullptr,
		  "Upgrade mipmapped samplers to trilinear filtering (UT: UseTrilinear)." },
		{ "fGlTextureLodBias", Kld::ConfigValueType::FloatValue, -8, 8, "0", nullptr,
		  "Constant LOD bias applied to every sampled texture (UT: LODBias)." },
		{ "fGlMaxAnisotropy", Kld::ConfigValueType::FloatValue, 0, 16, "0", nullptr,
		  "Upper bound for OpCreateSampler anisotropy; 0 keeps the command value (UT: MaxAnisotropy)." },
		{ "iGlMaxTextureSize", Kld::ConfigValueType::IntegerValue, 0, 32768, "0", nullptr,
		  "Largest accepted texture extent; 0 means no limit (UT: MaxLogTextureSize, absolute here)." }
	};
	return std::span<const Kld::ConfigField>(fields, sizeof(fields) / sizeof(fields[0]));
}

/** @brief Context fields followed by the executor fields, built once. */
const std::vector<Kld::ConfigField>& allFields()
{
	static const std::vector<Kld::ConfigField> fields = [] {
		std::vector<Kld::ConfigField> result;
		// fields() only reads the table, so a scratch target is enough.
		ContextConfig scratch;
		ContextConfigProvider contextProvider(scratch);
		const std::span<const Kld::ConfigField> contextFields =
			contextProvider.fields(Kld::ContextPurpose::MainWindow);
		result.assign(contextFields.begin(), contextFields.end());
		const std::span<const Kld::ConfigField> executor = executorFields();
		result.insert(result.end(), executor.begin(), executor.end());
		return result;
	}();
	return fields;
}

/** @brief Finds a field by key; the table is small and static. */
const Kld::ConfigField& findField(std::string_view key)
{
	for (const Kld::ConfigField& field : allFields()) {
		if (key == field.key) {
			return field;
		}
	}
	// All callers pass literals that exist above; a miss would be a programming error.
	return allFields().front();
}

} // namespace

DeviceConfigProvider::DeviceConfigProvider(DeviceConfig& config) noexcept
	: config_(config)
{
}

Kld::DeviceConfig DeviceConfigProvider::description(Kld::ContextPurpose purpose) const
{
	ContextConfigProvider contextProvider(config_.context);
	Kld::DeviceConfig description = contextProvider.description(purpose);
	description.maxAnisotropy = config_.executor.maxAnisotropy;
	// The command executor always honours the mip chain the command asks for, so
	// report the backend capability rather than a rendering policy.
	description.generateMipmaps = true;
	return description;
}

std::span<const Kld::ConfigField> DeviceConfigProvider::fields(Kld::ContextPurpose /*purpose*/) const
{
	return allFields();
}

bool DeviceConfigProvider::apply(const Kld::ConfigReader& reader)
{
	ContextConfigProvider contextProvider(config_.context);
	if (!contextProvider.apply(reader)) {
		return false;
	}
	config_.executor.debugLabels = Kld::readConfigBool(findField("bGlDebugLabels"), reader);
	config_.executor.forceNearestFiltering = Kld::readConfigBool(findField("bGlForceNearestFiltering"), reader);
	config_.executor.forceTrilinear = Kld::readConfigBool(findField("bGlForceTrilinear"), reader);
	config_.executor.textureLodBias =
		static_cast<float>(Kld::readConfigFloat(findField("fGlTextureLodBias"), reader));
	config_.executor.maxAnisotropy =
		static_cast<float>(Kld::readConfigFloat(findField("fGlMaxAnisotropy"), reader));
	config_.executor.maxTextureSize =
		static_cast<std::uint32_t>(Kld::readConfigInteger(findField("iGlMaxTextureSize"), reader));
	return true;
}

}
