#ifndef KLDHAL_HPP
#define KLDHAL_HPP
/**
 * @file KldHal.hpp
 * @brief Public Kaldi HAL version, capability, and status contract.
 *
 * Backend-specific discovery belongs behind the backend implementation. Public
 * callers see the normalized `HalCapabilities` snapshot and portable
 * `HalStatus` values, not API-specific extension names, feature structs, or
 * driver error objects.
 */
#include <cstdint>

namespace Kld {

/**
 * @brief Semantic version of the public Kaldi HAL contract.
 *
 * Major versions may change required command semantics. Minor versions add
 * compatible optional or additive surface area. Patch versions clarify the
 * contract without changing the command vocabulary.
 */
struct HalVersion {
	/** @brief Incompatible contract generation. */
	uint16_t major;
	/** @brief Backward-compatible contract extension level. */
	uint16_t minor;
	/** @brief Clarification or bug-fix revision. */
	uint16_t patch;
};

/** @brief Version tag for the first stable Kaldi graphics HAL contract. */
inline constexpr HalVersion KaldiHalVersion1_0 { 1, 0, 0 };

/**
 * @brief Baseline capabilities exposed by a backend after private probing.
 *
 * Backend-specific checks, such as OpenGL extension enumeration or Vulkan
 * feature queries, stay inside backend initialization. This struct is the
 * portable result consumed by higher layers.
 *
 * A conforming v1.0 backend supports the baseline command vocabulary advertised
 * by @ref KaldiHalVersion1_0 or reports `HalStatus`Code::UnsupportedBaseline
 * during initialization. Limits describe what the backend can support portably;
 * higher layers should plan work against these numbers rather than backend
 * extension details.
 */
struct HalCapabilities {
	/** @brief HAL contract version implemented by the backend. */
	HalVersion version;
	/** @brief Maximum supported 1D texture width in texels. */
	uint32_t maxTexture1DSize;
	/** @brief Maximum supported 2D texture width or height in texels. */
	uint32_t maxTexture2DSize;
	/** @brief Maximum supported 3D texture width, height, or depth in texels. */
	uint32_t maxTexture3DSize;
	/** @brief Maximum supported layers in a 2D texture array. */
	uint32_t maxTextureArrayLayers;
	/** @brief Maximum supported cube-map face size in texels. */
	uint32_t maxCubeMapSize;
	/** @brief Maximum supported sample count for multisampled images. */
	uint32_t maxSamples;
	/** @brief Maximum color attachments usable in a framebuffer/render pass. */
	uint32_t maxColorAttachments;
	/** @brief Maximum vertex attributes in a vertex descriptor. */
	uint32_t maxVertexAttributes;
	/** @brief Maximum concurrently bound vertex buffers. */
	uint32_t maxVertexBuffers;
	/** @brief Maximum sampled texture binding slots. */
	uint32_t maxSampledTextureBindings;
	/** @brief Maximum sampler binding slots. */
	uint32_t maxSamplerBindings;
	/** @brief Maximum uniform buffer binding slots. */
	uint32_t maxUniformBufferBindings;
	/** @brief Maximum storage buffer binding slots. */
	uint32_t maxStorageBufferBindings;
	/** @brief Maximum storage image binding slots. */
	uint32_t maxStorageImageBindings;
	/** @brief Maximum frames the backend may keep in flight. */
	uint32_t maxFramesInFlight;
	/** @brief True when compute pipelines and dispatch commands are supported. */
	bool supportsCompute;
	/** @brief True when indirect draw and dispatch commands are supported. */
	bool supportsIndirectDraw;
	/** @brief True when timestamp and occlusion query-pool operations are supported. */
	bool supportsQueries;
	/** @brief True when debug labels and markers are consumed by the backend. */
	bool supportsDebugLabels;
	/** @brief True when compressed texture payload commands are supported. */
	bool supportsTextureCompression;
	/** @brief True when buffer/image/query readback callbacks are supported. */
	bool supportsReadback;
	/** @brief True when storage buffer and storage image bindings are supported. */
	bool supportsStorageResources;
	/** @brief True when multisampled image resolve commands are supported. */
	bool supportsMultisampleResolve;
};

/** @brief Portable status code returned by backend initialization/submission. */
enum class HalStatusCode : uint8_t {
	/** @brief Operation completed successfully. */
	Success,
	/** @brief Backend initialization failed before a full capability snapshot existed. */
	InitializationFailed,
	/** @brief Backend cannot provide the required baseline HAL contract. */
	UnsupportedBaseline,
	/** @brief Command stream contains an invalid or unsupported command. */
	InvalidCommand,
	/** @brief Command stream references an unknown or wrong-kind handle. */
	InvalidHandle,
	/** @brief Command stream uses a resource in an invalid state. */
	InvalidResourceState,
	/** @brief Backend resource allocation failed. */
	AllocationFailed,
	/** @brief Presentation target was lost and must be recreated or resized. */
	SwapchainLost,
	/** @brief Backend shader module creation failed. */
	ShaderCreationFailed,
	/** @brief Backend graphics or compute pipeline creation failed. */
	PipelineCreationFailed,
	/** @brief Backend-specific failure that has no narrower portable code. */
	BackendError
};

/**
 * @brief POD status result for backend-facing API calls.
 *
 * The message pointer is optional and externally owned by the backend. Callers
 * must copy it if they need to retain the text beyond the backend-defined
 * lifetime.
 */
struct HalStatus {
	/** @brief Portable status category. */
	HalStatusCode code;
	/** @brief Optional backend-owned diagnostic text; may be null. */
	const char* message;

	/** @brief Returns true when @ref code is `HalStatus`Code::Success. */
	constexpr bool ok() const { return code == HalStatusCode::Success; }
};

/** @brief Canonical successful status value. */
inline constexpr HalStatus HalOk { HalStatusCode::Success, nullptr };

}
#endif // KLDHAL_HPP
