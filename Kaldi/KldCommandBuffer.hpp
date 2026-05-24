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
// ResourceProxyBase
// ─────────────────────────────────────────────────────────────────────────────

template <typename Derived, typename Dispatcher>
class ResourceProxyBase {
protected:
	HandleId    resourceId{};
	Dispatcher* owner{};

	inline ResourceProxyBase() = default;
	inline ResourceProxyBase(HandleId id, Dispatcher* dispatcher) : resourceId(id), owner(dispatcher) {}
	inline ResourceProxyBase(ResourceProxyBase&& other) noexcept
		: resourceId(other.release()), owner(std::exchange(other.owner, nullptr)) {}

	inline Derived& moveAssign(Derived&& other) noexcept {
		if (static_cast<Derived*>(this) != &other) {
			static_cast<Derived*>(this)->destroyResource();
			resourceId = other.release();
			owner = std::exchange(other.owner, nullptr);
		}
		return static_cast<Derived&>(*this);
	}

public:
	ResourceProxyBase(const ResourceProxyBase&)            = delete;
	ResourceProxyBase& operator=(const ResourceProxyBase&) = delete;
	/** @brief Returns the wrapped handle ID. */
	inline HandleId id() const { return resourceId; }
	/** @brief Allows passing the proxy anywhere a raw handle ID is expected. */
	inline operator HandleId() const { return resourceId; }
	/** @brief Releases ownership without enqueuing a destroy command. */
	inline HandleId release() { return std::exchange(resourceId, 0); }
};

// ─────────────────────────────────────────────────────────────────────────────
// oop_detail  —  RAII proxies + full recording surface, parameterised once
// ─────────────────────────────────────────────────────────────────────────────

