#ifndef KLDCOMMANDBUFFER_HPP
#define KLDCOMMANDBUFFER_HPP
/**
 * @file KldCommandBuffer.hpp
 * @brief Header-only object-oriented facade over Kaldi VM command buffers.
 *
 * This facade is a convenience layer over @ref GfxOp. It does not add a new
 * backend contract: every helper simply appends one raw command payload to
 * client-owned storage. Resource proxy classes are move-only handles that record
 * typed destroy commands into their owning @ref CommandBuffer when destroyed.
 *
 * @section kaldi_facade_lifetime Facade lifetime rules
 *
 * The facade does not take ownership of pointer payloads passed to command
 * helpers. Callers must keep upload data, descriptors, shader words, callbacks,
 * callback user data, debug strings, swapchain metadata, and render-pass
 * metadata alive until the backend has processed the command queue.
 *
 * @section kaldi_facade_threading Threading model
 *
 * @ref CommandBuffer is single-threaded. Recording, clearing, submitting, and
 * RAII proxy destruction all mutate or inspect the same command storage. Use
 * one command buffer per recording thread, optionally backed by a shared
 * @ref HandleAllocator, then merge streams with @ref mergeCommandBuffers in an
 * explicit order.
 */
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldDevice.hpp>
#include <atomic>
#include <memory_resource>
#include <span>
#include <utility>
#include <vector>

