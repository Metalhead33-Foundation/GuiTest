#ifndef KLDCOMMANDBUFFER_HPP
#define KLDCOMMANDBUFFER_HPP
/**
 * @file KldCommandBuffer.hpp
 * @brief Header-only object-oriented facade over Kaldi VM command buffers.
 *
 * This facade is a convenience layer over `GfxOp`. It does not add a new
 * backend contract: every helper simply appends one raw command payload to
 * client-owned storage. Resource proxy classes are move-only handles that record
 * typed destroy commands into their owning `CommandBuffer` when destroyed.
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
 * `CommandBuffer` is single-threaded. Recording, clearing, submitting, and
 * RAII proxy destruction all mutate or inspect the same command storage. Use
 * one command buffer per recording thread, optionally backed by a shared
 * `HandleAllocator`, then merge streams with `mergeCommandBuffers` in an
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

template <typename HandleAllocator = std::allocator<HandleId>> class HandleStorage {
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
	{
	}
	inline explicit HandleStorage(size_t reservedbufferIdsFreelistSize,
						   size_t reservedtextureIdsFreelistSize,
						   size_t reservedsamplerIdsFreelistSize,
						   size_t reservedimageIdsFreelistSize,
						   size_t reservedframebufferIdsFreelistSize,
						   size_t reservedpipelineIdsFreelistSize,
						   size_t reservedcomputePipelineIdsFreelistSize,
						   size_t reservedfenceIdsFreelistSize,
						   size_t reservedqueryPoolIdsFreelistSize,
						   size_t reservedswapchainIdsFreelistSize, const HandleAllocator& handleAlloc = HandleAllocator())
		: bufferIds(reservedbufferIdsFreelistSize, handleAlloc),
		textureIds(reservedtextureIdsFreelistSize, handleAlloc),
		samplerIds(reservedsamplerIdsFreelistSize, handleAlloc),
		imageIds(reservedimageIdsFreelistSize, handleAlloc),
		framebufferIds(reservedframebufferIdsFreelistSize, handleAlloc),
		pipelineIds(reservedpipelineIdsFreelistSize, handleAlloc),
		computePipelineIds(reservedcomputePipelineIdsFreelistSize, handleAlloc),
		fenceIds(reservedfenceIdsFreelistSize, handleAlloc),
		queryPoolIds(reservedqueryPoolIdsFreelistSize, handleAlloc),
		swapchainIds(reservedswapchainIdsFreelistSize, handleAlloc)
	{
	}
	[[nodiscard]] inline HandleId acquireId(ResourceHandleKind kind) {
		switch (kind) {
			case ResourceHandleKind::Buffer: return bufferIds.acquireId();
			case ResourceHandleKind::Texture: return textureIds.acquireId();
			case ResourceHandleKind::Sampler: return samplerIds.acquireId();
			case ResourceHandleKind::Image: return imageIds.acquireId();
			case ResourceHandleKind::Framebuffer: return framebufferIds.acquireId();
			case ResourceHandleKind::Pipeline: return pipelineIds.acquireId();
			case ResourceHandleKind::ComputePipeline: return computePipelineIds.acquireId();
			case ResourceHandleKind::Fence: return fenceIds.acquireId();
			case ResourceHandleKind::QueryPool: return queryPoolIds.acquireId();
			case ResourceHandleKind::Swapchain: return swapchainIds.acquireId();
			default: return 0;
		}
	}
	inline void acquireIds(ResourceHandleKind kind, HandleId* ids, size_t idCount) {
		switch (kind) {
			case ResourceHandleKind::Buffer: return bufferIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Texture: return textureIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Sampler: return samplerIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Image: return imageIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Framebuffer: return framebufferIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Pipeline: return pipelineIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::ComputePipeline: return computePipelineIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Fence: return fenceIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::QueryPool: return queryPoolIds.acquireMultiple(ids, idCount);
			case ResourceHandleKind::Swapchain: return swapchainIds.acquireMultiple(ids, idCount);
			default: return;
		}
	}
	template <typename Function> inline void acquireIds(ResourceHandleKind kind, Function&& fun, size_t idCount) {
		switch (kind) {
			case ResourceHandleKind::Buffer: return bufferIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::Texture: return textureIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::Sampler: return samplerIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::Image: return imageIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::Framebuffer: return framebufferIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::Pipeline: return pipelineIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::ComputePipeline: return computePipelineIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::Fence: return fenceIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::QueryPool: return queryPoolIds.acquireMultiple(fun, idCount);
			case ResourceHandleKind::Swapchain: return swapchainIds.acquireMultiple(fun, idCount);
			default: return;
		}
	}
	inline void freeId(ResourceHandleKind kind, HandleId id) {
		switch (kind) {
			case ResourceHandleKind::Buffer: bufferIds.release(id); break;
			case ResourceHandleKind::Texture: textureIds.release(id); break;
			case ResourceHandleKind::Sampler: samplerIds.release(id); break;
			case ResourceHandleKind::Image: imageIds.release(id); break;
			case ResourceHandleKind::Framebuffer: framebufferIds.release(id); break;
			case ResourceHandleKind::Pipeline: pipelineIds.release(id); break;
			case ResourceHandleKind::ComputePipeline: computePipelineIds.release(id); break;
			case ResourceHandleKind::Fence: fenceIds.release(id); break;
			case ResourceHandleKind::QueryPool: queryPoolIds.release(id); break;
			case ResourceHandleKind::Swapchain: swapchainIds.release(id); break;
			default: return;
		}
	}
	inline void freeIds(ResourceHandleKind kind, const HandleId* ids, size_t idCount) {
		switch (kind) {
			case ResourceHandleKind::Buffer: bufferIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Texture: textureIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Sampler: samplerIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Image: imageIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Framebuffer: framebufferIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Pipeline: pipelineIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::ComputePipeline: computePipelineIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Fence: fenceIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::QueryPool: queryPoolIds.releaseMultiple(ids, idCount); break;
			case ResourceHandleKind::Swapchain: swapchainIds.releaseMultiple(ids, idCount); break;
			default: return;
		}
	}
};

template <typename HandleAllocator = std::allocator<HandleId>, typename CommandAllocator = std::allocator<GfxOp>>
class CommandDispatcher : public HandleStorage<HandleAllocator> {
public:
	typedef Elv::Util::DoubleBufferedVector<GfxOp,CommandAllocator> CommandBuffer;
	typedef HandleStorage<HandleAllocator> HandleStore;
	typedef std::span<GfxOp> CommandSpan;
	typedef std::span<const GfxOp> ConstCommandSpan;
private:
	mutable std::mutex mutex;
	CommandBuffer cmdBuff;
public:
	explicit CommandDispatcher(const HandleAllocator& handleAlloc = HandleAllocator(), const CommandAllocator& commandAlloc = CommandAllocator())
		: HandleStore(handleAlloc), cmdBuff(commandAlloc)
	{}
	explicit CommandDispatcher(size_t reservedCommandBufferSize,
						   size_t reservedbufferIdsFreelistSize,
						   size_t reservedtextureIdsFreelistSize,
						   size_t reservedsamplerIdsFreelistSize,
						   size_t reservedimageIdsFreelistSize,
						   size_t reservedframebufferIdsFreelistSize,
						   size_t reservedpipelineIdsFreelistSize,
						   size_t reservedcomputePipelineIdsFreelistSize,
						   size_t reservedfenceIdsFreelistSize,
						   size_t reservedqueryPoolIdsFreelistSize,
						   size_t reservedswapchainIdsFreelistSize, const HandleAllocator& handleAlloc = HandleAllocator(),
							   const CommandAllocator& commandAlloc = CommandAllocator())
		: HandleStore(reservedbufferIdsFreelistSize, reservedtextureIdsFreelistSize, reservedsamplerIdsFreelistSize, reservedimageIdsFreelistSize,
			reservedframebufferIdsFreelistSize, reservedpipelineIdsFreelistSize, reservedcomputePipelineIdsFreelistSize, reservedfenceIdsFreelistSize,
					  reservedqueryPoolIdsFreelistSize, reservedswapchainIdsFreelistSize, handleAlloc), cmdBuff(reservedCommandBufferSize,commandAlloc)
	{
	}
	const CommandBuffer& getCommandBuffer() const { return cmdBuff; }
	CommandBuffer& getCommandBuffer() { return cmdBuff; }
};

#define OOP_DETAIL_HEADER template <typename HandleAllocator = std::allocator<HandleId>, typename CommandAllocator = std::allocator<GfxOp>>

OOP_DETAIL_HEADER struct oop_detail {
	typedef HandleAllocator IdAlloc;
	typedef CommandAllocator CommandAlloc;
	typedef CommandDispatcher<IdAlloc,CommandAlloc> Dispatcher;

	/** @brief Move-only RAII proxy for a buffer handle. */
	class Buffer {
		//
	};
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
};



}
#endif // KLDCOMMANDBUFFER_HPP
