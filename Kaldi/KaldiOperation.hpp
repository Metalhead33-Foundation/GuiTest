#ifndef KALDIOPERATION_HPP
#define KALDIOPERATION_HPP
/**
 * @file KaldiOperation.hpp
 * @brief Data-only graphics command payloads for the Kaldi VM-style renderer API.
 *
 * This header defines the client/server command vocabulary. Commands are plain
 * aggregates selected by @ref Opcode and carried by @ref GfxOp. Handles are
 * opaque IDs; command payloads never own backend resources or pointed-to data.
 *
 * @section kaldi_command_model Command model
 *
 * Kaldi command buffers are intentionally VM-like: a client records a linear
 * stream of small POD payloads, then submits an externally owned span of
 * @ref GfxOp values to a backend @ref Device. Append order is execution order.
 * The API does not define built-in materials, scene objects, visibility
 * systems, asset databases, or gameplay concepts; higher-level engines compile
 * those decisions down into this command vocabulary.
 *
 * @section kaldi_payload_lifetime Payload and pointer lifetime
 *
 * Many commands point at caller-owned data: upload bytes, decoded image data,
 * compressed mip payloads, shader words, vertex descriptors, binding layouts,
 * framebuffer attachments, swapchain descriptors, render-pass descriptors,
 * debug strings, callbacks, and callback user data. Those pointers must remain
 * valid until the backend has processed the command queue that references them.
 * Backend-provided callback data is valid only for the duration of the callback;
 * callers must copy anything they need after the callback returns.
 *
 * @section kaldi_backend_contract Backend contract
 *
 * Backends consume command spans, validate handles and resource states, report
 * failures through @ref HalStatus, and translate the portable vocabulary onto
 * APIs such as OpenGL 4.x, Direct3D 11, Vulkan, Direct3D 12, or WebGPU. A
 * destroy command is a logical retirement request: physical deletion must be
 * delayed until all previously submitted GPU work that can reference the
 * resource has completed.
 */
#include <cstddef>
#include <cstdint>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <Kaldi/KldVertexFormat.hpp>
/**
 * @namespace Kld
 * @brief Kaldi data-oriented graphics API namespace.
 */