namespace Kld {

class CommandBuffer;

/** @brief Move-only RAII proxy for a buffer handle. */
class Buffer;
/** @brief Move-only RAII proxy for a sampled texture handle. */
class Texture;
/** @brief Move-only RAII proxy for an image handle. */
class Image;
/** @brief Move-only RAII proxy for a sampler handle. */
class Sampler;
/** @brief Move-only RAII proxy for a framebuffer handle. */
class Framebuffer;
/** @brief Move-only RAII proxy for a graphics pipeline handle. */
class Pipeline;
/** @brief Move-only RAII proxy for a compute pipeline handle. */
class ComputePipeline;
/** @brief Move-only RAII proxy for a fence handle. */
class Fence;
/** @brief Move-only RAII proxy for a query pool handle. */
class QueryPool;

/**
 * @brief Thread-safe monotonic handle allocator for coordinated recording.
 *
 * Use this when multiple worker command buffers need disjoint handle IDs before
 * their command streams are merged. ID allocation order has no execution
 * meaning; only command append/merge order defines execution order.
 */
class HandleAllocator {
	std::atomic<HandleId> nextId;
public:
	/** @brief Creates an allocator that returns IDs starting at firstId. */
	inline explicit HandleAllocator(HandleId firstId = 1) : nextId(firstId) {}
	/** @brief Allocates one unique handle ID. */
	inline HandleId allocate() { return nextId.fetch_add(1, std::memory_order_relaxed); }
};

/** @brief Resource kind used by deferred destruction helpers. */
enum class ResourceHandleKind : uint8_t {
	Buffer,
	Texture,
	Sampler,
	Image,
	Framebuffer,
	Pipeline,
	ComputePipeline,
	Fence,
	QueryPool,
	Swapchain
};

/**
 * @brief One deferred resource retirement request.
 *
 * Useful when ownership crosses threads or when a worker command buffer has
 * already been merged/cleared and should no longer receive RAII destroy
 * commands directly.
 */
struct DeferredDestroy {
	ResourceHandleKind kind;
	HandleId id;
};

/**
 * @brief Owning buffer proxy.
 *
 * Destroying an owning instance appends @ref Opcode::DestroyBufferObject to the
 * associated @ref CommandBuffer. Call @ref release to suppress RAII destruction.
 * Destruction is recording, so destroy proxies on the command buffer owner
 * thread or move the handle into an explicit deferred-destroy path.
 */
class Buffer {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline Buffer() = default;
	inline Buffer(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;
	inline Buffer(Buffer&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline Buffer& operator=(Buffer&& other) noexcept;
	inline ~Buffer();
	/** @brief Returns the wrapped handle ID. */
	inline HandleId id() const { return resourceId; }
	/** @brief Releases ownership and returns the handle ID without enqueuing destroy. */
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning sampled texture proxy with destructor-enqueued destruction. */
class Texture {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline Texture() = default;
	inline Texture(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	inline Texture(Texture&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline Texture& operator=(Texture&& other) noexcept;
	inline ~Texture();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning image proxy with destructor-enqueued destruction. */
class Image {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline Image() = default;
	inline Image(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;
	inline Image(Image&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline Image& operator=(Image&& other) noexcept;
	inline ~Image();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning sampler proxy with destructor-enqueued destruction. */
class Sampler {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline Sampler() = default;
	inline Sampler(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	Sampler(const Sampler&) = delete;
	Sampler& operator=(const Sampler&) = delete;
	inline Sampler(Sampler&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline Sampler& operator=(Sampler&& other) noexcept;
	inline ~Sampler();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning framebuffer proxy with destructor-enqueued destruction. */
class Framebuffer {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline Framebuffer() = default;
	inline Framebuffer(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	Framebuffer(const Framebuffer&) = delete;
	Framebuffer& operator=(const Framebuffer&) = delete;
	inline Framebuffer(Framebuffer&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline Framebuffer& operator=(Framebuffer&& other) noexcept;
	inline ~Framebuffer();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning graphics pipeline proxy with destructor-enqueued destruction. */
class Pipeline {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline Pipeline() = default;
	inline Pipeline(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;
	inline Pipeline(Pipeline&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline Pipeline& operator=(Pipeline&& other) noexcept;
	inline ~Pipeline();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning compute pipeline proxy with destructor-enqueued destruction. */
class ComputePipeline {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline ComputePipeline() = default;
	inline ComputePipeline(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	ComputePipeline(const ComputePipeline&) = delete;
	ComputePipeline& operator=(const ComputePipeline&) = delete;
	inline ComputePipeline(ComputePipeline&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline ComputePipeline& operator=(ComputePipeline&& other) noexcept;
	inline ~ComputePipeline();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning fence proxy with destructor-enqueued destruction. */
class Fence {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline Fence() = default;
	inline Fence(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	Fence(const Fence&) = delete;
	Fence& operator=(const Fence&) = delete;
	inline Fence(Fence&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline Fence& operator=(Fence&& other) noexcept;
	inline ~Fence();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/** @brief Owning query pool proxy with destructor-enqueued destruction. */
class QueryPool {
	HandleId resourceId{};
	CommandBuffer* owner{};
public:
	inline QueryPool() = default;
	inline QueryPool(HandleId id, CommandBuffer* commandBuffer) : resourceId(id), owner(commandBuffer) {}
	QueryPool(const QueryPool&) = delete;
	QueryPool& operator=(const QueryPool&) = delete;
	inline QueryPool(QueryPool&& other) noexcept : resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
	inline QueryPool& operator=(QueryPool&& other) noexcept;
	inline ~QueryPool();
	inline HandleId id() const { return resourceId; }
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

/**
 * @brief Client-owned command storage and convenience command emitter.
 *
 * The command buffer owns `std::pmr::vector<GfxOp>` storage and allocates local
 * monotonically increasing resource IDs starting at `1`. It does not execute
 * commands itself; use @ref submit to pass a span to a @ref Device.
 *
 * All helpers append immediately. They do not validate backend limits, resource
 * states, or pointer lifetimes; that remains the responsibility of higher-level
 * engine code and the backend's submission validation.
 *
 * @note This class is not thread-safe. Concurrent recording requires separate
 * command buffers per thread or external synchronization around all access.
 */
class CommandBuffer {
	std::pmr::vector<GfxOp> commandStorage;
	HandleId nextId{1};
	HandleAllocator* sharedAllocator{};
public:
	/** @name Construction */
	///@{
	/** @brief Creates an empty command buffer using a PMR memory resource. */
	inline explicit CommandBuffer(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) : commandStorage(resource) {}
	/** @brief Creates an empty command buffer backed by a shared handle allocator. */
	inline CommandBuffer(HandleAllocator& allocator, std::pmr::memory_resource* resource = std::pmr::get_default_resource()) : commandStorage(resource), sharedAllocator(&allocator) {}
	///@}

	/** @name Raw storage and submission */
	///@{
	/** @brief Returns a const pointer to the first command. */
	inline const GfxOp* data() const { return commandStorage.data(); }
	/** @brief Returns a mutable pointer to the first command. */
	inline GfxOp* data() { return commandStorage.data(); }
	/** @brief Returns the number of queued commands. */
	inline size_t size() const { return commandStorage.size(); }
	/**
	 * @brief Returns the queued commands as an externally owned span.
	 *
	 * The span is invalidated by further recording that reallocates storage and
	 * by @ref clear. Backend code must not retain it beyond submission unless it
	 * copies the commands.
	 */
	inline std::span<const GfxOp> span() const { return std::span<const GfxOp>(commandStorage.data(), commandStorage.size()); }
	/** @brief Removes all queued commands without resetting ID allocation. */
	inline void clear() { commandStorage.clear(); }
	/** @brief Submits the queued commands to a backend device. */
	inline HalStatus submit(Device& device) const { return device.processCommandQueue(span()); }
	/** @brief Appends a raw command. */
	inline void push(const GfxOp& op) { commandStorage.push_back(op); }
	/** @brief Appends an externally recorded command span in execution order. */
	inline void append(std::span<const GfxOp> commands) { commandStorage.insert(commandStorage.end(), commands.begin(), commands.end()); }
	/** @brief Allocates a handle ID from the local or shared allocator. */
	inline HandleId allocateId() { return sharedAllocator ? sharedAllocator->allocate() : nextId++; }
	///@}

	/** @name Explicit destroy helpers */
	///@{
	inline void destroyBuffer(HandleId id) { push({ Opcode::DestroyBufferObject, { .opDestroy = { id } } }); }
	inline void destroyTexture(HandleId id) { push({ Opcode::DestroyTexture, { .opDestroy = { id } } }); }
	inline void destroySampler(HandleId id) { push({ Opcode::DestroySampler, { .opDestroy = { id } } }); }
	inline void destroyImage(HandleId id) { push({ Opcode::DestroyImage, { .opDestroyImage = { id } } }); }
	inline void destroyFramebuffer(HandleId id) { push({ Opcode::DestroyFramebuffer, { .opDestroyFramebuffer = { id } } }); }
	inline void destroyPipeline(HandleId id) { push({ Opcode::DestroyPipeline, { .opDestroyPipeline = { id } } }); }
	inline void destroyComputePipeline(HandleId id) { push({ Opcode::DestroyComputePipeline, { .opDestroyComputePipeline = { id } } }); }
	inline void destroyFence(HandleId id) { push({ Opcode::DestroyFence, { .opDestroyFence = { id } } }); }
	inline void destroyQueryPool(HandleId id) { push({ Opcode::DestroyQueryPool, { .opDestroyQueryPool = { id } } }); }
	inline void destroySwapchain(HandleId id) { push({ Opcode::DestroySwapchain, { .opDestroySwapchain = { id } } }); }
	///@}

	/** @name Resource creation helpers */
	///@{
	inline Buffer createBuffer(uint32_t size, uint32_t policy = 0, const void* data = nullptr) {
		const HandleId id = allocateId();
		push({ Opcode::CreateBufferObject, { .opCreateBufferObject = { id, size, policy, data } } });
		return Buffer(id, this);
	}
	inline Texture createTexture1D(TextureFormat format, uint32_t width, const void* pixels = nullptr) {
		const HandleId id = allocateId();
		push({ Opcode::CreateTexture1D, { .opCreateTexture1D = { id, format, width, pixels } } });
		return Texture(id, this);
	}
	inline Texture createTexture2D(TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t width, uint16_t height, const void* pixels = nullptr) {
		const HandleId id = allocateId();
		push({ Opcode::CreateTexture2D, { .opCreateTexture2D = { id, format, mipLevels, generateMipmaps, width, height, pixels } } });
		return Texture(id, this);
	}
	inline Texture createCompressedTexture2D(TextureFormat format, uint16_t width, uint16_t height, TextureCompressionLayout layout, const TextureMipData* mips, uint8_t mipCount) {
		const HandleId id = allocateId();
		push({ Opcode::CreateCompressedTexture2D, { .opCreateCompressedTexture2D = { id, format, width, height, layout, mips, mipCount } } });
		return Texture(id, this);
	}
	inline Texture createTexture2DFromDecodeTarget(const DecodeTarget* source, uint8_t mipLevels, bool generateMipmaps) {
		const HandleId id = allocateId();
		push({ Opcode::CreateTexture2DFromDecodeTarget, { .opCreateTexture2DFromDecodeTarget = { id, source, mipLevels, generateMipmaps } } });
		return Texture(id, this);
	}
	inline Texture createTexture3D(TextureFormat format, bool generateMipmaps, uint16_t width, uint16_t height, uint16_t depth, const void* pixels = nullptr) {
		const HandleId id = allocateId();
		push({ Opcode::CreateTexture3D, { .opCreateTexture3D = { id, format, generateMipmaps, width, height, depth, pixels } } });
		return Texture(id, this);
	}
	inline Texture createTextureCube(TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t size) {
		const HandleId id = allocateId();
		push({ Opcode::CreateTextureCube, { .opCreateTextureCube = { id, format, mipLevels, generateMipmaps, size } } });
		return Texture(id, this);
	}
	inline Texture createTexture2DArray(TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t width, uint16_t height, uint16_t layers, const void* pixels = nullptr) {
		const HandleId id = allocateId();
		push({ Opcode::CreateTexture2DArray, { .opCreateTexture2DArray = { id, format, mipLevels, generateMipmaps, width, height, layers, pixels } } });
		return Texture(id, this);
	}
	inline Image createImage2D(TextureFormat format, uint16_t width, uint16_t height, uint8_t mipLevels, uint32_t usage) {
		const HandleId id = allocateId();
		push({ Opcode::CreateImage2D, { .opCreateImage2D = { id, format, width, height, mipLevels, usage } } });
		return Image(id, this);
	}
	inline Image createMultisampledImage2D(TextureFormat format, uint16_t width, uint16_t height, uint8_t samples, uint32_t usage) {
		const HandleId id = allocateId();
		push({ Opcode::CreateMultisampledImage2D, { .opCreateMultisampledImage2D = { id, format, width, height, samples, usage } } });
		return Image(id, this);
	}
	inline Sampler createSampler(SamplerFiltering filtering, const SamplerWrapping (&wrapping)[3], float maxAnisotropy, float maxLod, float minLod, float lodBias) {
		const HandleId id = allocateId();
		push({ Opcode::CreateSampler, { .opCreateSampler = { id, filtering, { wrapping[0], wrapping[1], wrapping[2] }, maxAnisotropy, maxLod, minLod, lodBias } } });
		return Sampler(id, this);
	}
	inline Framebuffer createFramebuffer(uint16_t width, uint16_t height, const FramebufferAttachment* attachments, uint32_t attachmentCount) {
		const HandleId id = allocateId();
		push({ Opcode::CreateFramebuffer, { .opCreateFramebuffer = { id, width, height, attachments, attachmentCount } } });
		return Framebuffer(id, this);
	}
	inline Pipeline createPipeline(const char* name, const ShaderBinaryDescriptor* shaderBinaries, uint32_t shaderBinaryCount, const VertexDescriptor* vertexInput, const PipelineBindingLayout* bindingLayout = nullptr) {
		const HandleId id = allocateId();
		push({ Opcode::CreatePipeline, { .opCreatePipeline = { id, name, shaderBinaries, shaderBinaryCount, vertexInput, bindingLayout } } });
		return Pipeline(id, this);
	}
	inline ComputePipeline createComputePipeline(const char* name, const ShaderBinaryDescriptor* shader, const PipelineBindingLayout* bindingLayout = nullptr) {
		const HandleId id = allocateId();
		push({ Opcode::CreateComputePipeline, { .opCreateComputePipeline = { id, name, shader, bindingLayout } } });
		return ComputePipeline(id, this);
	}
	inline Fence createFence(uint64_t initialValue = 0) {
		const HandleId id = allocateId();
		push({ Opcode::CreateFence, { .opCreateFence = { id, initialValue } } });
		return Fence(id, this);
	}
	inline QueryPool createQueryPool(QueryType type, uint32_t queryCount) {
		const HandleId id = allocateId();
		push({ Opcode::CreateQueryPool, { .opCreateQueryPool = { id, type, queryCount } } });
		return QueryPool(id, this);
	}
	inline HandleId createSwapchain(const SwapchainDescriptor* descriptor) {
		const HandleId id = allocateId();
		push({ Opcode::CreateSwapchain, { .opCreateSwapchain = { id, descriptor } } });
		return id;
	}
	///@}

	/** @name Resource mutation, transfer, readback, and swapchain helpers */
	///@{
	inline void resizeBuffer(HandleId id, uint32_t size) { push({ Opcode::ResizeBufferObject, { .opResizeBufferObject = { id, size } } }); }
	inline void updateBuffer(HandleId id, uint32_t offset, uint32_t size, const void* data) { push({ Opcode::UpdateBufferObject, { .opUpdateBufferObject = { id, offset, size, data } } }); }
	inline void mapBuffer(HandleId id, uint32_t offset, uint32_t size, BufferMapAccess access, BufferMapCallback callback, void* userData) { push({ Opcode::MapBufferObject, { .opMapBufferObject = { id, offset, size, access, callback, userData } } }); }
	inline void copyBuffer(HandleId source, HandleId destination, uint32_t sourceOffset, uint32_t destinationOffset, uint32_t size) { push({ Opcode::CopyBuffer, { .opCopyBuffer = { source, destination, sourceOffset, destinationOffset, size } } }); }
	inline void readBuffer(HandleId buffer, uint32_t offset, uint32_t size, BufferReadbackCallback callback, void* userData) { push({ Opcode::ReadBuffer, { .opReadBuffer = { buffer, offset, size, callback, userData } } }); }
	inline void createVertexArray(HandleId id, const VertexDescriptor* descriptor, uint32_t elementCount) { push({ Opcode::CreateVertexArrayObject, { .opCreateVertexArrayObject = { id, descriptor, elementCount } } }); }
	inline void resizeTexture2D(HandleId id, TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t width, uint16_t height) { push({ Opcode::ResizeTexture2D, { .opResizeTexture2D = { id, format, mipLevels, generateMipmaps, width, height } } }); }
	inline void updateTexture2D(HandleId id, TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { push({ Opcode::UpdateTexture2D, { .opUpdateTexture2D = { id, format, mipLevel, x, y, width, height, pixels } } }); }
	inline void updateCompressedTexture2D(HandleId id, TextureFormat format, TextureCompressionLayout layout, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t byteSize, const void* data) { push({ Opcode::UpdateCompressedTexture2D, { .opUpdateCompressedTexture2D = { id, format, layout, mipLevel, x, y, width, height, byteSize, data } } }); }
	inline void updateTextureCubeFace(HandleId id, TextureCubeFace face, TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { push({ Opcode::UpdateTextureCubeFace, { .opUpdateTextureCubeFace = { id, face, format, mipLevel, x, y, width, height, pixels } } }); }
	inline void updateTexture2DArray(HandleId id, TextureFormat format, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { push({ Opcode::UpdateTexture2DArray, { .opUpdateTexture2DArray = { id, format, mipLevel, layer, x, y, width, height, pixels } } }); }
	inline void blitTexture2D(HandleId source, HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { push({ Opcode::BlitTexture2D, { .opBlitTexture2D = { source, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	inline void generateTextureMipmaps(HandleId id) { push({ Opcode::GenerateTextureMipmaps, { .opGenerateTextureMipmaps = { id } } }); }
	inline void resizeImage2D(HandleId id, TextureFormat format, uint16_t width, uint16_t height, uint8_t mipLevels, uint32_t usage) { push({ Opcode::ResizeImage2D, { .opResizeImage2D = { id, format, width, height, mipLevels, usage } } }); }
	inline void updateImage2D(HandleId id, TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { push({ Opcode::UpdateImage2D, { .opUpdateImage2D = { id, format, mipLevel, x, y, width, height, pixels } } }); }
	inline void blitImage2D(HandleId source, HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { push({ Opcode::BlitImage2D, { .opBlitImage2D = { source, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	inline void resolveImage2D(HandleId source, HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { push({ Opcode::ResolveImage2D, { .opResolveImage2D = { source, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	inline void copyBufferToImage2D(HandleId sourceBuffer, HandleId destinationImage, uint32_t bufferOffset, uint32_t bufferRowPitch, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height) { push({ Opcode::CopyBufferToImage2D, { .opCopyBufferToImage2D = { sourceBuffer, destinationImage, bufferOffset, bufferRowPitch, mipLevel, layer, x, y, width, height } } }); }
	inline void copyImage2DToBuffer(HandleId sourceImage, HandleId destinationBuffer, uint32_t bufferOffset, uint32_t bufferRowPitch, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height) { push({ Opcode::CopyImage2DToBuffer, { .opCopyImage2DToBuffer = { sourceImage, destinationBuffer, bufferOffset, bufferRowPitch, mipLevel, layer, x, y, width, height } } }); }
	inline void copyImage2D(HandleId sourceImage, HandleId destinationImage, uint8_t sourceMipLevel, uint16_t sourceLayer, uint8_t destinationMipLevel, uint16_t destinationLayer, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { push({ Opcode::CopyImage2D, { .opCopyImage2D = { sourceImage, destinationImage, sourceMipLevel, sourceLayer, destinationMipLevel, destinationLayer, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	inline void readImage2D(HandleId image, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, ImageReadbackCallback callback, void* userData) { push({ Opcode::ReadImage2D, { .opReadImage2D = { image, mipLevel, layer, x, y, width, height, callback, userData } } }); }
	inline void resizeSwapchain(HandleId id, uint16_t width, uint16_t height, const SwapchainImageBinding* imageBindings = nullptr, uint32_t imageBindingCount = 0) { push({ Opcode::ResizeSwapchain, { .opResizeSwapchain = { id, width, height, imageBindings, imageBindingCount } } }); }
	inline void acquireSwapchainImage(HandleId swapchain, SwapchainAcquireCallback callback, void* userData) { push({ Opcode::AcquireSwapchainImage, { .opAcquireSwapchainImage = { swapchain, callback, userData } } }); }
	inline void presentSwapchain(HandleId swapchain = DefaultSwapchain, uint32_t imageIndex = 0) { push({ Opcode::PresentSwapchain, { .opPresentSwapchain = { swapchain, imageIndex } } }); }
	///@}

	/** @name Framebuffer, render pass, and pipeline binding helpers */
	///@{
	inline void beginFrame(HandleId framebuffer = DefaultFramebuffer) { push({ Opcode::BeginFrame, { .opBeginFrame = { framebuffer } } }); }
	inline void endFrame(HandleId framebuffer = DefaultFramebuffer) { push({ Opcode::EndFrame, { .opEndFrame = { framebuffer } } }); }
	inline void present(HandleId framebuffer = DefaultFramebuffer) { push({ Opcode::Present, { .opPresent = { framebuffer } } }); }
	inline void bindFramebuffer(HandleId id) { push({ Opcode::BindFramebuffer, { .opBindFramebuffer = { id } } }); }
	inline void clearColorAttachment(HandleId framebuffer, uint32_t colorAttachmentIndex, const float (&color)[4]) { push({ Opcode::ClearColorAttachment, { .opClearColorAttachment = { framebuffer, colorAttachmentIndex, { color[0], color[1], color[2], color[3] } } } }); }
	inline void clearDepthStencilAttachment(HandleId framebuffer, bool clearDepth, bool clearStencil, float depth, uint32_t stencil) { push({ Opcode::ClearDepthStencilAttachment, { .opClearDepthStencilAttachment = { framebuffer, clearDepth, clearStencil, depth, stencil } } }); }
	inline void beginRenderPass(const RenderPassDescriptor* descriptor) { push({ Opcode::BeginRenderPass, { .opBeginRenderPass = { descriptor } } }); }
	inline void endRenderPass() { push({ Opcode::EndRenderPass, { .opEndRenderPass = { 0 } } }); }
	inline void bindPipeline(HandleId id) { push({ Opcode::BindPipeline, { .opBindPipeline = { id } } }); }
	inline void bindComputePipeline(HandleId id) { push({ Opcode::BindComputePipeline, { .opBindComputePipeline = { id } } }); }
	inline void bindTexture(uint32_t slot, HandleId texture, TextureBindingType type) { push({ Opcode::BindTexture, { .opBindTexture = { slot, texture, type } } }); }
	inline void bindSampler(uint32_t slot, HandleId sampler) { push({ Opcode::BindSampler, { .opBindSampler = { slot, sampler } } }); }
	inline void bindUniformBuffer(uint32_t slot, HandleId buffer, uint32_t offset, uint32_t size) { push({ Opcode::BindUniformBuffer, { .opBindUniformBuffer = { slot, buffer, offset, size } } }); }
	inline void bindStorageBuffer(uint32_t slot, HandleId buffer, uint32_t offset, uint32_t size, StorageAccess access) { push({ Opcode::BindStorageBuffer, { .opBindStorageBuffer = { slot, buffer, offset, size, access } } }); }
	inline void bindStorageImage(uint32_t slot, HandleId image, uint8_t mipLevel, uint16_t layer, StorageAccess access) { push({ Opcode::BindStorageImage, { .opBindStorageImage = { slot, image, mipLevel, layer, access } } }); }
	inline void bindVertexBuffer(uint32_t binding, HandleId buffer, uint32_t offset, uint32_t stride, uint32_t instanceDivisor = 0) { push({ Opcode::BindVertexBuffer, { .opBindVertexBuffer = { binding, buffer, offset, stride, instanceDivisor } } }); }
	inline void bindIndexBuffer(HandleId buffer, uint32_t offset, IndexType type) { push({ Opcode::BindIndexBuffer, { .opBindIndexBuffer = { buffer, offset, type } } }); }
	inline void drawTriangles(uint32_t vertexOffset, uint32_t vertexCount) { push({ Opcode::DrawTriangles, { .opDrawTriangles = { vertexOffset, vertexCount } } }); }
	inline void drawIndexedTriangles(uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset) { push({ Opcode::DrawIndexedTriangles, { .opDrawIndexedTriangles = { indexOffset, indexCount, vertexOffset } } }); }
	inline void drawLines(uint32_t vertexOffset, uint32_t vertexCount, float thickness) { push({ Opcode::DrawLines, { .opDrawLines = { vertexOffset, vertexCount, thickness } } }); }
	inline void drawIndexedLines(uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, float thickness) { push({ Opcode::DrawIndexedLines, { .opDrawIndexedLines = { indexOffset, indexCount, vertexOffset, thickness } } }); }
	inline void drawTrianglesInstanced(uint32_t vertexOffset, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstInstance) { push({ Opcode::DrawTrianglesInstanced, { .opDrawTrianglesInstanced = { vertexOffset, vertexCount, instanceCount, firstInstance } } }); }
	inline void drawIndexedTrianglesInstanced(uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceCount, uint32_t firstInstance) { push({ Opcode::DrawIndexedTrianglesInstanced, { .opDrawIndexedTrianglesInstanced = { indexOffset, indexCount, vertexOffset, instanceCount, firstInstance } } }); }
	inline void drawLinesInstanced(uint32_t vertexOffset, uint32_t vertexCount, float thickness, uint32_t instanceCount, uint32_t firstInstance) { push({ Opcode::DrawLinesInstanced, { .opDrawLinesInstanced = { vertexOffset, vertexCount, thickness, instanceCount, firstInstance } } }); }
	inline void drawIndexedLinesInstanced(uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, float thickness, uint32_t instanceCount, uint32_t firstInstance) { push({ Opcode::DrawIndexedLinesInstanced, { .opDrawIndexedLinesInstanced = { indexOffset, indexCount, vertexOffset, thickness, instanceCount, firstInstance } } }); }
	///@}

	/** @name State, synchronization, compute, query, and debug helpers */
	///@{
	inline void setViewport(float x, float y, float width, float height, float minDepth, float maxDepth) { push({ Opcode::SetViewport, { .opSetViewport = { x, y, width, height, minDepth, maxDepth } } }); }
	inline void setScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) { push({ Opcode::SetScissor, { .opSetScissor = { x, y, width, height } } }); }
	inline void setBlendState(const OpSetBlendState& state) { push({ Opcode::SetBlendState, { .opSetBlendState = state } }); }
	inline void setDepthStencilState(const OpSetDepthStencilState& state) { push({ Opcode::SetDepthStencilState, { .opSetDepthStencilState = state } }); }
	inline void setRasterizerState(const OpSetRasterizerState& state) { push({ Opcode::SetRasterizerState, { .opSetRasterizerState = state } }); }
	inline void transitionResource(HandleId id, ResourceKind kind, ResourceState oldState, ResourceState newState, uint32_t sourceStages, uint32_t destinationStages) { push({ Opcode::TransitionResource, { .opTransitionResource = { id, kind, oldState, newState, sourceStages, destinationStages } } }); }
	inline void bufferBarrier(HandleId buffer, uint32_t offset, uint32_t size, uint32_t sourceStages, uint32_t destinationStages, uint32_t sourceAccess, uint32_t destinationAccess) { push({ Opcode::BufferBarrier, { .opBufferBarrier = { buffer, offset, size, sourceStages, destinationStages, sourceAccess, destinationAccess } } }); }
	inline void imageBarrier(const OpImageBarrier& barrier) { push({ Opcode::ImageBarrier, { .opImageBarrier = barrier } }); }
	inline void signalFence(HandleId id, uint64_t value) { push({ Opcode::SignalFence, { .opSignalFence = { id, value } } }); }
	inline void waitFence(HandleId id, uint64_t value) { push({ Opcode::WaitFence, { .opWaitFence = { id, value } } }); }
	inline void dispatchCompute(uint32_t x, uint32_t y, uint32_t z) { push({ Opcode::DispatchCompute, { .opDispatchCompute = { x, y, z } } }); }
	inline void dispatchComputeIndirect(HandleId buffer, uint32_t offset) { push({ Opcode::DispatchComputeIndirect, { .opDispatchComputeIndirect = { buffer, offset } } }); }
	inline void drawIndirect(HandleId buffer, uint32_t offset, uint32_t drawCount, uint32_t stride) { push({ Opcode::DrawIndirect, { .opDrawIndirect = { buffer, offset, drawCount, stride } } }); }
	inline void drawIndexedIndirect(HandleId buffer, uint32_t offset, uint32_t drawCount, uint32_t stride) { push({ Opcode::DrawIndexedIndirect, { .opDrawIndexedIndirect = { buffer, offset, drawCount, stride } } }); }
	inline void resetQueryPool(HandleId id, uint32_t firstQuery, uint32_t queryCount) { push({ Opcode::ResetQueryPool, { .opResetQueryPool = { id, firstQuery, queryCount } } }); }
	inline void beginQuery(HandleId id, uint32_t query) { push({ Opcode::BeginQuery, { .opBeginQuery = { id, query } } }); }
	inline void endQuery(HandleId id, uint32_t query) { push({ Opcode::EndQuery, { .opEndQuery = { id, query } } }); }
	inline void writeTimestamp(HandleId id, uint32_t query, uint32_t pipelineStage) { push({ Opcode::WriteTimestamp, { .opWriteTimestamp = { id, query, pipelineStage } } }); }
	inline void readQueryResults(HandleId id, uint32_t firstQuery, uint32_t queryCount, QueryReadbackCallback callback, void* userData) { push({ Opcode::ReadQueryResults, { .opReadQueryResults = { id, firstQuery, queryCount, callback, userData } } }); }
	inline void debugLabelBegin(const char* name, const float (&color)[4]) { push({ Opcode::DebugLabelBegin, { .opDebugLabelBegin = { name, { color[0], color[1], color[2], color[3] } } } }); }
	inline void debugLabelEnd() { push({ Opcode::DebugLabelEnd, { .opDebugLabelEnd = { 0 } } }); }
	inline void debugMarker(const char* name, const float (&color)[4]) { push({ Opcode::DebugMarker, { .opDebugMarker = { name, { color[0], color[1], color[2], color[3] } } } }); }
	///@}
};

/**
 * @brief Appends command buffers into a destination in caller-specified order.
 *
 * This is the conservative parallel-recording primitive: each worker records
 * independently, then the coordinator chooses a deterministic merge order.
 */
inline void mergeCommandBuffers(CommandBuffer& destination, std::span<const CommandBuffer* const> sources) {
	for (const CommandBuffer* source : sources) {
		if (source) destination.append(source->span());
	}
}

/**
 * @brief Coordinator-owned queue of explicit deferred destroy requests.
 *
 * Use this when resource ownership is no longer tied to a live worker command
 * buffer. The coordinator records the actual destroy commands into an active
 * command buffer by calling @ref DeferredDestroyQueue::flushInto.
 */
class DeferredDestroyQueue {
	std::pmr::vector<DeferredDestroy> retirements;
public:
	/** @brief Creates an empty deferred destruction queue. */
	inline explicit DeferredDestroyQueue(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) : retirements(resource) {}
	/** @brief Adds a typed handle retirement request. */
	inline void retire(ResourceHandleKind kind, HandleId id) { retirements.push_back({ kind, id }); }
	/** @brief Returns the queued retirements. */
	inline std::span<const DeferredDestroy> span() const { return std::span<const DeferredDestroy>(retirements.data(), retirements.size()); }
	/** @brief Returns the number of queued retirements. */
	inline size_t size() const { return retirements.size(); }
	/** @brief Clears queued retirements without recording destroy commands. */
	inline void clear() { retirements.clear(); }
	/** @brief Records typed destroy commands into a coordinator command buffer. */
	inline void flushInto(CommandBuffer& destination) {
		for (const DeferredDestroy& retirement : retirements) {
			switch (retirement.kind) {
				case ResourceHandleKind::Buffer: destination.destroyBuffer(retirement.id); break;
				case ResourceHandleKind::Texture: destination.destroyTexture(retirement.id); break;
				case ResourceHandleKind::Sampler: destination.destroySampler(retirement.id); break;
				case ResourceHandleKind::Image: destination.destroyImage(retirement.id); break;
				case ResourceHandleKind::Framebuffer: destination.destroyFramebuffer(retirement.id); break;
				case ResourceHandleKind::Pipeline: destination.destroyPipeline(retirement.id); break;
				case ResourceHandleKind::ComputePipeline: destination.destroyComputePipeline(retirement.id); break;
				case ResourceHandleKind::Fence: destination.destroyFence(retirement.id); break;
				case ResourceHandleKind::QueryPool: destination.destroyQueryPool(retirement.id); break;
				case ResourceHandleKind::Swapchain: destination.destroySwapchain(retirement.id); break;
			}
		}
		clear();
	}
};

inline Buffer& Buffer::operator=(Buffer&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyBuffer(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline Buffer::~Buffer() { if (owner && resourceId) owner->destroyBuffer(resourceId); }
inline Texture& Texture::operator=(Texture&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyTexture(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline Texture::~Texture() { if (owner && resourceId) owner->destroyTexture(resourceId); }
inline Image& Image::operator=(Image&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyImage(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline Image::~Image() { if (owner && resourceId) owner->destroyImage(resourceId); }
inline Sampler& Sampler::operator=(Sampler&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroySampler(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline Sampler::~Sampler() { if (owner && resourceId) owner->destroySampler(resourceId); }
inline Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyFramebuffer(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline Framebuffer::~Framebuffer() { if (owner && resourceId) owner->destroyFramebuffer(resourceId); }
inline Pipeline& Pipeline::operator=(Pipeline&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyPipeline(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline Pipeline::~Pipeline() { if (owner && resourceId) owner->destroyPipeline(resourceId); }
inline ComputePipeline& ComputePipeline::operator=(ComputePipeline&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyComputePipeline(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline ComputePipeline::~ComputePipeline() { if (owner && resourceId) owner->destroyComputePipeline(resourceId); }
inline Fence& Fence::operator=(Fence&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyFence(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline Fence::~Fence() { if (owner && resourceId) owner->destroyFence(resourceId); }
inline QueryPool& QueryPool::operator=(QueryPool&& other) noexcept { if (this != &other) { if (owner && resourceId) owner->destroyQueryPool(resourceId); resourceId = other.release(); owner = std::exchange(other.owner, nullptr); } return *this; }
inline QueryPool::~QueryPool() { if (owner && resourceId) owner->destroyQueryPool(resourceId); }

}
#endif // KLDCOMMANDBUFFER_HPP
