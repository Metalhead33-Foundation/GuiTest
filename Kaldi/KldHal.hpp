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
 * @brief Version tag for the first additive extension of the HAL contract.
 *
 * 1.1 adds only optional surface area, so a 1.1 consumer degrades cleanly on a
 * 1.0 backend: explicit swapchain objects, a capability flag for them, and the
 * backend-reserved handle range that reports backend-owned framebuffers back to
 * the caller.
 */
inline constexpr HalVersion KaldiHalVersion1_1 { 1, 1, 0 };

/**
 * @brief Version tag for the second additive extension of the HAL contract.
 *
 * 1.2 adds @ref HalCapabilities::clipSpaceYAxis, which names the direction of the
 * positive y axis in normalized device coordinates. It is additive: a 1.1
 * consumer that ignores it keeps working, because a backend that does not report
 * 1.2 leaves the field at its zero value (`Up`, the OpenGL/Direct3D convention).
 */
inline constexpr HalVersion KaldiHalVersion1_2 { 1, 2, 0 };

/**
 * @brief Direction of the positive y axis in normalized device coordinates.
 *
 * `OpSetViewport` uses the OpenGL convention for the viewport *rectangle*: `y` is
 * measured from the bottom of the framebuffer, and backends translate that to
 * their own window-space origin. The clip-space **y direction** is a different
 * question, and the graphics APIs genuinely disagree:
 *
 * | API | y = +1 in NDC is | Value |
 * |---|---|---|
 * | OpenGL / OpenGL ES | the top of the framebuffer | @ref ClipSpaceYAxis::Up |
 * | Direct3D 11 / 12 | the top of the framebuffer | @ref ClipSpaceYAxis::Up |
 * | Vulkan | the bottom of the framebuffer | @ref ClipSpaceYAxis::Down |
 *
 * A renderer that builds clip-space positions itself (a 2D UI whose vertices are
 * pixels, or a projection matrix) **must** consult this instead of assuming one
 * convention: without it, the same vertex data renders vertically flipped on one
 * of the backends. Vulkan's inverted axis is the reason "one shader for every
 * backend" projects need this flag.
 */
enum class ClipSpaceYAxis : uint8_t {
	/** @brief NDC `y = +1` is the top of the framebuffer (OpenGL, Direct3D). */
	Up,
	/** @brief NDC `y = +1` is the bottom of the framebuffer (Vulkan). */
	Down
};

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
	/**
	 * @brief True when explicit swapchain objects (HAL 1.1) are honoured.
	 *
	 * A false value means `Opcode`::CreateSwapchain answers
	 * `HalStatus`Code::UnsupportedBaseline and callers must stay on the
	 * default-frame path (`BeginFrame`/`EndFrame`/`Present` with
	 * @ref DefaultFramebuffer). A true value additionally promises that
	 * @ref SwapchainAcquireInfo reports a usable framebuffer handle, so an
	 * acquire callback is enough to start drawing into the acquired image.
	 */
	bool supportsExplicitSwapchains;
	/**
	 * @brief Direction of the positive y axis in clip space (HAL 1.2).
	 *
	 * Zero-initialised to @ref ClipSpaceYAxis::Up, the OpenGL/Direct3D
	 * convention, so consumers that never see a probed Vulkan device keep the
	 * pre-1.2 assumption. Vulkan backends report @ref ClipSpaceYAxis::Down; see
	 * @ref ClipSpaceYAxis for why a renderer must not hard-code either value.
	 */
	ClipSpaceYAxis clipSpaceYAxis;
};

/**
 * @brief Conservative frame-in-flight count for APIs that do not expose one.
 *
 * `HalCapabilities`::maxFramesInFlight is a planning number, not always a hard
 * API limit. OpenGL, Direct3D 11 and Vulkan-without-a-swapchain cannot answer
 * "how many frames may I keep in flight" through a query, so their probes report
 * this value, which matches the common double/triple-buffered default of
 * desktop presentation.
 */
inline constexpr uint32_t kDefaultMaxFramesInFlight = 3;

/**
 * @brief Capability snapshot of a device that has not probed a backend API yet.
 *
 * Every limit is zero and every feature flag is false; only @ref
 * HalCapabilities::version is filled in, because the backend still implements the
 * baseline v1.0 contract (a 1.1 backend replaces the version during probing).
 * Backends return this from a detached/uninitialized device, and probes fall back
 * to it when the API objects they need are missing. A zero limit means "not
 * usable", never "unlimited" - callers must not plan work against a snapshot that
 * has not been replaced by a real one.
 */
inline constexpr HalCapabilities halCapabilitiesNone() noexcept
{
	HalCapabilities capabilities{};
	capabilities.version = KaldiHalVersion1_0;
	return capabilities;
}

/** @brief Portable status code returned by backend initialization/submission. */
enum class HalStatusCode : uint8_t {
	/**
	 * @brief Operation completed successfully.
	 *
	 * @note Spelled `Succeeded` rather than `Success` because `<X11/X.h>` defines a
	 * `Success` macro and the backend translation units include Xlib.
	 */
	Succeeded,
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
	constexpr bool ok() const { return code == HalStatusCode::Succeeded; }
};

/** @brief Canonical successful status value. */
inline constexpr HalStatus HalOk { HalStatusCode::Succeeded, nullptr };

}
#endif // KLDHAL_HPP