#define OOP_DETAIL_HEADER template <typename HandleAllocator = std::allocator<HandleId>, typename CommandAllocator = std::allocator<GfxOp>>


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

	/** @brief Move-only RAII proxy for a buffer handle. */
	class Buffer : public ResourceProxyBase<Buffer, Dispatcher> {
		typedef ResourceProxyBase<Buffer, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::Buffer, resourceId);
			owner->push({ Opcode::DestroyBufferObject, { .opDestroy = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline Buffer() = default;
		inline Buffer(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		Buffer(const Buffer&)            = delete;
		Buffer& operator=(const Buffer&) = delete;
		inline Buffer(Buffer&& other) noexcept = default;
		inline Buffer& operator=(Buffer&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~Buffer() { destroyResource(); }
		inline void resize(uint32_t size) const { owner->push({ Opcode::ResizeBufferObject, { .opResizeBufferObject = { resourceId, size } } }); }
		inline void update(uint32_t offset, uint32_t size, const void* data) const { owner->push({ Opcode::UpdateBufferObject, { .opUpdateBufferObject = { resourceId, offset, size, data } } }); }
		inline void map(uint32_t offset, uint32_t size, BufferMapAccess access, BufferMapCallback callback, void* userData) const { owner->push({ Opcode::MapBufferObject, { .opMapBufferObject = { resourceId, offset, size, access, callback, userData } } }); }
		inline void copyTo(HandleId destination, uint32_t sourceOffset, uint32_t destinationOffset, uint32_t size) const { owner->push({ Opcode::CopyBuffer, { .opCopyBuffer = { resourceId, destination, sourceOffset, destinationOffset, size } } }); }
		inline void read(uint32_t offset, uint32_t size, BufferReadbackCallback callback, void* userData) const { owner->push({ Opcode::ReadBuffer, { .opReadBuffer = { resourceId, offset, size, callback, userData } } }); }
		inline void createVertexArray(const VertexDescriptor* descriptor, uint32_t elementCount) const { owner->push({ Opcode::CreateVertexArrayObject, { .opCreateVertexArrayObject = { resourceId, descriptor, elementCount } } }); }
		inline void bindUniform(uint32_t slot, uint32_t offset, uint32_t size) const { owner->push({ Opcode::BindUniformBuffer, { .opBindUniformBuffer = { slot, resourceId, offset, size } } }); }
		inline void bindStorage(uint32_t slot, uint32_t offset, uint32_t size, StorageAccess access) const { owner->push({ Opcode::BindStorageBuffer, { .opBindStorageBuffer = { slot, resourceId, offset, size, access } } }); }
		inline void bindVertex(uint32_t binding, uint32_t offset, uint32_t stride, uint32_t instanceDivisor = 0) const { owner->push({ Opcode::BindVertexBuffer, { .opBindVertexBuffer = { binding, resourceId, offset, stride, instanceDivisor } } }); }
		inline void bindIndex(uint32_t offset, IndexType type) const { owner->push({ Opcode::BindIndexBuffer, { .opBindIndexBuffer = { resourceId, offset, type } } }); }
		inline void dispatchComputeIndirect(uint32_t offset) const { owner->push({ Opcode::DispatchComputeIndirect, { .opDispatchComputeIndirect = { resourceId, offset } } }); }
		inline void drawIndirect(uint32_t offset, uint32_t drawCount, uint32_t stride) const { owner->push({ Opcode::DrawIndirect, { .opDrawIndirect = { resourceId, offset, drawCount, stride } } }); }
		inline void drawIndexedIndirect(uint32_t offset, uint32_t drawCount, uint32_t stride) const { owner->push({ Opcode::DrawIndexedIndirect, { .opDrawIndexedIndirect = { resourceId, offset, drawCount, stride } } }); }
		inline void barrier(uint32_t offset, uint32_t size, uint32_t sourceStages, uint32_t destinationStages, uint32_t sourceAccess, uint32_t destinationAccess) const { owner->push({ Opcode::BufferBarrier, { .opBufferBarrier = { resourceId, offset, size, sourceStages, destinationStages, sourceAccess, destinationAccess } } }); }
		inline void transition(ResourceState oldState, ResourceState newState, uint32_t sourceStages, uint32_t destinationStages) const { owner->push({ Opcode::TransitionResource, { .opTransitionResource = { resourceId, ResourceKind::Buffer, oldState, newState, sourceStages, destinationStages } } }); }
	};

	/** @brief Move-only RAII proxy for a sampled texture handle. */
	class Texture : public ResourceProxyBase<Texture, Dispatcher> {
		typedef ResourceProxyBase<Texture, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::Texture, resourceId);
			owner->push({ Opcode::DestroyTexture, { .opDestroy = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline Texture() = default;
		inline Texture(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		Texture(const Texture&)            = delete;
		Texture& operator=(const Texture&) = delete;
		inline Texture(Texture&& other) noexcept = default;
		inline Texture& operator=(Texture&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~Texture() { destroyResource(); }
		inline void resize2D(TextureFormat format, uint8_t mipLevels, bool generateMipmaps, uint16_t width, uint16_t height) const { owner->push({ Opcode::ResizeTexture2D, { .opResizeTexture2D = { resourceId, format, mipLevels, generateMipmaps, width, height } } }); }
		inline void update2D(TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) const { owner->push({ Opcode::UpdateTexture2D, { .opUpdateTexture2D = { resourceId, format, mipLevel, x, y, width, height, pixels } } }); }
		inline void updateCompressed2D(TextureFormat format, TextureCompressionLayout layout, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t byteSize, const void* data) const { owner->push({ Opcode::UpdateCompressedTexture2D, { .opUpdateCompressedTexture2D = { resourceId, format, layout, mipLevel, x, y, width, height, byteSize, data } } }); }
		inline void updateCubeFace(TextureCubeFace face, TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) const { owner->push({ Opcode::UpdateTextureCubeFace, { .opUpdateTextureCubeFace = { resourceId, face, format, mipLevel, x, y, width, height, pixels } } }); }
		inline void update2DArray(TextureFormat format, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) const { owner->push({ Opcode::UpdateTexture2DArray, { .opUpdateTexture2DArray = { resourceId, format, mipLevel, layer, x, y, width, height, pixels } } }); }
		inline void blitTo(HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) const { owner->push({ Opcode::BlitTexture2D, { .opBlitTexture2D = { resourceId, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
		inline void generateMipmaps() const { owner->push({ Opcode::GenerateTextureMipmaps, { .opGenerateTextureMipmaps = { resourceId } } }); }
		inline void bind(uint32_t slot, TextureBindingType type) const { owner->push({ Opcode::BindTexture, { .opBindTexture = { slot, resourceId, type } } }); }
		inline void transition(ResourceState oldState, ResourceState newState, uint32_t sourceStages, uint32_t destinationStages) const { owner->push({ Opcode::TransitionResource, { .opTransitionResource = { resourceId, ResourceKind::Texture, oldState, newState, sourceStages, destinationStages } } }); }
	};

	/** @brief Move-only RAII proxy for an image handle. */
	class Image : public ResourceProxyBase<Image, Dispatcher> {
		typedef ResourceProxyBase<Image, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::Image, resourceId);
			owner->push({ Opcode::DestroyImage, { .opDestroyImage = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline Image() = default;
		inline Image(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		Image(const Image&)            = delete;
		Image& operator=(const Image&) = delete;
		inline Image(Image&& other) noexcept = default;
		inline Image& operator=(Image&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~Image() { destroyResource(); }
		inline void resize2D(TextureFormat format, uint16_t width, uint16_t height, uint8_t mipLevels, uint32_t usage) const { owner->push({ Opcode::ResizeImage2D, { .opResizeImage2D = { resourceId, format, width, height, mipLevels, usage } } }); }
		inline void update2D(TextureFormat format, uint8_t mipLevel, uint16_t x, uint16_t y, uint16_t width, uint16_t height, const void* pixels) const { owner->push({ Opcode::UpdateImage2D, { .opUpdateImage2D = { resourceId, format, mipLevel, x, y, width, height, pixels } } }); }
		inline void blitTo(HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) const { owner->push({ Opcode::BlitImage2D, { .opBlitImage2D = { resourceId, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
		inline void resolveTo(HandleId destination, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) const { owner->push({ Opcode::ResolveImage2D, { .opResolveImage2D = { resourceId, destination, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
		inline void copyTo(HandleId destination, uint8_t sourceMipLevel, uint16_t sourceLayer, uint8_t destinationMipLevel, uint16_t destinationLayer, uint16_t sourceX, uint16_t sourceY, uint16_t destinationX, uint16_t destinationY, uint16_t width, uint16_t height) const { owner->push({ Opcode::CopyImage2D, { .opCopyImage2D = { resourceId, destination, sourceMipLevel, sourceLayer, destinationMipLevel, destinationLayer, sourceX, sourceY, destinationX, destinationY, width, height } } }); }
		inline void copyFromBuffer(HandleId sourceBuffer, uint32_t bufferOffset, uint32_t bufferRowPitch, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height) const { owner->push({ Opcode::CopyBufferToImage2D, { .opCopyBufferToImage2D = { sourceBuffer, resourceId, bufferOffset, bufferRowPitch, mipLevel, layer, x, y, width, height } } }); }
		inline void copyToBuffer(HandleId destinationBuffer, uint32_t bufferOffset, uint32_t bufferRowPitch, uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height) const { owner->push({ Opcode::CopyImage2DToBuffer, { .opCopyImage2DToBuffer = { resourceId, destinationBuffer, bufferOffset, bufferRowPitch, mipLevel, layer, x, y, width, height } } }); }
		inline void read2D(uint8_t mipLevel, uint16_t layer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, ImageReadbackCallback callback, void* userData) const { owner->push({ Opcode::ReadImage2D, { .opReadImage2D = { resourceId, mipLevel, layer, x, y, width, height, callback, userData } } }); }
		inline void bindStorage(uint32_t slot, uint8_t mipLevel, uint16_t layer, StorageAccess access) const { owner->push({ Opcode::BindStorageImage, { .opBindStorageImage = { slot, resourceId, mipLevel, layer, access } } }); }
		inline void barrier(const OpImageBarrier& barrier) const { owner->push({ Opcode::ImageBarrier, { .opImageBarrier = barrier } }); }
		inline void transition(ResourceState oldState, ResourceState newState, uint32_t sourceStages, uint32_t destinationStages) const { owner->push({ Opcode::TransitionResource, { .opTransitionResource = { resourceId, ResourceKind::Image, oldState, newState, sourceStages, destinationStages } } }); }
	};

	/** @brief Move-only RAII proxy for a sampler handle. */
	class Sampler : public ResourceProxyBase<Sampler, Dispatcher> {
		typedef ResourceProxyBase<Sampler, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::Sampler, resourceId);
			owner->push({ Opcode::DestroySampler, { .opDestroy = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline Sampler() = default;
		inline Sampler(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		Sampler(const Sampler&)            = delete;
		Sampler& operator=(const Sampler&) = delete;
		inline Sampler(Sampler&& other) noexcept = default;
		inline Sampler& operator=(Sampler&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~Sampler() { destroyResource(); }
		inline void bind(uint32_t slot) const { owner->push({ Opcode::BindSampler, { .opBindSampler = { slot, resourceId } } }); }
	};

	/** @brief Move-only RAII proxy for a framebuffer handle. */
	class Framebuffer : public ResourceProxyBase<Framebuffer, Dispatcher> {
		typedef ResourceProxyBase<Framebuffer, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::Framebuffer, resourceId);
			owner->push({ Opcode::DestroyFramebuffer, { .opDestroyFramebuffer = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline Framebuffer() = default;
		inline Framebuffer(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		Framebuffer(const Framebuffer&)            = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
		inline Framebuffer(Framebuffer&& other) noexcept = default;
		inline Framebuffer& operator=(Framebuffer&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~Framebuffer() { destroyResource(); }
		inline void bind() const { owner->push({ Opcode::BindFramebuffer, { .opBindFramebuffer = { resourceId } } }); }
		inline void beginFrame() const { owner->push({ Opcode::BeginFrame, { .opBeginFrame = { resourceId } } }); }
		inline void endFrame() const { owner->push({ Opcode::EndFrame, { .opEndFrame = { resourceId } } }); }
		inline void present() const { owner->push({ Opcode::Present, { .opPresent = { resourceId } } }); }
		inline void clearColor(uint32_t colorAttachmentIndex, const float (&color)[4]) const { owner->push({ Opcode::ClearColorAttachment, { .opClearColorAttachment = { resourceId, colorAttachmentIndex, { color[0], color[1], color[2], color[3] } } } }); }
		inline void clearDepthStencil(bool clearDepth, bool clearStencil, float depth, uint32_t stencil) const { owner->push({ Opcode::ClearDepthStencilAttachment, { .opClearDepthStencilAttachment = { resourceId, clearDepth, clearStencil, depth, stencil } } }); }
		inline void transition(ResourceState oldState, ResourceState newState, uint32_t sourceStages, uint32_t destinationStages) const { owner->push({ Opcode::TransitionResource, { .opTransitionResource = { resourceId, ResourceKind::Framebuffer, oldState, newState, sourceStages, destinationStages } } }); }
	};

	/** @brief Move-only RAII proxy for a graphics pipeline handle. */
	class Pipeline : public ResourceProxyBase<Pipeline, Dispatcher> {
		typedef ResourceProxyBase<Pipeline, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::Pipeline, resourceId);
			owner->push({ Opcode::DestroyPipeline, { .opDestroyPipeline = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline Pipeline() = default;
		inline Pipeline(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		Pipeline(const Pipeline&)            = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		inline Pipeline(Pipeline&& other) noexcept = default;
		inline Pipeline& operator=(Pipeline&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~Pipeline() { destroyResource(); }
		inline void bind() const { owner->push({ Opcode::BindPipeline, { .opBindPipeline = { resourceId } } }); }
	};

	/** @brief Move-only RAII proxy for a compute pipeline handle. */
	class ComputePipeline : public ResourceProxyBase<ComputePipeline, Dispatcher> {
		typedef ResourceProxyBase<ComputePipeline, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::ComputePipeline, resourceId);
			owner->push({ Opcode::DestroyComputePipeline, { .opDestroyComputePipeline = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline ComputePipeline() = default;
		inline ComputePipeline(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		ComputePipeline(const ComputePipeline&)            = delete;
		ComputePipeline& operator=(const ComputePipeline&) = delete;
		inline ComputePipeline(ComputePipeline&& other) noexcept = default;
		inline ComputePipeline& operator=(ComputePipeline&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~ComputePipeline() { destroyResource(); }
		inline void bind() const { owner->push({ Opcode::BindComputePipeline, { .opBindComputePipeline = { resourceId } } }); }
	};

	/** @brief Move-only RAII proxy for a fence handle. */
	class Fence : public ResourceProxyBase<Fence, Dispatcher> {
		typedef ResourceProxyBase<Fence, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::Fence, resourceId);
			owner->push({ Opcode::DestroyFence, { .opDestroyFence = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline Fence() = default;
		inline Fence(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		Fence(const Fence&)            = delete;
		Fence& operator=(const Fence&) = delete;
		inline Fence(Fence&& other) noexcept = default;
		inline Fence& operator=(Fence&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~Fence() { destroyResource(); }
		inline void signal(uint64_t value) const { owner->push({ Opcode::SignalFence, { .opSignalFence = { resourceId, value } } }); }
		inline void wait(uint64_t value) const { owner->push({ Opcode::WaitFence, { .opWaitFence = { resourceId, value } } }); }
	};

	/** @brief Move-only RAII proxy for a query pool handle. */
	class QueryPool : public ResourceProxyBase<QueryPool, Dispatcher> {
		typedef ResourceProxyBase<QueryPool, Dispatcher> Base;
		using Base::moveAssign;
		using Base::owner;
		using Base::resourceId;
		friend Base;
		inline void destroyResource() {
			if (!owner || !resourceId) return;
			owner->freeId(ResourceHandleKind::QueryPool, resourceId);
			owner->push({ Opcode::DestroyQueryPool, { .opDestroyQueryPool = { resourceId } } });
			resourceId = 0;
		}
	public:
		inline QueryPool() = default;
		inline QueryPool(HandleId id, Dispatcher* dispatcher) : Base(id, dispatcher) {}
		QueryPool(const QueryPool&)            = delete;
		QueryPool& operator=(const QueryPool&) = delete;
		inline QueryPool(QueryPool&& other) noexcept = default;
		inline QueryPool& operator=(QueryPool&& other) noexcept { return moveAssign(std::move(other)); }
		inline ~QueryPool() { destroyResource(); }
		inline void reset(uint32_t firstQuery, uint32_t queryCount) const { owner->push({ Opcode::ResetQueryPool, { .opResetQueryPool = { resourceId, firstQuery, queryCount } } }); }
		inline void begin(uint32_t query) const { owner->push({ Opcode::BeginQuery, { .opBeginQuery = { resourceId, query } } }); }
		inline void end(uint32_t query) const { owner->push({ Opcode::EndQuery, { .opEndQuery = { resourceId, query } } }); }
		inline void writeTimestamp(uint32_t query, uint32_t pipelineStage) const { owner->push({ Opcode::WriteTimestamp, { .opWriteTimestamp = { resourceId, query, pipelineStage } } }); }
		inline void readResults(uint32_t firstQuery, uint32_t queryCount, QueryReadbackCallback callback, void* userData) const { owner->push({ Opcode::ReadQueryResults, { .opReadQueryResults = { resourceId, firstQuery, queryCount, callback, userData } } }); }
	};

	// ─────────────────────────────────────────────────────────────────────────
	// Recording surface
	//
	// Resource creation remains here because it allocates IDs. Commands that
	// naturally operate on a resource ID live on the resource proxy classes.
	// ─────────────────────────────────────────────────────────────────────────

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

	/** @name Swapchain helpers */
	///@{
	static inline void resizeSwapchain(Dispatcher& d, HandleId id, uint16_t width, uint16_t height, const SwapchainImageBinding* imageBindings = nullptr, uint32_t imageBindingCount = 0) { d.push({ Opcode::ResizeSwapchain, { .opResizeSwapchain = { id, width, height, imageBindings, imageBindingCount } } }); }
	static inline void acquireSwapchainImage(Dispatcher& d, HandleId swapchain, SwapchainAcquireCallback callback, void* userData) { d.push({ Opcode::AcquireSwapchainImage, { .opAcquireSwapchainImage = { swapchain, callback, userData } } }); }
	static inline void presentSwapchain(Dispatcher& d, HandleId swapchain = DefaultSwapchain, uint32_t imageIndex = 0) { d.push({ Opcode::PresentSwapchain, { .opPresentSwapchain = { swapchain, imageIndex } } }); }
	///@}

	/** @name Default framebuffer, render pass, and draw helpers */
	///@{
	static inline void beginFrame(Dispatcher& d, HandleId framebuffer = DefaultFramebuffer)    { d.push({ Opcode::BeginFrame,      { .opBeginFrame      = { framebuffer } } }); }
	static inline void endFrame(Dispatcher& d, HandleId framebuffer = DefaultFramebuffer)      { d.push({ Opcode::EndFrame,        { .opEndFrame        = { framebuffer } } }); }
	static inline void present(Dispatcher& d, HandleId framebuffer = DefaultFramebuffer)       { d.push({ Opcode::Present,         { .opPresent         = { framebuffer } } }); }
	static inline void beginRenderPass(Dispatcher& d, const RenderPassDescriptor* descriptor)  { d.push({ Opcode::BeginRenderPass, { .opBeginRenderPass = { descriptor } } }); }
	static inline void endRenderPass(Dispatcher& d)                                            { d.push({ Opcode::EndRenderPass,   { .opEndRenderPass   = { 0 } } }); }
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
	static inline void dispatchCompute(Dispatcher& d, uint32_t x, uint32_t y, uint32_t z)     { d.push({ Opcode::DispatchCompute,       { .opDispatchCompute      = { x, y, z } } }); }
	static inline void debugLabelBegin(Dispatcher& d, const char* name, const float (&color)[4]) { d.push({ Opcode::DebugLabelBegin, { .opDebugLabelBegin = { name, { color[0], color[1], color[2], color[3] } } } }); }
	static inline void debugLabelEnd(Dispatcher& d)                                            { d.push({ Opcode::DebugLabelEnd, { .opDebugLabelEnd = { 0 } } }); }
	static inline void debugMarker(Dispatcher& d, const char* name, const float (&color)[4])   { d.push({ Opcode::DebugMarker,   { .opDebugMarker   = { name, { color[0], color[1], color[2], color[3] } } } }); }
	///@}

};

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
		for (const DeferredDestroy& r : retirements) {
			destination.freeId(r.kind, r.id);
			switch (r.kind) {
				case ResourceHandleKind::Buffer:          destination.push({ Opcode::DestroyBufferObject,    { .opDestroy                = { r.id } } }); break;
				case ResourceHandleKind::Texture:         destination.push({ Opcode::DestroyTexture,         { .opDestroy                = { r.id } } }); break;
				case ResourceHandleKind::Sampler:         destination.push({ Opcode::DestroySampler,         { .opDestroy                = { r.id } } }); break;
				case ResourceHandleKind::Image:           destination.push({ Opcode::DestroyImage,           { .opDestroyImage           = { r.id } } }); break;
				case ResourceHandleKind::Framebuffer:     destination.push({ Opcode::DestroyFramebuffer,     { .opDestroyFramebuffer     = { r.id } } }); break;
				case ResourceHandleKind::Pipeline:        destination.push({ Opcode::DestroyPipeline,        { .opDestroyPipeline        = { r.id } } }); break;
				case ResourceHandleKind::ComputePipeline: destination.push({ Opcode::DestroyComputePipeline, { .opDestroyComputePipeline = { r.id } } }); break;
				case ResourceHandleKind::Fence:           destination.push({ Opcode::DestroyFence,           { .opDestroyFence           = { r.id } } }); break;
				case ResourceHandleKind::QueryPool:       destination.push({ Opcode::DestroyQueryPool,       { .opDestroyQueryPool       = { r.id } } }); break;
				case ResourceHandleKind::Swapchain:       destination.push({ Opcode::DestroySwapchain,       { .opDestroySwapchain       = { r.id } } }); break;
			}
		}
		clear();
	}
};

} // namespace Kld
#endif // KLDCOMMANDBUFFER_HPP
