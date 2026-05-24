#ifndef KLDCOMMANDBUFFER_HPP
#define KLDCOMMANDBUFFER_HPP
/**
 * @file KldCommandBuffer.hpp
 * @brief Header-only object-oriented facade over Kaldi VM command buffers.
 *
 * This facade is a convenience layer over `GfxOp`. It does not add a new
 * backend contract: every helper simply appends one raw command payload to
 * client-owned storage. Resource proxy classes are move-only handles that record
 * typed destroy commands into their owning `CommandDispatcher` when destroyed.
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
 * `CommandDispatcher` is single-threaded for recording. Recording, clearing,
 * submitting, and RAII proxy destruction all mutate or inspect the same command
 * storage. Use one dispatcher per recording thread, optionally backed by a
 * shared `HandleStorage`, then merge streams with `mergeCommandBuffers` in an
 * explicit order.
 */
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldDevice.hpp>
#include <memory_resource>
#include <span>
#include <utility>
#include <vector>
#include <Elvavena/Util/ElvIdPool.hpp>
#include <Elvavena/Util/ElvDoubleBufferedVector.hpp>

namespace Kld {

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

// ─────────────────────────────────────────────────────────────────────────────
// HandleStorage
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Typed ID pools for every resource kind.
 *
 * Each resource kind gets its own `IdPool` so IDs are independently recycled.
 * The allocator is shared across all pools.
 */
template <typename HandleAllocator = std::allocator<HandleId>>
class HandleStorage {
public:
	typedef Elv::Util::IdPool<HandleId, HandleAllocator> HandlePool;
private:
	HandlePool bufferIds;
	HandlePool textureIds;
	HandlePool samplerIds;
	HandlePool imageIds;
	HandlePool framebufferIds;
	HandlePool pipelineIds;
	HandlePool computePipelineIds;
	HandlePool fenceIds;
	HandlePool queryPoolIds;
	HandlePool swapchainIds;
public:
	inline explicit HandleStorage(const HandleAllocator& handleAlloc = HandleAllocator())
		: bufferIds(handleAlloc),
		textureIds(handleAlloc),
		samplerIds(handleAlloc),
		imageIds(handleAlloc),
		framebufferIds(handleAlloc),
		pipelineIds(handleAlloc),
		computePipelineIds(handleAlloc),
		fenceIds(handleAlloc),
		queryPoolIds(handleAlloc),
		swapchainIds(handleAlloc)
	{}

	inline explicit HandleStorage(size_t reservedBufferFreelistSize,
								  size_t reservedTextureFreelistSize,
								  size_t reservedSamplerFreelistSize,
								  size_t reservedImageFreelistSize,
								  size_t reservedFramebufferFreelistSize,
								  size_t reservedPipelineFreelistSize,
								  size_t reservedComputePipelineFreelistSize,
								  size_t reservedFenceFreelistSize,
								  size_t reservedQueryPoolFreelistSize,
								  size_t reservedSwapchainFreelistSize,
								  const HandleAllocator& handleAlloc = HandleAllocator())
		: bufferIds(reservedBufferFreelistSize, handleAlloc),
		textureIds(reservedTextureFreelistSize, handleAlloc),
		samplerIds(reservedSamplerFreelistSize, handleAlloc),
		imageIds(reservedImageFreelistSize, handleAlloc),
		framebufferIds(reservedFramebufferFreelistSize, handleAlloc),
		pipelineIds(reservedPipelineFreelistSize, handleAlloc),
		computePipelineIds(reservedComputePipelineFreelistSize, handleAlloc),
		fenceIds(reservedFenceFreelistSize, handleAlloc),
		queryPoolIds(reservedQueryPoolFreelistSize, handleAlloc),
		swapchainIds(reservedSwapchainFreelistSize, handleAlloc)
	{}

	[[nodiscard]] inline HandleId acquireId(ResourceHandleKind kind) {
		switch (kind) {
			case ResourceHandleKind::Buffer:          return bufferIds.acquireId();
			case ResourceHandleKind::Texture:         return textureIds.acquireId();
			case ResourceHandleKind::Sampler:         return samplerIds.acquireId();
			case ResourceHandleKind::Image:           return imageIds.acquireId();
			case ResourceHandleKind::Framebuffer:     return framebufferIds.acquireId();
			case ResourceHandleKind::Pipeline:        return pipelineIds.acquireId();
			case ResourceHandleKind::ComputePipeline: return computePipelineIds.acquireId();
			case ResourceHandleKind::Fence:           return fenceIds.acquireId();
			case ResourceHandleKind::QueryPool:       return queryPoolIds.acquireId();
			case ResourceHandleKind::Swapchain:       return swapchainIds.acquireId();
			default:                                  return 0;
		}
	}

	inline void acquireIds(ResourceHandleKind kind, HandleId* ids, size_t idCount) {
		switch (kind) {
			case ResourceHandleKind::Buffer:          return bufferIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Texture:         return textureIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Sampler:         return samplerIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Image:           return imageIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Framebuffer:     return framebufferIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Pipeline:        return pipelineIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::ComputePipeline: return computePipelineIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Fence:           return fenceIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::QueryPool:       return queryPoolIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Swapchain:       return swapchainIds.acquireMultiple(ids, idCount);
			default:                                  return;
		}
	}

	template <typename Function>
	inline void acquireIds(ResourceHandleKind kind, Function&& fun, size_t idCount) {
		switch (kind) {
			case ResourceHandleKind::Buffer:          return bufferIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::Texture:         return textureIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::Sampler:         return samplerIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::Image:           return imageIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::Framebuffer:     return framebufferIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::Pipeline:        return pipelineIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::ComputePipeline: return computePipelineIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::Fence:           return fenceIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::QueryPool:       return queryPoolIds.acquireMultiple(idCount, fun);
			case ResourceHandleKind::Swapchain:       return swapchainIds.acquireMultiple(idCount, fun);
			default:                                  return;
		}
	}

	inline void freeId(ResourceHandleKind kind, HandleId id) {
		switch (kind) {
			case ResourceHandleKind::Buffer:          bufferIds.release(id);          break;
			case ResourceHandleKind::Texture:         textureIds.release(id);         break;
			case ResourceHandleKind::Sampler:         samplerIds.release(id);         break;
			case ResourceHandleKind::Image:           imageIds.release(id);           break;
			case ResourceHandleKind::Framebuffer:     framebufferIds.release(id);     break;
			case ResourceHandleKind::Pipeline:        pipelineIds.release(id);        break;
			case ResourceHandleKind::ComputePipeline: computePipelineIds.release(id); break;
			case ResourceHandleKind::Fence:           fenceIds.release(id);           break;
			case ResourceHandleKind::QueryPool:       queryPoolIds.release(id);       break;
			case ResourceHandleKind::Swapchain:       swapchainIds.release(id);       break;
			default:                                  return;
		}
	}