namespace Kld {

/**
 * @brief Pixel/texture/image format used by Kaldi resources.
 */
typedef Euph::Media::Image::Format TextureFormat;
/** @brief Decoded image container used for texture creation. */
typedef Euph::Media::Image::DecodeTarget DecodeTarget;

/**
 * @brief Opaque resource identifier shared by every command resource kind.
 *
 * Handles are allocated on the client side before creation commands are
 * submitted. Backends associate the ID with their private resource object when
 * processing the matching create command. The same numeric namespace is shared
 * by all resource kinds, so callers must preserve the expected kind separately.
 */
typedef uint32_t HandleId;
/**
 * @brief Reserved framebuffer ID for the swapchain/default render target.
 *
 * The v1.0 default framebuffer is backend-owned. It is usable with frame,
 * bind, clear, render, and present commands, but it is not an ordinary image
 * handle and must not be destroyed by clients.
 */
constexpr HandleId DefaultFramebuffer = 0;
/**
 * @brief Reserved swapchain ID for the backend-owned/default presentation target.
 *
 * Explicit swapchains use nonzero IDs. @ref DefaultSwapchain refers to the
 * backend-provided presentation target used by the simple frame path.
 */
constexpr HandleId DefaultSwapchain = 0;

/**
 * @brief Operation selector for @ref GfxOp.
 *
 * Each opcode has a matching payload member in @ref GfxOp::opt, except legacy
 * destroy operations that intentionally reuse @ref OpDestroy.
 *
 * Backend implementers should treat unknown or unsupported opcodes as
 * @ref HalStatusCode::InvalidCommand or @ref HalStatusCode::UnsupportedBaseline,
 * depending on whether the opcode is malformed for the submitted stream or
 * outside the backend's advertised contract.
 */
enum class Opcode : uint16_t {
	CreateBufferObject,
	DestroyBufferObject,
	CreateVertexArrayObject,
	CreateTexture1D,
	CreateTexture2D,
	CreateCompressedTexture2D,
	CreateTexture2DFromDecodeTarget,
	CreateTexture3D,
	DestroyTexture,
	CreateSampler,
	DestroySampler,
	CreateImage2D,
	DestroyImage,
	CreateFramebuffer,
	DestroyFramebuffer,
	BindFramebuffer,
	BeginFrame,
	EndFrame,
	ClearColorAttachment,
	ClearDepthStencilAttachment,
	Present,
	CreatePipeline,
	DestroyPipeline,
	BindPipeline,
	BindTexture,
	BindSampler,
	BindUniformBuffer,
	BindVertexBuffer,
	BindIndexBuffer,
	DrawTriangles,
	DrawIndexedTriangles,
	DrawLines,
	DrawIndexedLines,
	ResizeBufferObject,
	UpdateBufferObject,
	MapBufferObject,
	ResizeTexture2D,
	UpdateTexture2D,
	UpdateCompressedTexture2D,
	BlitTexture2D,
	GenerateTextureMipmaps,
	ResizeImage2D,
	UpdateImage2D,
	BlitImage2D,
	SetViewport,
	SetScissor,
	SetBlendState,
	SetDepthStencilState,
	SetRasterizerState,
	TransitionResource,
	BufferBarrier,
	ImageBarrier,
	CreateFence,
	DestroyFence,
	SignalFence,
	WaitFence,
	CreateComputePipeline,
	DestroyComputePipeline,
	BindComputePipeline,
	DispatchCompute,
	DispatchComputeIndirect,
	DrawIndirect,
	DrawIndexedIndirect,
	CreateQueryPool,
	DestroyQueryPool,
	ResetQueryPool,
	BeginQuery,
	EndQuery,
	WriteTimestamp,
	DebugLabelBegin,
	DebugLabelEnd,
	DebugMarker,
	CopyBuffer,
	CopyBufferToImage2D,
	CopyImage2DToBuffer,
	CopyImage2D,
	ReadBuffer,
	ReadImage2D,
	ReadQueryResults,
	CreateTextureCube,
	UpdateTextureCubeFace,
	CreateTexture2DArray,
	UpdateTexture2DArray,
	CreateMultisampledImage2D,
	ResolveImage2D,
	BindStorageBuffer,
	BindStorageImage,
	DrawTrianglesInstanced,
	DrawIndexedTrianglesInstanced,
	DrawLinesInstanced,
	DrawIndexedLinesInstanced,
	CreateSwapchain,
	DestroySwapchain,
	ResizeSwapchain,
	AcquireSwapchainImage,
	PresentSwapchain,
	BeginRenderPass,
	EndRenderPass
	// More to come later!
};
/**
 * @brief Generic one-ID destroy payload used by legacy resource destroy ops.
 */
struct OpDestroy {
	HandleId id; // Just a dumb id.
};
/** @brief Bitmask flags describing intended buffer usage. */
enum BufferUsage : uint32_t {
	BUFFER_USAGE_TRANSFER_SRC = 0x00000001,
	BUFFER_USAGE_TRANSFER_DST = 0x00000002,
	BUFFER_USAGE_VERTEX = 0x00000004,
	BUFFER_USAGE_INDEX = 0x00000008,
	BUFFER_USAGE_UNIFORM = 0x00000010,
	BUFFER_USAGE_STORAGE = 0x00000020,
	BUFFER_USAGE_INDIRECT = 0x00000040,
	BUFFER_USAGE_READBACK = 0x00000080
};
/**
 * @brief Creates a backend buffer object and optionally uploads initial data.
 */
struct OpCreateBufferObject {
	HandleId id; // Preallocated by the backend
	uint32_t size; // In bytes
	uint32_t policy; // @ref BufferUsage bitmask.
	const void* data; // nullptr if you just want to reserve space
};
/** @brief Resizes an existing buffer object, preserving backend-defined contents where possible. */
struct OpResizeBufferObject {
	HandleId id;
	uint32_t size; // In bytes
};
/** @brief Uploads a byte range into an existing buffer object. */
struct OpUpdateBufferObject {
	HandleId id;
	uint32_t offset; // In bytes
	uint32_t size; // In bytes
	const void* data;
};
/** @brief CPU mapping access mode for @ref OpMapBufferObject callbacks. */
enum class BufferMapAccess : uint8_t {
	ReadOnly,
	WriteOnly,
	ReadWrite
};
/** @brief Callback used by @ref OpMapBufferObject to expose mapped bytes. */
typedef void (*BufferMapCallback)(void* data, uint32_t size, void* userData);
/**
 * @brief Maps a buffer range and invokes a POD function-pointer callback.
 *
 * The caller owns the callback and user-data lifetime until command execution.
 */
struct OpMapBufferObject {
	HandleId id;
	uint32_t offset; // In bytes
	uint32_t size; // In bytes
	BufferMapAccess access;
	BufferMapCallback callback;
	void* userData;
};
/** @brief Creates a vertex array object from a static vertex descriptor. */
struct OpCreateVertexArrayObject {
	HandleId id; // Preallocated by the backend
	const VertexDescriptor* descriptor;
	uint32_t elementCount;
};
/** @brief Creates or reserves a 1D sampled texture. */
struct OpCreateTexture1D {
	HandleId id; // Preallocated by the backend
	TextureFormat format;
	uint32_t width;
	const void* pixels; // nullptr if you just want to reserve space
	// Also, be very careful - because we execute commands asychronously, you must ensure that pointer remains valid until the command is executed
	// Maybe I'll look for a workaround in the future, or force syncing and return promises.
};
/** @brief Creates or reserves a 2D sampled texture. */
struct OpCreateTexture2D {
	HandleId id; // Preallocated by the backend
	TextureFormat format;
	uint8_t mipLevels;
	bool generateMipmaps; // If TRUE, then mipLevels is the number of mipmaps to generate. If FALSE, mipLevels is the number of mipmaps already generated.
	uint16_t width;
	uint16_t height;
	const void* pixels; // nullptr if you just want to reserve space
	// Same problems as with OpCreateTexture1D
};
/** @brief Block-compressed texture layout metadata. */
struct TextureCompressionLayout {
	uint8_t blockWidth;
	uint8_t blockHeight;
	uint8_t bytesPerBlock;
};
/**
 * @brief Externally owned texture mip payload.
 *
 * The data pointer may refer to raw pixels or compressed block data depending
 * on the command that consumes it.
 */
struct TextureMipData {
	uint16_t width;
	uint16_t height;
	uint32_t byteSize;
	const void* data;
};
/** @brief Creates a 2D sampled texture from opaque block-compressed mip data. */
struct OpCreateCompressedTexture2D {
	HandleId id;
	TextureFormat format;
	uint16_t width;
	uint16_t height;
	TextureCompressionLayout layout;
	const TextureMipData* mips;
	uint8_t mipCount;
};
/**
 * @brief Creates a 2D sampled texture from a decoded image target.
 *
 * When generateMipmaps is false, DecodeTarget frames are interpreted as an
 * already-computed mip chain. When true, only frame 0 is used as the source.
 */
struct OpCreateTexture2DFromDecodeTarget {
	HandleId id;
	const DecodeTarget* source;
	uint8_t mipLevels;
	bool generateMipmaps;
};
/** @brief Resizes a 2D sampled texture. */
struct OpResizeTexture2D {
	HandleId id;
	TextureFormat format;
	uint8_t mipLevels;
	bool generateMipmaps;
	uint16_t width;
	uint16_t height;
};
/** @brief Uploads a rectangular region into a 2D sampled texture. */
struct OpUpdateTexture2D {
	HandleId id;
	TextureFormat format;
	uint8_t mipLevel;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	const void* pixels;
};
/** @brief Updates one compressed mip region with opaque block data. */
struct OpUpdateCompressedTexture2D {
	HandleId id;
	TextureFormat format;
	TextureCompressionLayout layout;
	uint8_t mipLevel;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	uint32_t byteSize;
	const void* data;
};
/** @brief Copies a rectangular region between 2D sampled textures. */
struct OpBlitTexture2D {
	HandleId source;
	HandleId destination;
	uint16_t sourceX;
	uint16_t sourceY;
	uint16_t destinationX;
	uint16_t destinationY;
	uint16_t width;
	uint16_t height;
};
/** @brief Requests mipmap generation for a sampled texture. */
struct OpGenerateTextureMipmaps {
	HandleId id;
};
/** @brief Creates or reserves a 3D sampled texture. */
struct OpCreateTexture3D {
	HandleId id; // Preallocated by the backend
	TextureFormat format;
	bool generateMipmaps;
	uint16_t width;
	uint16_t height;
	uint16_t depth;
	const void* pixels; // nullptr if you just want to reserve space
	// Same problems as with OpCreateTexture1D
};
/** @brief Cube texture face selector. */
enum class TextureCubeFace : uint8_t {
	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	PositiveZ,
	NegativeZ
};
/** @brief Creates or reserves a cube sampled texture. */
struct OpCreateTextureCube {
	HandleId id;
	TextureFormat format;
	uint8_t mipLevels;
	bool generateMipmaps;
	uint16_t size;
};
/** @brief Uploads a rectangular region into one cube texture face. */
struct OpUpdateTextureCubeFace {
	HandleId id;
	TextureCubeFace face;
	TextureFormat format;
	uint8_t mipLevel;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	const void* pixels;
};
/** @brief Creates or reserves a 2D array sampled texture. */
struct OpCreateTexture2DArray {
	HandleId id;
	TextureFormat format;
	uint8_t mipLevels;
	bool generateMipmaps;
	uint16_t width;
	uint16_t height;
	uint16_t layers;
	const void* pixels;
};
/** @brief Uploads a rectangular region into one 2D array texture layer. */
struct OpUpdateTexture2DArray {
	HandleId id;
	TextureFormat format;
	uint8_t mipLevel;
	uint16_t layer;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	const void* pixels;
};
/** @brief Texture coordinate wrapping mode. */
enum class SamplerWrapping : uint8_t {
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
	MIRRORED_REPEAT,
	REPEAT
};
/** @brief Texture sampling/filtering mode. */
enum class SamplerFiltering : uint8_t {
	NEAREST,
	LINEAR,
	NEAREST_MIPMAP_NEAREST,
	LINEAR_MIPMAP_NEAREST,
	NEAREST_MIPMAP_LINEAR,
	LINEAR_MIPMAP_LINEAR
};
/** @brief Creates a sampler object. */
struct OpCreateSampler {
	HandleId id; // Preallocated by the backend
	SamplerFiltering filtering;
	SamplerWrapping wrapping[3];
	float maxAnisotropy;
	float maxLod;
	float minLod;
	float lodBias;
};

/** @brief Bitmask flags describing intended image usage. */
enum ImageUsage : uint32_t {
	IMAGE_USAGE_COLOR_ATTACHMENT = 0x01,
	IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x02,
	IMAGE_USAGE_SAMPLED = 0x04,
	IMAGE_USAGE_TRANSFER_SRC = 0x08,
	IMAGE_USAGE_TRANSFER_DST = 0x10,
	IMAGE_USAGE_STORAGE = 0x20,
	IMAGE_USAGE_RESOLVE_SRC = 0x40,
	IMAGE_USAGE_RESOLVE_DST = 0x80
};

/** @brief Creates a 2D image resource for renderable or transfer storage. */
struct OpCreateImage2D {
	HandleId id; // Preallocated by the backend
	TextureFormat format;
	uint16_t width;
	uint16_t height;
	uint8_t mipLevels;
	uint32_t usage;
};
/** @brief Resizes a 2D image resource. */
struct OpResizeImage2D {
	HandleId id;
	TextureFormat format;
	uint16_t width;
	uint16_t height;
	uint8_t mipLevels;
	uint32_t usage;
};
/** @brief Uploads a rectangular region into a 2D image resource. */
struct OpUpdateImage2D {
	HandleId id;
	TextureFormat format;
	uint8_t mipLevel;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	const void* pixels;
};
/** @brief Copies a rectangular region between 2D image resources. */
struct OpBlitImage2D {
	HandleId source;
	HandleId destination;
	uint16_t sourceX;
	uint16_t sourceY;
	uint16_t destinationX;
	uint16_t destinationY;
	uint16_t width;
	uint16_t height;
};
/** @brief Creates a 2D multisampled image resource for rendering or resolve operations. */
struct OpCreateMultisampledImage2D {
	HandleId id;
	TextureFormat format;
	uint16_t width;
	uint16_t height;
	uint8_t samples;
	uint32_t usage;
};
/** @brief Resolves a rectangular region from a multisampled image into a single-sample image. */
struct OpResolveImage2D {
	HandleId source;
	HandleId destination;
	uint16_t sourceX;
	uint16_t sourceY;
	uint16_t destinationX;
	uint16_t destinationY;
	uint16_t width;
	uint16_t height;
};
/** @brief Destroys a 2D image resource. */
struct OpDestroyImage {
	HandleId id;
};

/** @brief Framebuffer attachment role. */
enum class FramebufferAttachmentRole : uint8_t {
	Color,
	Depth,
	Stencil,
	DepthStencil
};

/** @brief Describes one image attachment in a framebuffer. */
struct FramebufferAttachment {
	HandleId image;
	FramebufferAttachmentRole role;
	uint32_t colorAttachmentIndex; // Only meaningful for color attachments.
};

/** @brief Creates a framebuffer from externally owned attachment descriptors. */
struct OpCreateFramebuffer {
	HandleId id; // Preallocated by the backend
	uint16_t width;
	uint16_t height;
	const FramebufferAttachment* attachments;
	uint32_t attachmentCount;
};
/** @brief Destroys a framebuffer. */
struct OpDestroyFramebuffer {
	HandleId id;
};

/** @brief Binds the active framebuffer. */
struct OpBindFramebuffer {
	HandleId id; // Use DefaultFramebuffer for the swapchain/default target.
};

/** @brief Begins work on a frame. */
struct OpBeginFrame {
	HandleId framebuffer; // Usually DefaultFramebuffer.
};

/** @brief Ends work on a frame before presentation. */
struct OpEndFrame {
	HandleId framebuffer; // Usually DefaultFramebuffer.
};

/** @brief Clears one color attachment of a framebuffer. */
struct OpClearColorAttachment {
	HandleId framebuffer;
	uint32_t colorAttachmentIndex;
	float color[4];
};

/** @brief Clears depth and/or stencil attachment data. */
struct OpClearDepthStencilAttachment {
	HandleId framebuffer;
	bool clearDepth;
	bool clearStencil;
	float depth;
	uint32_t stencil;
};

/** @brief Presents a framebuffer, normally @ref DefaultFramebuffer. */
struct OpPresent {
	HandleId framebuffer; // Usually DefaultFramebuffer.
};

/** @brief Presentation pacing policy for an explicit swapchain. */
enum class PresentMode : uint8_t {
	Immediate,
	Fifo,
	Mailbox
};

/** @brief Result state for acquired or presented swapchain images. */
enum class SwapchainStatus : uint8_t {
	Ready,
	Suboptimal,
	Lost,
	Timeout
};

/** @brief Caller-declared image/framebuffer handles for one swapchain image. */
struct SwapchainImageBinding {
	uint32_t imageIndex;
	HandleId colorImage;
	HandleId depthStencilImage;
	HandleId framebuffer;
};

/**
 * @brief Explicit swapchain creation metadata.
 *
 * Explicit swapchains are optional presentation objects for backends and
 * platforms that expose caller-visible swapchain control. The default-frame
 * path remains available through @ref Opcode::BeginFrame,
 * @ref Opcode::EndFrame, @ref Opcode::Present, @ref DefaultFramebuffer, and
 * @ref DefaultSwapchain.
 *
 * The native surface pointer is opaque backend-facing data. It must remain valid
 * until the backend has processed the create command. Optional image bindings
 * let the caller associate swapchain image indices with predeclared image and
 * framebuffer handles; backends that own the images internally may report those
 * handles through @ref SwapchainAcquireInfo when applicable.
 */
struct SwapchainDescriptor {
	void* nativeSurface;
	uint16_t width;
	uint16_t height;
	TextureFormat colorFormat;
	TextureFormat depthStencilFormat;
	PresentMode presentMode;
	uint8_t imageCount;
	const SwapchainImageBinding* imageBindings;
	uint32_t imageBindingCount;
};

/**
 * @brief Data reported when a backend acquires a swapchain image.
 *
 * The callback receives the active image index and any caller-declared image or
 * framebuffer handles associated with that image. The pointed-to acquire info
 * is backend-owned and only valid for the callback duration.
 */
struct SwapchainAcquireInfo {
	HandleId swapchain;
	SwapchainStatus status;
	uint32_t imageIndex;
	HandleId colorImage;
	HandleId depthStencilImage;
	HandleId framebuffer;
};

/**
 * @brief Callback used by @ref OpAcquireSwapchainImage after image acquisition.
 *
 * The callback pointer and user-data pointer must remain valid until the
 * backend has processed the command queue containing the acquire command.
 */
typedef void (*SwapchainAcquireCallback)(const SwapchainAcquireInfo* acquireInfo, void* userData);

/** @brief Creates an explicit swapchain from externally owned metadata. */
struct OpCreateSwapchain {
	HandleId id;
	const SwapchainDescriptor* descriptor;
};

/** @brief Destroys an explicit swapchain. */
struct OpDestroySwapchain {
	HandleId id;
};

/** @brief Resizes an explicit swapchain and optionally replaces image bindings. */
struct OpResizeSwapchain {
	HandleId id;
	uint16_t width;
	uint16_t height;
	const SwapchainImageBinding* imageBindings;
	uint32_t imageBindingCount;
};

/**
 * @brief Acquires a presentable image from a swapchain.
 *
 * A backend reports resize/loss conditions through @ref SwapchainAcquireInfo
 * and may also return @ref HalStatusCode::SwapchainLost from submission when
 * the queue cannot continue without swapchain recreation.
 */
struct OpAcquireSwapchainImage {
	HandleId swapchain;
	SwapchainAcquireCallback callback;
	void* userData;
};

/**
 * @brief Presents the last acquired or explicitly selected swapchain image.
 *
 * For @ref DefaultSwapchain, the image index may be ignored by backends that
 * expose only the simple default presentation path.
 */
struct OpPresentSwapchain {
	HandleId swapchain;
	uint32_t imageIndex;
};

/** @brief Attachment load policy at render-pass begin. */
enum class AttachmentLoadOp : uint8_t {
	Load,
	Clear,
	DontCare
};

/** @brief Attachment store policy at render-pass end. */
enum class AttachmentStoreOp : uint8_t {
	Store,
	DontCare
};

/** @brief One color attachment policy for a render pass. */
struct RenderPassColorAttachment {
	uint32_t colorAttachmentIndex;
	AttachmentLoadOp loadOp;
	AttachmentStoreOp storeOp;
	float clearColor[4];
};

/** @brief Depth/stencil attachment policy for a render pass. */
struct RenderPassDepthStencilAttachment {
	bool enabled;
	AttachmentLoadOp depthLoadOp;
	AttachmentStoreOp depthStoreOp;
	AttachmentLoadOp stencilLoadOp;
	AttachmentStoreOp stencilStoreOp;
	float clearDepth;
	uint32_t clearStencil;
};

/**
 * @brief Externally owned render-pass attachment metadata.
 *
 * Render-pass metadata gives explicit/tile-based backends load and store
 * intent without removing the older immediate-style framebuffer bind and clear
 * commands. The descriptor and attachment arrays must remain valid until the
 * backend processes the command queue.
 */
struct RenderPassDescriptor {
	HandleId framebuffer;
	const RenderPassColorAttachment* colorAttachments;
	uint32_t colorAttachmentCount;
	RenderPassDepthStencilAttachment depthStencilAttachment;
};

/**
 * @brief Begins a render pass and implicitly establishes its framebuffer.
 *
 * Following draw and clear operations execute inside this render pass until
 * @ref OpEndRenderPass. Backends may translate this to native render-pass
 * objects, command-list attachment setup, or immediate framebuffer state.
 */
struct OpBeginRenderPass {
	const RenderPassDescriptor* descriptor;
};

/** @brief Ends the current render pass. */
struct OpEndRenderPass {
	uint8_t reserved;
};

/** @brief SPIR-V shader stage. */
enum class ShaderStage : uint8_t {
	Vertex,
	Fragment,
	Geometry,
	TessControl,
	TessEvaluation,
	Compute
};

/**
 * @brief Externally owned SPIR-V shader binary.
 *
 * Kaldi does not provide built-in shaders. Engines own shader compilation and
 * pass backend-facing SPIR-V word arrays through this descriptor.
 *
 * @warning The @ref spirv pointer must remain valid until the command queue is
 * processed by the backend.
 */
struct ShaderBinaryDescriptor {
	ShaderStage stage;
	const uint32_t* spirv;
	size_t wordCount;
};

/** @brief Sampled texture binding kind. */
enum class TextureBindingType : uint8_t {
	Texture1D,
	Texture2D,
	Texture3D,
	ArrayTexture2D,
	TextureCube
};

/** @brief Shader storage access mode. */
enum class StorageAccess : uint8_t {
	ReadOnly,
	WriteOnly,
	ReadWrite
};

/** @brief Shader-visible sampled texture binding contract for a pipeline. */
struct SampledTextureBindingLayout {
	uint32_t slot;
	TextureBindingType type;
	uint32_t count;
	ShaderStage stages;
};

/** @brief Shader-visible sampler binding contract for a pipeline. */
struct SamplerBindingLayout {
	uint32_t slot;
	uint32_t count;
	ShaderStage stages;
};

/** @brief Shader-visible uniform buffer binding contract for a pipeline. */
struct UniformBufferBindingLayout {
	uint32_t slot;
	uint32_t size;
	ShaderStage stages;
};

/** @brief Shader-visible storage buffer binding contract for a pipeline. */
struct StorageBufferBindingLayout {
	uint32_t slot;
	uint32_t size;
	StorageAccess access;
	ShaderStage stages;
};

/** @brief Shader-visible storage image binding contract for a pipeline. */
struct StorageImageBindingLayout {
	uint32_t slot;
	StorageAccess access;
	ShaderStage stages;
};

/**
 * @brief Optional externally owned shader resource layout metadata.
 *
 * Null pointers and zero counts are valid. Backends may use this metadata to
 * prebuild binding layouts; engines still own all shaders and bindings.
 * Backends that do not require explicit layouts may ignore this metadata after
 * validating that the requested binding counts fit advertised capabilities.
 */
struct PipelineBindingLayout {
	const SampledTextureBindingLayout* sampledTextures;
	uint32_t sampledTextureCount;
	const SamplerBindingLayout* samplers;
	uint32_t samplerCount;
	const UniformBufferBindingLayout* uniformBuffers;
	uint32_t uniformBufferCount;
	const StorageBufferBindingLayout* storageBuffers;
	uint32_t storageBufferCount;
	const StorageImageBindingLayout* storageImages;
	uint32_t storageImageCount;
};

/**
 * @brief Creates a graphics pipeline from engine-supplied SPIR-V binaries.
 *
 * @warning The optional name, shader array, shader words, vertex descriptor,
 * and binding layout pointers are externally owned and must remain valid until
 * the backend has processed the command queue.
 */
struct OpCreatePipeline {
	HandleId id; // Preallocated by the backend
	const char* name; // Optional debug/cache label; nullptr for anonymous pipelines.
	const ShaderBinaryDescriptor* shaderBinaries;
	uint32_t shaderBinaryCount;
	const VertexDescriptor* vertexInput;
	const PipelineBindingLayout* bindingLayout;
};

/** @brief Destroys a graphics pipeline. */
struct OpDestroyPipeline {
	HandleId id;
};

/** @brief Binds the active graphics pipeline. */
struct OpBindPipeline {
	HandleId id;
};

/** @brief Binds a sampled texture to a shader slot. */
struct OpBindTexture {
	uint32_t slot;
	HandleId texture;
	TextureBindingType type;
};

/** @brief Binds a sampler to a shader slot. */
struct OpBindSampler {
	uint32_t slot;
	HandleId sampler;
};

/** @brief Binds a uniform buffer range to a shader slot. */
struct OpBindUniformBuffer {
	uint32_t slot;
	HandleId buffer;
	uint32_t offset;
	uint32_t size;
};
/** @brief Binds a storage buffer range to a shader slot. */
struct OpBindStorageBuffer {
	uint32_t slot;
	HandleId buffer;
	uint32_t offset;
	uint32_t size;
	StorageAccess access;
};
/** @brief Binds a storage image to a shader slot. */
struct OpBindStorageImage {
	uint32_t slot;
	HandleId image;
	uint8_t mipLevel;
	uint16_t layer;
	StorageAccess access;
};

/** @brief Binds a vertex buffer range. */
struct OpBindVertexBuffer {
	uint32_t binding;
	HandleId buffer;
	uint32_t offset;
	uint32_t stride;
	uint32_t instanceDivisor; // 0 for per-vertex input; nonzero for instanced input.
};

/** @brief Index element width for indexed draws. */
enum class IndexType : uint8_t {
	Uint16,
	Uint32
};

/** @brief Binds an index buffer. */
struct OpBindIndexBuffer {
	HandleId buffer;
	uint32_t offset;
	IndexType type;
};

/** @brief Issues an unindexed triangle draw. */
struct OpDrawTriangles {
	uint32_t vertexOffset;
	uint32_t vertexCount;
};

/** @brief Issues an indexed triangle draw. */
struct OpDrawIndexedTriangles {
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t vertexOffset;
};

/** @brief Issues an unindexed line draw. */
struct OpDrawLines {
	uint32_t vertexOffset;
	uint32_t vertexCount;
	float thickness;
};

/** @brief Issues an indexed line draw. */
struct OpDrawIndexedLines {
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t vertexOffset;
	float thickness;
};
/** @brief Issues an instanced unindexed triangle draw. */
struct OpDrawTrianglesInstanced {
	uint32_t vertexOffset;
	uint32_t vertexCount;
	uint32_t instanceCount;
	uint32_t firstInstance;
};
/** @brief Issues an instanced indexed triangle draw. */
struct OpDrawIndexedTrianglesInstanced {
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t vertexOffset;
	uint32_t instanceCount;
	uint32_t firstInstance;
};
/** @brief Issues an instanced unindexed line draw. */
struct OpDrawLinesInstanced {
	uint32_t vertexOffset;
	uint32_t vertexCount;
	float thickness;
	uint32_t instanceCount;
	uint32_t firstInstance;
};
/** @brief Issues an instanced indexed line draw. */
struct OpDrawIndexedLinesInstanced {
	uint32_t indexOffset;
	uint32_t indexCount;
	uint32_t vertexOffset;
	float thickness;
	uint32_t instanceCount;
	uint32_t firstInstance;
};

/** @brief Sets the viewport rectangle and depth range. */
struct OpSetViewport {
	float x;
	float y;
	float width;
	float height;
	float minDepth;
	float maxDepth;
};

/** @brief Sets the scissor rectangle. */
struct OpSetScissor {
	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;
};

/** @brief Blend factor used by @ref OpSetBlendState. */
enum class BlendFactor : uint8_t {
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha
};

/** @brief Blend arithmetic operation. */
enum class BlendOp : uint8_t {
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
};

/** @brief Bitmask selecting writable color channels. */
enum ColorWriteMask : uint8_t {
	COLOR_WRITE_RED = 0x01,
	COLOR_WRITE_GREEN = 0x02,
	COLOR_WRITE_BLUE = 0x04,
	COLOR_WRITE_ALPHA = 0x08,
	COLOR_WRITE_ALL = COLOR_WRITE_RED | COLOR_WRITE_GREEN | COLOR_WRITE_BLUE | COLOR_WRITE_ALPHA
};

/** @brief Sets color blend state for subsequent draws. */
struct OpSetBlendState {
	bool enabled;
	BlendFactor srcColor;
	BlendFactor dstColor;
	BlendOp colorOp;
	BlendFactor srcAlpha;
	BlendFactor dstAlpha;
	BlendOp alphaOp;
	uint8_t colorWriteMask;
};

/** @brief Comparison operation used by depth/stencil tests. */
enum class CompareOp : uint8_t {
	Never,
	Less,
	Equal,
	LessOrEqual,
	Greater,
	NotEqual,
	GreaterOrEqual,
	Always
};

/** @brief Stencil buffer operation. */
enum class StencilOp : uint8_t {
	Keep,
	Zero,
	Replace,
	IncrementClamp,
	DecrementClamp,
	Invert,
	IncrementWrap,
	DecrementWrap
};

/** @brief Stencil state for one face orientation. */
struct StencilFaceState {
	StencilOp failOp;
	StencilOp passOp;
	StencilOp depthFailOp;
	CompareOp compareOp;
	uint32_t compareMask;
	uint32_t writeMask;
	uint32_t reference;
};

/** @brief Sets depth and stencil state for subsequent draws. */
struct OpSetDepthStencilState {
	bool depthTestEnabled;
	bool depthWriteEnabled;
	CompareOp depthCompare;
	bool stencilTestEnabled;
	StencilFaceState front;
	StencilFaceState back;
};

/** @brief Primitive culling mode. */
enum class CullMode : uint8_t {
	None,
	Front,
	Back,
	FrontAndBack
};

/** @brief Front-face winding convention. */
enum class FrontFace : uint8_t {
	CounterClockwise,
	Clockwise
};

/** @brief Polygon rasterization mode. */
enum class PolygonMode : uint8_t {
	Fill,
	Line,
	Point
};

/** @brief Sets rasterization state for subsequent draws. */
struct OpSetRasterizerState {
	CullMode cullMode;
	FrontFace frontFace;
	PolygonMode polygonMode;
	bool depthClampEnabled;
	bool depthBiasEnabled;
	float depthBiasConstantFactor;
	float depthBiasClamp;
	float depthBiasSlopeFactor;
};

/** @brief Resource class used by transition commands. */
enum class ResourceKind : uint8_t {
	Buffer,
	Texture,
	Image,
	Framebuffer
};

/** @brief Coarse resource state for explicit transitions. */
enum class ResourceState : uint8_t {
	Undefined,
	CopySource,
	CopyDestination,
	VertexBuffer,
	IndexBuffer,
	UniformBuffer,
	ShaderRead,
	ShaderWrite,
	ShaderReadWrite,
	ColorAttachment,
	DepthStencilAttachment,
	Present
};

/** @brief Bitmask of coarse pipeline stages for barriers. */
enum PipelineStageFlags : uint32_t {
	PIPELINE_STAGE_TOP_OF_PIPE = 0x00000001,
	PIPELINE_STAGE_TRANSFER = 0x00000002,
	PIPELINE_STAGE_VERTEX_INPUT = 0x00000004,
	PIPELINE_STAGE_VERTEX_SHADER = 0x00000008,
	PIPELINE_STAGE_FRAGMENT_SHADER = 0x00000010,
	PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT = 0x00000020,
	PIPELINE_STAGE_EARLY_FRAGMENT_TESTS = 0x00000040,
	PIPELINE_STAGE_LATE_FRAGMENT_TESTS = 0x00000080,
	PIPELINE_STAGE_COMPUTE_SHADER = 0x00000100,
	PIPELINE_STAGE_BOTTOM_OF_PIPE = 0x00000200
};

/** @brief Bitmask of coarse resource access modes for barriers. */
enum AccessFlags : uint32_t {
	ACCESS_NONE = 0x00000000,
	ACCESS_TRANSFER_READ = 0x00000001,
	ACCESS_TRANSFER_WRITE = 0x00000002,
	ACCESS_VERTEX_ATTRIBUTE_READ = 0x00000004,
	ACCESS_INDEX_READ = 0x00000008,
	ACCESS_UNIFORM_READ = 0x00000010,
	ACCESS_SHADER_READ = 0x00000020,
	ACCESS_SHADER_WRITE = 0x00000040,
	ACCESS_COLOR_ATTACHMENT_READ = 0x00000080,
	ACCESS_COLOR_ATTACHMENT_WRITE = 0x00000100,
	ACCESS_DEPTH_STENCIL_READ = 0x00000200,
	ACCESS_DEPTH_STENCIL_WRITE = 0x00000400,
	ACCESS_HOST_READ = 0x00000800,
	ACCESS_HOST_WRITE = 0x00001000
};

/** @brief Transitions a whole resource between coarse states. */
struct OpTransitionResource {
	HandleId id;
	ResourceKind kind;
	ResourceState oldState;
	ResourceState newState;
	uint32_t sourceStages;
	uint32_t destinationStages;
};

/** @brief Defines memory visibility/order for a buffer range. */
struct OpBufferBarrier {
	HandleId buffer;
	uint32_t offset;
	uint32_t size;
	uint32_t sourceStages;
	uint32_t destinationStages;
	uint32_t sourceAccess;
	uint32_t destinationAccess;
};

/** @brief Defines memory visibility/order for an image subresource range. */
struct OpImageBarrier {
	HandleId image;
	uint8_t baseMipLevel;
	uint8_t mipLevelCount;
	uint16_t baseLayer;
	uint16_t layerCount;
	ResourceState oldState;
	ResourceState newState;
	uint32_t sourceStages;
	uint32_t destinationStages;
	uint32_t sourceAccess;
	uint32_t destinationAccess;
};

/** @brief Creates a timeline-style fence. */
struct OpCreateFence {
	HandleId id;
	uint64_t initialValue;
};

/** @brief Destroys a fence. */
struct OpDestroyFence {
	HandleId id;
};

/** @brief Signals a fence value. */
struct OpSignalFence {
	HandleId id;
	uint64_t value;
};

/** @brief Waits for a fence value. */
struct OpWaitFence {
	HandleId id;
	uint64_t value;
};

/**
 * @brief Creates a compute pipeline from an engine-supplied SPIR-V shader.
 *
 * @warning The optional name, shader descriptor, shader words, and binding
 * layout pointers are externally owned and must remain valid until the backend
 * has processed the command queue.
 */
struct OpCreateComputePipeline {
	HandleId id;
	const char* name; // Optional debug/cache label; nullptr for anonymous pipelines.
	const ShaderBinaryDescriptor* shader;
	const PipelineBindingLayout* bindingLayout;
};

/** @brief Destroys a compute pipeline. */
struct OpDestroyComputePipeline {
	HandleId id;
};

/** @brief Binds the active compute pipeline. */
struct OpBindComputePipeline {
	HandleId id;
};

/** @brief Dispatches compute workgroups directly. */
struct OpDispatchCompute {
	uint32_t groupCountX;
	uint32_t groupCountY;
	uint32_t groupCountZ;
};

/** @brief Dispatches compute workgroups from an indirect buffer. */
struct OpDispatchComputeIndirect {
	HandleId buffer;
	uint32_t offset;
};

/** @brief Issues non-indexed indirect draw commands from a buffer. */
struct OpDrawIndirect {
	HandleId buffer;
	uint32_t offset;
	uint32_t drawCount;
	uint32_t stride;
};

/** @brief Issues indexed indirect draw commands from a buffer. */
struct OpDrawIndexedIndirect {
	HandleId buffer;
	uint32_t offset;
	uint32_t drawCount;
	uint32_t stride;
};

/** @brief Query pool purpose. */
enum class QueryType : uint8_t {
	/**
	 * @brief GPU timestamp query.
	 *
	 * Timestamp units, availability, and conversion to seconds are backend
	 * capability details. Portable callers should use timestamp values for
	 * relative comparisons only when the backend contract says they are valid.
	 */
	Timestamp,
	/**
	 * @brief Portable binary occlusion query.
	 *
	 * Kaldi's common-denominator occlusion contract is zero/nonzero visibility:
	 * a result of 0 means no samples passed the active per-fragment tests, and
	 * any nonzero result means at least one sample passed. Backends that can
	 * cheaply expose an exact sample count may return that count, but portable
	 * callers must not depend on exact counts unless a future precise query type
	 * or capability explicitly promises them.
	 */
	Occlusion
};

/**
 * @brief Creates a pool of timestamp or occlusion queries.
 *
 * Query pools are homogeneous: every query slot in the pool has the same
 * @ref QueryType. Backends should reject out-of-range query indices during
 * submission with @ref HalStatusCode::InvalidCommand or
 * @ref HalStatusCode::InvalidHandle as appropriate.
 */
struct OpCreateQueryPool {
	HandleId id;
	QueryType type;
	uint32_t queryCount;
};

/** @brief Destroys a query pool. */
struct OpDestroyQueryPool {
	HandleId id;
};

/** @brief Resets a range of queries in a query pool. */
struct OpResetQueryPool {
	HandleId id;
	uint32_t firstQuery;
	uint32_t queryCount;
};

/**
 * @brief Begins an interval query such as occlusion.
 *
 * Intended for query types that cover a range of subsequent commands. Timestamp
 * queries are written with @ref OpWriteTimestamp instead.
 */
struct OpBeginQuery {
	HandleId id;
	uint32_t query;
};

/**
 * @brief Ends an interval query such as occlusion.
 *
 * The query index should match an active query previously opened by
 * @ref OpBeginQuery in the same submitted command stream.
 */
struct OpEndQuery {
	HandleId id;
	uint32_t query;
};

/**
 * @brief Writes a timestamp query at a coarse pipeline stage.
 *
 * The stage mask uses @ref PipelineStageFlags and gives explicit backends a
 * coarse point in the pipeline at which to record the timestamp.
 */
struct OpWriteTimestamp {
	HandleId id;
	uint32_t query;
	uint32_t pipelineStage;
};

/** @brief Callback used by @ref OpReadBuffer to expose readback bytes. */
typedef void (*BufferReadbackCallback)(const void* data, uint32_t size, void* userData);
/** @brief Callback used by @ref OpReadImage2D to expose readback pixels. */
typedef void (*ImageReadbackCallback)(const void* data, uint32_t size, uint16_t width, uint16_t height, uint32_t rowPitch, void* userData);
/** @brief Callback used by @ref OpReadQueryResults to expose query values. */
typedef void (*QueryReadbackCallback)(const uint64_t* values, uint32_t count, void* userData);

/** @brief Copies a byte range between buffers. */
struct OpCopyBuffer {
	HandleId source;
	HandleId destination;
	uint32_t sourceOffset;
	uint32_t destinationOffset;
	uint32_t size;
};
/** @brief Copies buffer bytes into a 2D image subresource. */
struct OpCopyBufferToImage2D {
	HandleId sourceBuffer;
	HandleId destinationImage;
	uint32_t bufferOffset;
	uint32_t bufferRowPitch;
	uint8_t mipLevel;
	uint16_t layer;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
};
/** @brief Copies a 2D image subresource into a buffer. */
struct OpCopyImage2DToBuffer {
	HandleId sourceImage;
	HandleId destinationBuffer;
	uint32_t bufferOffset;
	uint32_t bufferRowPitch;
	uint8_t mipLevel;
	uint16_t layer;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
};
/** @brief Copies a rectangular region between 2D image subresources. */
struct OpCopyImage2D {
	HandleId sourceImage;
	HandleId destinationImage;
	uint8_t sourceMipLevel;
	uint16_t sourceLayer;
	uint8_t destinationMipLevel;
	uint16_t destinationLayer;
	uint16_t sourceX;
	uint16_t sourceY;
	uint16_t destinationX;
	uint16_t destinationY;
	uint16_t width;
	uint16_t height;
};
/** @brief Reads a buffer range and invokes a POD function-pointer callback. */
struct OpReadBuffer {
	HandleId buffer;
	uint32_t offset;
	uint32_t size;
	BufferReadbackCallback callback;
	void* userData;
};
/** @brief Reads image pixels and invokes a POD function-pointer callback. */
struct OpReadImage2D {
	HandleId image;
	uint8_t mipLevel;
	uint16_t layer;
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	ImageReadbackCallback callback;
	void* userData;
};
/**
 * @brief Reads query results and invokes a POD function-pointer callback.
 *
 * For @ref QueryType::Occlusion, callers should interpret each returned value
 * portably as binary visibility: 0 is fully occluded, nonzero is visible.
 */
struct OpReadQueryResults {
	HandleId queryPool;
	uint32_t firstQuery;
	uint32_t queryCount;
	QueryReadbackCallback callback;
	void* userData;
};

/** @brief Begins a named debug label region. */
struct OpDebugLabelBegin {
	const char* name;
	float color[4];
};

/** @brief Ends the most recent debug label region. */
struct OpDebugLabelEnd {
	uint8_t reserved;
};

/** @brief Emits a point debug marker. */
struct OpDebugMarker {
	const char* name;
	float color[4];
};

/**
 * @brief Tagged command payload.
 *
 * The active union member is selected by @ref opcode. All payloads are public
 * aggregates so command buffers can be built without allocation or virtual
 * dispatch.
 */
struct GfxOp {
	Opcode opcode;
	union {
		OpDestroy opDestroy;
		OpCreateBufferObject opCreateBufferObject;
		OpResizeBufferObject opResizeBufferObject;
		OpUpdateBufferObject opUpdateBufferObject;
		OpMapBufferObject opMapBufferObject;
		OpCreateVertexArrayObject opCreateVertexArrayObject;
		OpCreateTexture1D opCreateTexture1D;
		OpCreateTexture2D opCreateTexture2D;
		OpCreateCompressedTexture2D opCreateCompressedTexture2D;
		OpCreateTexture2DFromDecodeTarget opCreateTexture2DFromDecodeTarget;
		OpCreateTextureCube opCreateTextureCube;
		OpUpdateTextureCubeFace opUpdateTextureCubeFace;
		OpCreateTexture2DArray opCreateTexture2DArray;
		OpUpdateTexture2DArray opUpdateTexture2DArray;
		OpResizeTexture2D opResizeTexture2D;
		OpUpdateTexture2D opUpdateTexture2D;
		OpUpdateCompressedTexture2D opUpdateCompressedTexture2D;
		OpBlitTexture2D opBlitTexture2D;
		OpGenerateTextureMipmaps opGenerateTextureMipmaps;
		OpCreateTexture3D opCreateTexture3D;
		OpCreateSampler opCreateSampler;
		OpCreateImage2D opCreateImage2D;
		OpResizeImage2D opResizeImage2D;
		OpUpdateImage2D opUpdateImage2D;
		OpBlitImage2D opBlitImage2D;
		OpCreateMultisampledImage2D opCreateMultisampledImage2D;
		OpResolveImage2D opResolveImage2D;
		OpDestroyImage opDestroyImage;
		OpCreateFramebuffer opCreateFramebuffer;
		OpDestroyFramebuffer opDestroyFramebuffer;
		OpBindFramebuffer opBindFramebuffer;
		OpBeginFrame opBeginFrame;
		OpEndFrame opEndFrame;
		OpClearColorAttachment opClearColorAttachment;
		OpClearDepthStencilAttachment opClearDepthStencilAttachment;
		OpPresent opPresent;
		OpCreatePipeline opCreatePipeline;
		OpDestroyPipeline opDestroyPipeline;
		OpBindPipeline opBindPipeline;
		OpBindTexture opBindTexture;
		OpBindSampler opBindSampler;
		OpBindUniformBuffer opBindUniformBuffer;
		OpBindStorageBuffer opBindStorageBuffer;
		OpBindStorageImage opBindStorageImage;
		OpBindVertexBuffer opBindVertexBuffer;
		OpBindIndexBuffer opBindIndexBuffer;
		OpDrawTriangles opDrawTriangles;
		OpDrawIndexedTriangles opDrawIndexedTriangles;
		OpDrawLines opDrawLines;
		OpDrawIndexedLines opDrawIndexedLines;
		OpDrawTrianglesInstanced opDrawTrianglesInstanced;
		OpDrawIndexedTrianglesInstanced opDrawIndexedTrianglesInstanced;
		OpDrawLinesInstanced opDrawLinesInstanced;
		OpDrawIndexedLinesInstanced opDrawIndexedLinesInstanced;
		OpSetViewport opSetViewport;
		OpSetScissor opSetScissor;
		OpSetBlendState opSetBlendState;
		OpSetDepthStencilState opSetDepthStencilState;
		OpSetRasterizerState opSetRasterizerState;
		OpTransitionResource opTransitionResource;
		OpBufferBarrier opBufferBarrier;
		OpImageBarrier opImageBarrier;
		OpCreateFence opCreateFence;
		OpDestroyFence opDestroyFence;
		OpSignalFence opSignalFence;
		OpWaitFence opWaitFence;
		OpCreateComputePipeline opCreateComputePipeline;
		OpDestroyComputePipeline opDestroyComputePipeline;
		OpBindComputePipeline opBindComputePipeline;
		OpDispatchCompute opDispatchCompute;
		OpDispatchComputeIndirect opDispatchComputeIndirect;
		OpDrawIndirect opDrawIndirect;
		OpDrawIndexedIndirect opDrawIndexedIndirect;
		OpCreateQueryPool opCreateQueryPool;
		OpDestroyQueryPool opDestroyQueryPool;
		OpResetQueryPool opResetQueryPool;
		OpBeginQuery opBeginQuery;
		OpEndQuery opEndQuery;
		OpWriteTimestamp opWriteTimestamp;
		OpCopyBuffer opCopyBuffer;
		OpCopyBufferToImage2D opCopyBufferToImage2D;
		OpCopyImage2DToBuffer opCopyImage2DToBuffer;
		OpCopyImage2D opCopyImage2D;
		OpReadBuffer opReadBuffer;
		OpReadImage2D opReadImage2D;
		OpReadQueryResults opReadQueryResults;
		OpDebugLabelBegin opDebugLabelBegin;
		OpDebugLabelEnd opDebugLabelEnd;
		OpDebugMarker opDebugMarker;
		OpCreateSwapchain opCreateSwapchain;
		OpDestroySwapchain opDestroySwapchain;
		OpResizeSwapchain opResizeSwapchain;
		OpAcquireSwapchainImage opAcquireSwapchainImage;
		OpPresentSwapchain opPresentSwapchain;
		OpBeginRenderPass opBeginRenderPass;
		OpEndRenderPass opEndRenderPass;
		// More to come later!
	} opt;
};

}
#endif // KALDIOPERATION_HPP