	inline void freeIds(ResourceHandleKind kind, const HandleId* ids, size_t idCount) {
		switch (kind) {
			case ResourceHandleKind::Buffer:          bufferIds.releaseMultiple(ids, idCount);          break;
			case ResourceHandleKind::Texture:         textureIds.releaseMultiple(ids, idCount);         break;
			case ResourceHandleKind::Sampler:         samplerIds.releaseMultiple(ids, idCount);         break;
			case ResourceHandleKind::Image:           imageIds.releaseMultiple(ids, idCount);           break;
			case ResourceHandleKind::Framebuffer:     framebufferIds.releaseMultiple(ids, idCount);     break;
			case ResourceHandleKind::Pipeline:        pipelineIds.releaseMultiple(ids, idCount);        break;
			case ResourceHandleKind::ComputePipeline: computePipelineIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Fence:           fenceIds.releaseMultiple(ids, idCount);           break;
			case ResourceHandleKind::QueryPool:       queryPoolIds.releaseMultiple(ids, idCount);       break;
			case ResourceHandleKind::Swapchain:       swapchainIds.releaseMultiple(ids, idCount);       break;
			default:                                  return;
		}
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// CommandDispatcher
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Combines typed ID pools with a double-buffered command queue.
 *
 * Recording threads push GfxOp commands into the write buffer. The reader
 * (typically a coordinator or submit thread) consumes the read buffer after
 * `swapBuffers()`.
 */
template <typename HandleAllocator = std::allocator<HandleId>,
		 typename CommandAllocator = std::allocator<GfxOp>>
class CommandDispatcher : public HandleStorage<HandleAllocator> {
public:
	typedef Elv::Util::DoubleBufferedVector<GfxOp, CommandAllocator> CommandBuffer;
	typedef HandleStorage<HandleAllocator>                            HandleStore;
	typedef std::span<GfxOp>                                         CommandSpan;
	typedef std::span<const GfxOp>                                   ConstCommandSpan;

private:
	CommandBuffer cmdBuff;

public:
	explicit CommandDispatcher(const HandleAllocator&  handleAlloc  = HandleAllocator(),
							   const CommandAllocator& commandAlloc = CommandAllocator())
		: HandleStore(handleAlloc), cmdBuff(commandAlloc)
	{}

	explicit CommandDispatcher(size_t reservedCommandBufferSize,
							   size_t reservedBufferFreelistSize,
							   size_t reservedTextureFreelistSize,
							   size_t reservedSamplerFreelistSize,
							   size_t reservedImageFreelistSize,
							   size_t reservedFramebufferFreelistSize,
							   size_t reservedPipelineFreelistSize,
							   size_t reservedComputePipelineFreelistSize,
							   size_t reservedFenceFreelistSize,
							   size_t reservedQueryPoolFreelistSize,
							   size_t reservedSwapchainFreelistSize,
							   const HandleAllocator&  handleAlloc  = HandleAllocator(),
							   const CommandAllocator& commandAlloc = CommandAllocator())
		: HandleStore(reservedBufferFreelistSize,
					  reservedTextureFreelistSize,
					  reservedSamplerFreelistSize,
					  reservedImageFreelistSize,
					  reservedFramebufferFreelistSize,
					  reservedPipelineFreelistSize,
					  reservedComputePipelineFreelistSize,
					  reservedFenceFreelistSize,
					  reservedQueryPoolFreelistSize,
					  reservedSwapchainFreelistSize,
					  handleAlloc),
		cmdBuff(reservedCommandBufferSize, commandAlloc)
	{}

	const CommandBuffer& getCommandBuffer() const { return cmdBuff; }
	CommandBuffer&       getCommandBuffer()       { return cmdBuff; }

	// ── Low-level write helpers used by oop_detail recording methods ──────────

	/** @brief Appends a raw command to the write buffer. */
	inline void push(const GfxOp& op) { cmdBuff.push_back(op); }

	/** @brief Appends a span of commands to the write buffer. */
	inline void append(ConstCommandSpan commands) {
		cmdBuff.onWriteBuffer([&](auto& vec) {
			vec.insert(vec.end(), commands.begin(), commands.end());
		});
	}

	/** @brief Submits the current read buffer to a backend device. */
	inline HalStatus submit(Device& device) const {
		HalStatus result{};
		cmdBuff.onReadBuffer([&](const auto& vec) {
			result = device.processCommandQueue(
				ConstCommandSpan(vec.data(), vec.size()));
		});
		return result;
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// oop_detail  —  RAII proxies + full recording surface, parameterised once
// ─────────────────────────────────────────────────────────────────────────────

#define OOP_DETAIL_HEADER template <typename HandleAllocator = std::allocator<HandleId>, typename CommandAllocator = std::allocator<GfxOp>>
#define OOP_DETAIL_HEADER_NODEFAULT template <typename HandleAllocator, typename CommandAllocator>


OOP_DETAIL_HEADER struct oop_detail {
	typedef HandleAllocator                           IdAlloc;
	typedef CommandAllocator                          CommandAlloc;
	typedef CommandDispatcher<IdAlloc, CommandAlloc>  Dispatcher;

	// ── Forward declarations so proxies can reference each other ─────────────
	class Buffer;
	class Texture;
	class Image;
	class Sampler;
	class Framebuffer;
	class Pipeline;
	class ComputePipeline;
	class Fence;
	class QueryPool;

	// ─────────────────────────────────────────────────────────────────────────
	// RAII proxy base pattern
	//
	// Every proxy:
	//   • is move-only
	//   • owns one HandleId + a non-owning Dispatcher*
	//   • enqueues the appropriate destroy command on destruction / move-assign
	//   • exposes id() and release() just like the SLOP version
	//
	// operator= and ~Proxy are defined *after* the Dispatcher command helpers
	// are visible, so they can call e.g. owner->destroyBuffer(id).
	// ─────────────────────────────────────────────────────────────────────────

	/** @brief Move-only RAII proxy for a buffer handle. */
	class Buffer {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline Buffer() = default;
		inline Buffer(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		Buffer(const Buffer&)            = delete;
		Buffer& operator=(const Buffer&) = delete;
		inline Buffer(Buffer&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline Buffer& operator=(Buffer&& other) noexcept;
		inline ~Buffer();
		/** @brief Returns the wrapped handle ID. */
		inline HandleId id()      const { return resourceId; }
		/** @brief Releases ownership without enqueuing a destroy command. */
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for a sampled texture handle. */
	class Texture {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline Texture() = default;
		inline Texture(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		Texture(const Texture&)            = delete;
		Texture& operator=(const Texture&) = delete;
		inline Texture(Texture&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline Texture& operator=(Texture&& other) noexcept;
		inline ~Texture();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for an image handle. */
	class Image {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline Image() = default;
		inline Image(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		Image(const Image&)            = delete;
		Image& operator=(const Image&) = delete;
		inline Image(Image&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline Image& operator=(Image&& other) noexcept;
		inline ~Image();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for a sampler handle. */
	class Sampler {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline Sampler() = default;
		inline Sampler(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		Sampler(const Sampler&)            = delete;
		Sampler& operator=(const Sampler&) = delete;
		inline Sampler(Sampler&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline Sampler& operator=(Sampler&& other) noexcept;
		inline ~Sampler();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for a framebuffer handle. */
	class Framebuffer {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline Framebuffer() = default;
		inline Framebuffer(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		Framebuffer(const Framebuffer&)            = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
		inline Framebuffer(Framebuffer&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline Framebuffer& operator=(Framebuffer&& other) noexcept;
		inline ~Framebuffer();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for a graphics pipeline handle. */
	class Pipeline {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline Pipeline() = default;
		inline Pipeline(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		Pipeline(const Pipeline&)            = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		inline Pipeline(Pipeline&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline Pipeline& operator=(Pipeline&& other) noexcept;
		inline ~Pipeline();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for a compute pipeline handle. */
	class ComputePipeline {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline ComputePipeline() = default;
		inline ComputePipeline(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		ComputePipeline(const ComputePipeline&)            = delete;
		ComputePipeline& operator=(const ComputePipeline&) = delete;
		inline ComputePipeline(ComputePipeline&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline ComputePipeline& operator=(ComputePipeline&& other) noexcept;
		inline ~ComputePipeline();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for a fence handle. */
	class Fence {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline Fence() = default;
		inline Fence(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		Fence(const Fence&)            = delete;
		Fence& operator=(const Fence&) = delete;
		inline Fence(Fence&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline Fence& operator=(Fence&& other) noexcept;
		inline ~Fence();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	/** @brief Move-only RAII proxy for a query pool handle. */
	class QueryPool {
		HandleId    resourceId{};
		Dispatcher* owner{};
	public:
		inline QueryPool() = default;
		inline QueryPool(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
		QueryPool(const QueryPool&)            = delete;
		QueryPool& operator=(const QueryPool&) = delete;
		inline QueryPool(QueryPool&& other) noexcept
			: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}
		inline QueryPool& operator=(QueryPool&& other) noexcept;
		inline ~QueryPool();
		inline HandleId id()      const { return resourceId; }
		inline HandleId release()       { return std::exchange(resourceId, 0); }
	};

	// ─────────────────────────────────────────────────────────────────────────
	// Recording surface
	//
	// Mirrors CommandBuffer in the SLOP version, but operates on a Dispatcher.
	// All helpers are free functions so that the Dispatcher itself stays a plain
	// data-and-queue object with no knowledge of GfxOp opcodes.
	// ─────────────────────────────────────────────────────────────────────────

	/** @name Explicit destroy helpers */
	///@{
	static inline void destroyBuffer(Dispatcher& d, HandleId id)          { d.freeId(ResourceHandleKind::Buffer, id); d.push({ Opcode::DestroyBufferObject,    { .opDestroy              = { id } } }); }
	static inline void destroyTexture(Dispatcher& d, HandleId id)         { d.freeId(ResourceHandleKind::Texture, id); d.push({ Opcode::DestroyTexture,         { .opDestroy              = { id } } }); }
	static inline void destroySampler(Dispatcher& d, HandleId id)         { d.freeId(ResourceHandleKind::Sampler, id); d.push({ Opcode::DestroySampler,         { .opDestroy              = { id } } }); }
	static inline void destroyImage(Dispatcher& d, HandleId id)           { d.freeId(ResourceHandleKind::Image, id); d.push({ Opcode::DestroyImage,           { .opDestroyImage         = { id } } }); }
	static inline void destroyFramebuffer(Dispatcher& d, HandleId id)     { d.freeId(ResourceHandleKind::Framebuffer, id); d.push({ Opcode::DestroyFramebuffer,     { .opDestroyFramebuffer   = { id } } }); }
	static inline void destroyPipeline(Dispatcher& d, HandleId id)        { d.freeId(ResourceHandleKind::Pipeline, id); d.push({ Opcode::DestroyPipeline,        { .opDestroyPipeline      = { id } } }); }
	static inline void destroyComputePipeline(Dispatcher& d, HandleId id) { d.freeId(ResourceHandleKind::ComputePipeline, id); d.push({ Opcode::DestroyComputePipeline, { .opDestroyComputePipeline= { id } } }); }
	static inline void destroyFence(Dispatcher& d, HandleId id)           { d.freeId(ResourceHandleKind::Fence, id); d.push({ Opcode::DestroyFence,           { .opDestroyFence         = { id } } }); }
	static inline void destroyQueryPool(Dispatcher& d, HandleId id)       { d.freeId(ResourceHandleKind::QueryPool, id); d.push({ Opcode::DestroyQueryPool,       { .opDestroyQueryPool     = { id } } }); }
	static inline void destroySwapchain(Dispatcher& d, HandleId id)       { d.freeId(ResourceHandleKind::Swapchain, id); d.push({ Opcode::DestroySwapchain,       { .opDestroySwapchain     = { id } } }); }
	///@}

	/** @name Resource creation helpers */
	///@{
	static inline Buffer createBuffer(Dispatcher& d, uint32_t size, uint32_t policy = 0, const void* data = nullptr) {
		const HandleId id = d.acquireId(ResourceHandleKind::Buffer);
		d.push({ Opcode::CreateBufferObject, { .opCreateBufferObject = { id, size, policy, data } } });
		return Buffer(id, &d);
	}
	static inline Texture createTexture1D(Dispatcher& d, TextureFormat format, uint32_t width, const void* pixels = nullptr) {
		const HandleId id = d.acquireId(ResourceHandleKind::Texture);
		d.push({ Opcode::CreateTexture1D, { .opCreateTexture1D = { id, format, width, pixels } } });
		return Texture(id, &d);
	}
	static inline Texture createTexture2D(Dispatcher& d, TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t width, uint16_t height, const void* pixels = nullptr) {
		const HandleId id = d.acquireId(ResourceHandleKind::Texture);
		d.push({ Opcode::CreateTexture2D, { .opCreateTexture2D = { id, format, mipLevels, generateMipmaps, width, height, pixels } } });
		return Texture(id, &d);
	}
	static inline Texture createCompressedTexture2D(Dispatcher& d, TextureFormat format, uint16_t width, uint16_t height, TextureCompressionLayout layout, const TextureMipData* mips, uint8_t mipCount) {
		const HandleId id = d.acquireId(ResourceHandleKind::Texture);
		d.push({ Opcode::CreateCompressedTexture2D, { .opCreateCompressedTexture2D = { id, format, width, height, layout, mips, mipCount } } });
		return Texture(id, &d);
	}
	static inline Texture createTexture2DFromDecodeTarget(Dispatcher& d, const DecodeTarget* source, uint8_t mipLevels, bool generateMipmaps) {
		const HandleId id = d.acquireId(ResourceHandleKind::Texture);
		d.push({ Opcode::CreateTexture2DFromDecodeTarget, { .opCreateTexture2DFromDecodeTarget = { id, source, mipLevels, generateMipmaps } } });
		return Texture(id, &d);
	}
	static inline Texture createTexture3D(Dispatcher& d, TextureFormat format, bool generateMipmaps, uint16_t width, uint16_t height, uint16_t depth, const void* pixels = nullptr) {
		const HandleId id = d.acquireId(ResourceHandleKind::Texture);
		d.push({ Opcode::CreateTexture3D, { .opCreateTexture3D = { id, format, generateMipmaps, width, height, depth, pixels } } });
		return Texture(id, &d);
	}
	static inline Texture createTextureCube(Dispatcher& d, TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t size) {
		const HandleId id = d.acquireId(ResourceHandleKind::Texture);
		d.push({ Opcode::CreateTextureCube, { .opCreateTextureCube = { id, format, mipLevels, generateMipmaps, size } } });
		return Texture(id, &d);
	}
	static inline Texture createTexture2DArray(Dispatcher& d, TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t width, uint16_t height, uint16_t layers, const void* pixels = nullptr) {
		const HandleId id = d.acquireId(ResourceHandleKind::Texture);
		d.push({ Opcode::CreateTexture2DArray, { .opCreateTexture2DArray = { id, format, mipLevels, generateMipmaps, width, height, layers, pixels } } });
		return Texture(id, &d);
	}
	static inline Image createImage2D(Dispatcher& d, TextureFormat format, uint16_t width, uint16_t height, uint8_t mipLevels, uint32_t usage) {
		const HandleId id = d.acquireId(ResourceHandleKind::Image);
		d.push({ Opcode::CreateImage2D, { .opCreateImage2D = { id, format, width, height, mipLevels, usage } } });
		return Image(id, &d);
	}
	static inline Image createMultisampledImage2D(Dispatcher& d, TextureFormat format, uint16_t width, uint16_t height, uint8_t samples, uint32_t usage) {
		const HandleId id = d.acquireId(ResourceHandleKind::Image);
		d.push({ Opcode::CreateMultisampledImage2D, { .opCreateMultisampledImage2D = { id, format, width, height, samples, usage } } });
		return Image(id, &d);
	}
	static inline Sampler createSampler(Dispatcher& d, SamplerFiltering filtering, const SamplerWrapping (&wrapping)[3], float maxAnisotropy, float maxLod, float minLod, float lodBias) {
		const HandleId id = d.acquireId(ResourceHandleKind::Sampler);
		d.push({ Opcode::CreateSampler, { .opCreateSampler = { id, filtering, { wrapping[0], wrapping[1], wrapping[2] }, maxAnisotropy, maxLod, minLod, lodBias } } });
		return Sampler(id, &d);
	}
	static inline Framebuffer createFramebuffer(Dispatcher& d, uint16_t width, uint16_t height, const FramebufferAttachment* attachments, uint32_t attachmentCount) {
		const HandleId id = d.acquireId(ResourceHandleKind::Framebuffer);
		d.push({ Opcode::CreateFramebuffer, { .opCreateFramebuffer = { id, width, height, attachments, attachmentCount } } });
		return Framebuffer(id, &d);
	}
	static inline Pipeline createPipeline(Dispatcher& d, const char* name, const ShaderBinaryDescriptor* shaderBinaries, uint32_t shaderBinaryCount, const VertexDescriptor* vertexInput, const PipelineBindingLayout* bindingLayout = nullptr) {
		const HandleId id = d.acquireId(ResourceHandleKind::Pipeline);
		d.push({ Opcode::CreatePipeline, { .opCreatePipeline = { id, name, shaderBinaries, shaderBinaryCount, vertexInput, bindingLayout } } });
		return Pipeline(id, &d);
	}
	static inline ComputePipeline createComputePipeline(Dispatcher& d, const char* name, const ShaderBinaryDescriptor* shader, const PipelineBindingLayout* bindingLayout = nullptr) {
		const HandleId id = d.acquireId(ResourceHandleKind::ComputePipeline);
		d.push({ Opcode::CreateComputePipeline, { .opCreateComputePipeline = { id, name, shader, bindingLayout } } });
		return ComputePipeline(id, &d);
	}
	static inline Fence createFence(Dispatcher& d, uint64_t initialValue = 0) {
		const HandleId id = d.acquireId(ResourceHandleKind::Fence);
		d.push({ Opcode::CreateFence, { .opCreateFence = { id, initialValue } } });
		return Fence(id, &d);
	}
	static inline QueryPool createQueryPool(Dispatcher& d, QueryType type, uint32_t queryCount) {
		const HandleId id = d.acquireId(ResourceHandleKind::QueryPool);
		d.push({ Opcode::CreateQueryPool, { .opCreateQueryPool = { id, type, queryCount } } });
		return QueryPool(id, &d);
	}
	/** @brief Creates a swapchain and returns its raw handle ID (not RAII-wrapped). */
	static inline HandleId createSwapchain(Dispatcher& d, const SwapchainDescriptor* descriptor) {
		const HandleId id = d.acquireId(ResourceHandleKind::Swapchain);
		d.push({ Opcode::CreateSwapchain, { .opCreateSwapchain = { id, descriptor } } });
		return id;
	}
	///@}

	/** @name Resource mutation, transfer, readback, and swapchain helpers */
	///@{
	static inline void resizeBuffer(Dispatcher& d, HandleId id, uint32_t size)                                                                                       { d.push({ Opcode::ResizeBufferObject,         { .opResizeBufferObject         = { id, size } } }); }
	static inline void updateBuffer(Dispatcher& d, HandleId id, uint32_t offset, uint32_t size, const void* data)                                                   { d.push({ Opcode::UpdateBufferObject,         { .opUpdateBufferObject         = { id, offset, size, data } } }); }
	static inline void mapBuffer(Dispatcher& d, HandleId id, uint32_t offset, uint32_t size, BufferMapAccess access, BufferMapCallback callback, void* userData)     { d.push({ Opcode::MapBufferObject,            { .opMapBufferObject            = { id, offset, size, access, callback, userData } } }); }
	static inline void copyBuffer(Dispatcher& d, HandleId source, HandleId destination, uint32_t sourceOffset, uint32_t destinationOffset, uint32_t size)            { d.push({ Opcode::CopyBuffer,                 { .opCopyBuffer                 = { source, destination, sourceOffset, destinationOffset, size } } }); }
	static inline void readBuffer(Dispatcher& d, HandleId buffer, uint32_t offset, uint32_t size, BufferReadbackCallback callback, void* userData)                   { d.push({ Opcode::ReadBuffer,                 { .opReadBuffer                 = { buffer, offset, size, callback, userData } } }); }
	static inline void createVertexArray(Dispatcher& d, HandleId id, const VertexDescriptor* descriptor, uint32_t elementCount)                                     { d.push({ Opcode::CreateVertexArrayObject,    { .opCreateVertexArrayObject    = { id, descriptor, elementCount } } }); }
	static inline void resizeTexture2D(Dispatcher& d, HandleId id, TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t width, uint16_t height)  { d.push({ Opcode::ResizeTexture2D,            { .opResizeTexture2D            = { id, format, mipLevels, generateMipmaps, width, height } } }); }
	static inline void updateTexture2D(Dispatcher& d, HandleId id, TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { d.push({ Opcode::UpdateTexture2D, { .opUpdateTexture2D = { id, format, mipLevel, x, y, width, height, pixels } } }); }
	static inline void updateCompressedTexture2D(Dispatcher& d, HandleId id, TextureFormat format, TextureCompressionLayout layout, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t byteSize, const void* data) { d.push({ Opcode::UpdateCompressedTexture2D, { .opUpdateCompressedTexture2D = { id, format, layout, mipLevel, x, y, width, height, byteSize, data } } }); }
	static inline void updateTextureCubeFace(Dispatcher& d, HandleId id, TextureCubeFace face, TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { d.push({ Opcode::UpdateTextureCubeFace, { .opUpdateTextureCubeFace = { id, face, format, mipLevel, x, y, width, height, pixels } } }); }
	static inline void updateTexture2DArray(Dispatcher& d, HandleId id, TextureFormat format, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { d.push({ Opcode::UpdateTexture2DArray, { .opUpdateTexture2DArray = { id, format, mipLevel, layer, x, y, width, height, pixels } } }); }
	static inline void blitTexture2D(Dispatcher& d, HandleId source, HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { d.push({ Opcode::BlitTexture2D, { .opBlitTexture2D = { source, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	static inline void generateTextureMipmaps(Dispatcher& d, HandleId id)                                                                                           { d.push({ Opcode::GenerateTextureMipmaps,     { .opGenerateTextureMipmaps     = { id } } }); }
	static inline void resizeImage2D(Dispatcher& d, HandleId id, TextureFormat format, uint16_t width, uint16_t height, uint8_t mipLevels, uint32_t usage)         { d.push({ Opcode::ResizeImage2D,              { .opResizeImage2D              = { id, format, width, height, mipLevels, usage } } }); }
	static inline void updateImage2D(Dispatcher& d, HandleId id, TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) { d.push({ Opcode::UpdateImage2D, { .opUpdateImage2D = { id, format, mipLevel, x, y, width, height, pixels } } }); }
	static inline void blitImage2D(Dispatcher& d, HandleId source, HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { d.push({ Opcode::BlitImage2D, { .opBlitImage2D = { source, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	static inline void resolveImage2D(Dispatcher& d, HandleId source, HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { d.push({ Opcode::ResolveImage2D, { .opResolveImage2D = { source, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	static inline void copyBufferToImage2D(Dispatcher& d, HandleId sourceBuffer, HandleId destinationImage, uint32_t bufferOffset, uint32_t bufferRowPitch, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height) { d.push({ Opcode::CopyBufferToImage2D, { .opCopyBufferToImage2D = { sourceBuffer, destinationImage, bufferOffset, bufferRowPitch, mipLevel, layer, x, y, width, height } } }); }
	static inline void copyImage2DToBuffer(Dispatcher& d, HandleId sourceImage, HandleId destinationBuffer, uint32_t bufferOffset, uint32_t bufferRowPitch, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height) { d.push({ Opcode::CopyImage2DToBuffer, { .opCopyImage2DToBuffer = { sourceImage, destinationBuffer, bufferOffset, bufferRowPitch, mipLevel, layer, x, y, width, height } } }); }
	static inline void copyImage2D(Dispatcher& d, HandleId sourceImage, HandleId destinationImage, uint8_t sourceMipLevel, uint16_t sourceLayer, uint8_t destinationMipLevel, uint16_t destinationLayer, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) { d.push({ Opcode::CopyImage2D, { .opCopyImage2D = { sourceImage, destinationImage, sourceMipLevel, sourceLayer, destinationMipLevel, destinationLayer, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
	static inline void readImage2D(Dispatcher& d, HandleId image, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, ImageReadbackCallback callback, void* userData) { d.push({ Opcode::ReadImage2D, { .opReadImage2D = { image, mipLevel, layer, x, y, width, height, callback, userData } } }); }
	static inline void resizeSwapchain(Dispatcher& d, HandleId id, uint16_t width, uint16_t height, const SwapchainImageBinding* imageBindings = nullptr, uint32_t imageBindingCount = 0) { d.push({ Opcode::ResizeSwapchain, { .opResizeSwapchain = { id, width, height, imageBindings, imageBindingCount } } }); }
	static inline void acquireSwapchainImage(Dispatcher& d, HandleId swapchain, SwapchainAcquireCallback callback, void* userData) { d.push({ Opcode::AcquireSwapchainImage, { .opAcquireSwapchainImage = { swapchain, callback, userData } } }); }
	static inline void presentSwapchain(Dispatcher& d, HandleId swapchain = DefaultSwapchain, uint32_t imageIndex = 0) { d.push({ Opcode::PresentSwapchain, { .opPresentSwapchain = { swapchain, imageIndex } } }); }
	///@}

	/** @name Framebuffer, render pass, and pipeline binding helpers */
	///@{
	static inline void beginFrame(Dispatcher& d, HandleId framebuffer = DefaultFramebuffer)    { d.push({ Opcode::BeginFrame,      { .opBeginFrame      = { framebuffer } } }); }
	static inline void endFrame(Dispatcher& d, HandleId framebuffer = DefaultFramebuffer)      { d.push({ Opcode::EndFrame,        { .opEndFrame        = { framebuffer } } }); }
	static inline void present(Dispatcher& d, HandleId framebuffer = DefaultFramebuffer)       { d.push({ Opcode::Present,         { .opPresent         = { framebuffer } } }); }
	static inline void bindFramebuffer(Dispatcher& d, HandleId id)                             { d.push({ Opcode::BindFramebuffer, { .opBindFramebuffer = { id } } }); }
	static inline void clearColorAttachment(Dispatcher& d, HandleId framebuffer, uint32_t colorAttachmentIndex, const float (&color)[4]) { d.push({ Opcode::ClearColorAttachment, { .opClearColorAttachment = { framebuffer, colorAttachmentIndex, { color[0], color[1], color[2], color[3] } } } }); }
	static inline void clearDepthStencilAttachment(Dispatcher& d, HandleId framebuffer, bool clearDepth, bool clearStencil, float depth, uint32_t stencil) { d.push({ Opcode::ClearDepthStencilAttachment, { .opClearDepthStencilAttachment = { framebuffer, clearDepth, clearStencil, depth, stencil } } }); }
	static inline void beginRenderPass(Dispatcher& d, const RenderPassDescriptor* descriptor)  { d.push({ Opcode::BeginRenderPass, { .opBeginRenderPass = { descriptor } } }); }
	static inline void endRenderPass(Dispatcher& d)                                            { d.push({ Opcode::EndRenderPass,   { .opEndRenderPass   = { 0 } } }); }
	static inline void bindPipeline(Dispatcher& d, HandleId id)                                { d.push({ Opcode::BindPipeline,    { .opBindPipeline    = { id } } }); }
	static inline void bindComputePipeline(Dispatcher& d, HandleId id)                         { d.push({ Opcode::BindComputePipeline, { .opBindComputePipeline = { id } } }); }
	static inline void bindTexture(Dispatcher& d, uint32_t slot, HandleId texture, TextureBindingType type) { d.push({ Opcode::BindTexture, { .opBindTexture = { slot, texture, type } } }); }
	static inline void bindSampler(Dispatcher& d, uint32_t slot, HandleId sampler)             { d.push({ Opcode::BindSampler,     { .opBindSampler    = { slot, sampler } } }); }
	static inline void bindUniformBuffer(Dispatcher& d, uint32_t slot, HandleId buffer, uint32_t offset, uint32_t size) { d.push({ Opcode::BindUniformBuffer, { .opBindUniformBuffer = { slot, buffer, offset, size } } }); }
	static inline void bindStorageBuffer(Dispatcher& d, uint32_t slot, HandleId buffer, uint32_t offset, uint32_t size, StorageAccess access) { d.push({ Opcode::BindStorageBuffer, { .opBindStorageBuffer = { slot, buffer, offset, size, access } } }); }
	static inline void bindStorageImage(Dispatcher& d, uint32_t slot, HandleId image, uint8_t mipLevel, uint16_t layer, StorageAccess access) { d.push({ Opcode::BindStorageImage, { .opBindStorageImage = { slot, image, mipLevel, layer, access } } }); }
	static inline void bindVertexBuffer(Dispatcher& d, uint32_t binding, HandleId buffer, uint32_t offset, uint32_t stride, uint32_t instanceDivisor = 0) { d.push({ Opcode::BindVertexBuffer, { .opBindVertexBuffer = { binding, buffer, offset, stride, instanceDivisor } } }); }
	static inline void bindIndexBuffer(Dispatcher& d, HandleId buffer, uint32_t offset, IndexType type) { d.push({ Opcode::BindIndexBuffer, { .opBindIndexBuffer = { buffer, offset, type } } }); }
	static inline void drawTriangles(Dispatcher& d, uint32_t vertexOffset, uint32_t vertexCount) { d.push({ Opcode::DrawTriangles, { .opDrawTriangles = { vertexOffset, vertexCount } } }); }
	static inline void drawIndexedTriangles(Dispatcher& d, uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset) { d.push({ Opcode::DrawIndexedTriangles, { .opDrawIndexedTriangles = { indexOffset, indexCount, vertexOffset } } }); }
	static inline void drawLines(Dispatcher& d, uint32_t vertexOffset, uint32_t vertexCount, float thickness) { d.push({ Opcode::DrawLines, { .opDrawLines = { vertexOffset, vertexCount, thickness } } }); }
	static inline void drawIndexedLines(Dispatcher& d, uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, float thickness) { d.push({ Opcode::DrawIndexedLines, { .opDrawIndexedLines = { indexOffset, indexCount, vertexOffset, thickness } } }); }
	static inline void drawTrianglesInstanced(Dispatcher& d, uint32_t vertexOffset, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstInstance) { d.push({ Opcode::DrawTrianglesInstanced, { .opDrawTrianglesInstanced = { vertexOffset, vertexCount, instanceCount, firstInstance } } }); }
	static inline void drawIndexedTrianglesInstanced(Dispatcher& d, uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceCount, uint32_t firstInstance) { d.push({ Opcode::DrawIndexedTrianglesInstanced, { .opDrawIndexedTrianglesInstanced = { indexOffset, indexCount, vertexOffset, instanceCount, firstInstance } } }); }
	static inline void drawLinesInstanced(Dispatcher& d, uint32_t vertexOffset, uint32_t vertexCount, float thickness, uint32_t instanceCount, uint32_t firstInstance) { d.push({ Opcode::DrawLinesInstanced, { .opDrawLinesInstanced = { vertexOffset, vertexCount, thickness, instanceCount, firstInstance } } }); }
	static inline void drawIndexedLinesInstanced(Dispatcher& d, uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, float thickness, uint32_t instanceCount, uint32_t firstInstance) { d.push({ Opcode::DrawIndexedLinesInstanced, { .opDrawIndexedLinesInstanced = { indexOffset, indexCount, vertexOffset, thickness, instanceCount, firstInstance } } }); }
	///@}

	/** @name State, synchronization, compute, query, and debug helpers */
	///@{
	static inline void setViewport(Dispatcher& d, float x, float y, float width, float height, float minDepth, float maxDepth) { d.push({ Opcode::SetViewport, { .opSetViewport = { x, y, width, height, minDepth, maxDepth } } }); }
	static inline void setScissor(Dispatcher& d, int32_t x, int32_t y, uint32_t width, uint32_t height) { d.push({ Opcode::SetScissor, { .opSetScissor = { x, y, width, height } } }); }
	static inline void setBlendState(Dispatcher& d, const OpSetBlendState& state)              { d.push({ Opcode::SetBlendState,        { .opSetBlendState        = state } }); }
	static inline void setDepthStencilState(Dispatcher& d, const OpSetDepthStencilState& state){ d.push({ Opcode::SetDepthStencilState,  { .opSetDepthStencilState = state } }); }
	static inline void setRasterizerState(Dispatcher& d, const OpSetRasterizerState& state)    { d.push({ Opcode::SetRasterizerState,    { .opSetRasterizerState   = state } }); }
	static inline void transitionResource(Dispatcher& d, HandleId id, ResourceKind kind, ResourceState oldState, ResourceState newState, uint32_t sourceStages, uint32_t destinationStages) { d.push({ Opcode::TransitionResource, { .opTransitionResource = { id, kind, oldState, newState, sourceStages, destinationStages } } }); }
	static inline void bufferBarrier(Dispatcher& d, HandleId buffer, uint32_t offset, uint32_t size, uint32_t sourceStages, uint32_t destinationStages, uint32_t sourceAccess, uint32_t destinationAccess) { d.push({ Opcode::BufferBarrier, { .opBufferBarrier = { buffer, offset, size, sourceStages, destinationStages, sourceAccess, destinationAccess } } }); }
	static inline void imageBarrier(Dispatcher& d, const OpImageBarrier& barrier)              { d.push({ Opcode::ImageBarrier,          { .opImageBarrier         = barrier } }); }
	static inline void signalFence(Dispatcher& d, HandleId id, uint64_t value)                 { d.push({ Opcode::SignalFence,           { .opSignalFence          = { id, value } } }); }
	static inline void waitFence(Dispatcher& d, HandleId id, uint64_t value)                   { d.push({ Opcode::WaitFence,             { .opWaitFence            = { id, value } } }); }
	static inline void dispatchCompute(Dispatcher& d, uint32_t x, uint32_t y, uint32_t z)     { d.push({ Opcode::DispatchCompute,       { .opDispatchCompute      = { x, y, z } } }); }
	static inline void dispatchComputeIndirect(Dispatcher& d, HandleId buffer, uint32_t offset){ d.push({ Opcode::DispatchComputeIndirect,{ .opDispatchComputeIndirect = { buffer, offset } } }); }
	static inline void drawIndirect(Dispatcher& d, HandleId buffer, uint32_t offset, uint32_t drawCount, uint32_t stride) { d.push({ Opcode::DrawIndirect, { .opDrawIndirect = { buffer, offset, drawCount, stride } } }); }
	static inline void drawIndexedIndirect(Dispatcher& d, HandleId buffer, uint32_t offset, uint32_t drawCount, uint32_t stride) { d.push({ Opcode::DrawIndexedIndirect, { .opDrawIndexedIndirect = { buffer, offset, drawCount, stride } } }); }
	static inline void resetQueryPool(Dispatcher& d, HandleId id, uint32_t firstQuery, uint32_t queryCount) { d.push({ Opcode::ResetQueryPool, { .opResetQueryPool = { id, firstQuery, queryCount } } }); }
	static inline void beginQuery(Dispatcher& d, HandleId id, uint32_t query)                  { d.push({ Opcode::BeginQuery,  { .opBeginQuery  = { id, query } } }); }
	static inline void endQuery(Dispatcher& d, HandleId id, uint32_t query)                    { d.push({ Opcode::EndQuery,    { .opEndQuery    = { id, query } } }); }
	static inline void writeTimestamp(Dispatcher& d, HandleId id, uint32_t query, uint32_t pipelineStage) { d.push({ Opcode::WriteTimestamp, { .opWriteTimestamp = { id, query, pipelineStage } } }); }
	static inline void readQueryResults(Dispatcher& d, HandleId id, uint32_t firstQuery, uint32_t queryCount, QueryReadbackCallback callback, void* userData) { d.push({ Opcode::ReadQueryResults, { .opReadQueryResults = { id, firstQuery, queryCount, callback, userData } } }); }
	static inline void debugLabelBegin(Dispatcher& d, const char* name, const float (&color)[4]) { d.push({ Opcode::DebugLabelBegin, { .opDebugLabelBegin = { name, { color[0], color[1], color[2], color[3] } } } }); }
	static inline void debugLabelEnd(Dispatcher& d)                                            { d.push({ Opcode::DebugLabelEnd, { .opDebugLabelEnd = { 0 } } }); }
	static inline void debugMarker(Dispatcher& d, const char* name, const float (&color)[4])   { d.push({ Opcode::DebugMarker,   { .opDebugMarker   = { name, { color[0], color[1], color[2], color[3] } } } }); }
	///@}

	// ─────────────────────────────────────────────────────────────────────────
	// Deferred-definition bodies for proxy move-assign and destructors.
	//
	// These are placed after all recording helpers are declared so that the
	// calls to destroyXxx() above are fully visible.  They are defined inside
	// the struct body so they remain in the same template instantiation scope.
	// ─────────────────────────────────────────────────────────────────────────

	// Buffer
	// (inline keyword on out-of-class-but-in-struct definitions is redundant
	//  but kept for readability parity with the SLOP version.)
};

// Proxy bodies — defined outside the struct body so the struct is complete,
// but still inside the namespace and gated by the same template parameters.

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::Buffer&
	oop_detail<HandleAllocator, CommandAllocator>::Buffer::operator=(Buffer&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyBuffer(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::Buffer::~Buffer() {
	if (owner && resourceId) oop_detail::destroyBuffer(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::Texture&
	oop_detail<HandleAllocator, CommandAllocator>::Texture::operator=(Texture&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyTexture(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::Texture::~Texture() {
	if (owner && resourceId) oop_detail::destroyTexture(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::Image&
	oop_detail<HandleAllocator, CommandAllocator>::Image::operator=(Image&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyImage(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::Image::~Image() {
	if (owner && resourceId) oop_detail::destroyImage(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::Sampler&
	oop_detail<HandleAllocator, CommandAllocator>::Sampler::operator=(Sampler&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroySampler(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::Sampler::~Sampler() {
	if (owner && resourceId) oop_detail::destroySampler(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::Framebuffer&
	oop_detail<HandleAllocator, CommandAllocator>::Framebuffer::operator=(Framebuffer&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyFramebuffer(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::Framebuffer::~Framebuffer() {
	if (owner && resourceId) oop_detail::destroyFramebuffer(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::Pipeline&
	oop_detail<HandleAllocator, CommandAllocator>::Pipeline::operator=(Pipeline&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyPipeline(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::Pipeline::~Pipeline() {
	if (owner && resourceId) oop_detail::destroyPipeline(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::ComputePipeline&
	oop_detail<HandleAllocator, CommandAllocator>::ComputePipeline::operator=(ComputePipeline&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyComputePipeline(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::ComputePipeline::~ComputePipeline() {
	if (owner && resourceId) oop_detail::destroyComputePipeline(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::Fence&
	oop_detail<HandleAllocator, CommandAllocator>::Fence::operator=(Fence&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyFence(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::Fence::~Fence() {
	if (owner && resourceId) oop_detail::destroyFence(*owner, resourceId);
}

OOP_DETAIL_HEADER_NODEFAULT
	inline typename oop_detail<HandleAllocator, CommandAllocator>::QueryPool&
	oop_detail<HandleAllocator, CommandAllocator>::QueryPool::operator=(QueryPool&& other) noexcept {
	if (this != &other) {
		if (owner && resourceId) oop_detail::destroyQueryPool(*owner, resourceId);
		resourceId = other.release();
		owner = std::exchange(other.owner, nullptr);
	}
	return *this;
}
OOP_DETAIL_HEADER_NODEFAULT
	inline oop_detail<HandleAllocator, CommandAllocator>::QueryPool::~QueryPool() {
	if (owner && resourceId) oop_detail::destroyQueryPool(*owner, resourceId);
}

// ─────────────────────────────────────────────────────────────────────────────
// Free helpers (parallel to the SLOP-era free functions)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Appends the write buffers of each source dispatcher into the
 *        destination dispatcher's write buffer, in caller-specified order.
 *
 * Sources are read via `onReadBuffer` so the read/write lock protocol of
 * `DoubleBufferedVector` is respected on both ends.
 */
OOP_DETAIL_HEADER
	inline void mergeCommandBuffers(
		CommandDispatcher<HandleAllocator, CommandAllocator>&                       destination,
		std::span<const CommandDispatcher<HandleAllocator, CommandAllocator>* const> sources)
{
	for (const auto* source : sources) {
		if (!source) continue;
		source->getCommandBuffer().onReadBuffer([&](const auto& vec) {
			destination.append(
				typename CommandDispatcher<HandleAllocator, CommandAllocator>::ConstCommandSpan(
					vec.data(), vec.size()));
		});
	}
}

/**
 * @brief One deferred resource retirement request.
 *
 * Useful when ownership crosses threads or when a worker dispatcher has already
 * been swapped/cleared and should no longer receive RAII destroy commands.
 */
struct DeferredDestroy {
	ResourceHandleKind kind;
	HandleId           id;
};

/**
 * @brief Coordinator-owned queue of explicit deferred destroy requests.
 *
 * Accepts retirements from any resource kind and flushes typed destroy commands
 * into a live `CommandDispatcher` via `flushInto`.
 */
OOP_DETAIL_HEADER
	class DeferredDestroyQueue {
	typedef CommandDispatcher<HandleAllocator, CommandAllocator> Dispatcher;
	std::pmr::vector<DeferredDestroy> retirements;
public:
	/** @brief Creates an empty deferred destruction queue. */
	inline explicit DeferredDestroyQueue(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
		: retirements(resource) {}

	/** @brief Adds a typed handle retirement request. */
	inline void retire(ResourceHandleKind kind, HandleId id) { retirements.push_back({ kind, id }); }
	/** @brief Returns the queued retirements. */
	inline std::span<const DeferredDestroy> span() const { return { retirements.data(), retirements.size() }; }
	/** @brief Returns the number of queued retirements. */
	inline size_t size() const { return retirements.size(); }
	/** @brief Clears queued retirements without recording destroy commands. */
	inline void clear() { retirements.clear(); }

	/** @brief Records typed destroy commands into a live dispatcher. */
	inline void flushInto(Dispatcher& destination) {
		using D = oop_detail<HandleAllocator, CommandAllocator>;
		for (const DeferredDestroy& r : retirements) {
			switch (r.kind) {
				case ResourceHandleKind::Buffer:          D::destroyBuffer(destination, r.id);          break;
				case ResourceHandleKind::Texture:         D::destroyTexture(destination, r.id);         break;
				case ResourceHandleKind::Sampler:         D::destroySampler(destination, r.id);         break;
				case ResourceHandleKind::Image:           D::destroyImage(destination, r.id);           break;
				case ResourceHandleKind::Framebuffer:     D::destroyFramebuffer(destination, r.id);     break;
				case ResourceHandleKind::Pipeline:        D::destroyPipeline(destination, r.id);        break;
				case ResourceHandleKind::ComputePipeline: D::destroyComputePipeline(destination, r.id); break;
				case ResourceHandleKind::Fence:           D::destroyFence(destination, r.id);           break;
				case ResourceHandleKind::QueryPool:       D::destroyQueryPool(destination, r.id);       break;
				case ResourceHandleKind::Swapchain:       D::destroySwapchain(destination, r.id);       break;
			}
		}
		clear();
	}
};

} // namespace Kld
#endif // KLDCOMMANDBUFFER_HPP