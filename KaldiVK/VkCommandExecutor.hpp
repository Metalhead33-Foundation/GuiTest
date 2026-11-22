#ifndef KALDIVKCOMMANDEXECUTOR_HPP
#define KALDIVKCOMMANDEXECUTOR_HPP
/**
 * @file VkCommandExecutor.hpp
 * @brief Vulkan implementation of `Kld::Device::processCommandQueue()`.
 *
 * @section kaldivk_executor_model Execution model (bring-up, deliberately simple)
 *
 * The executor records into one primary command buffer and submits it
 * **synchronously**: `process()` returns only after `vkQueueWaitIdle`, unless a
 * frame started with `BeginFrame` is open, in which case the command buffer
 * stays open until `Present` (or the next `process()` call ends it). That keeps
 * handle/readback semantics identical to the GL backend without per-frame
 * fences, timeline semaphores or a retirement queue. The price is throughput:
 * every non-frame submission is a full round trip, which the first production
 * version should replace with frames-in-flight and deferred retirement (both are
 * already designed for in `Kaldi/README.md`).
 *
 * Because submissions are synchronous, destroy commands go into a deferred list
 * released after the queue is idle. Fence objects carry values but no GPU
 * timeline; `SignalFence`/`WaitFence` reduce to bookkeeping and say so.
 *
 * @section kaldivk_executor_memory Memory policy
 *
 * Every buffer comes from `HOST_VISIBLE | HOST_COHERENT` memory (falling back to
 * `HOST_VISIBLE | HOST_CACHED`) and stays mapped. Images are device-local;
 * uploads use temporary host-visible staging buffers recorded into the same
 * command buffer. A production backend should move static data to device-local
 * memory through a staging ring.
 *
 * @section kaldivk_executor_scope Implemented command scope
 *
 * Implemented: buffers (create/destroy/resize/update/map); 2D textures and
 * images (create/destroy/resize/update/readback, compressed uploads, blit-chain
 * mip generation); samplers; framebuffers; graphics pipelines from SPIR-V with
 * a pipeline-variant cache (topology, blend, depth/stencil, cull, depth bias);
 * compute pipelines and dispatch; texture/sampler/uniform/storage/image
 * bindings; every draw and indirect form (multi-draw becomes a loop); viewport
 * and scissor; clears; barriers/transitions; copies; the default swapchain path
 * (`BeginFrame`/`EndFrame`/`Present`); occlusion and timestamp queries; and the
 * default-swapchain acquire/present callbacks.
 *
 * Deliberately unsupported (`HalStatusCode::UnsupportedBaseline`): explicit
 * swapchain objects, cube/array/3D/multisampled textures, base-instance draws
 * (Vulkan 1.0 has no base-instance draw parameter without
 * `VK_KHR_shader_draw_parameters`), stencil-only readback and debug labels (the
 * context enables no debug-marker extension yet). Limits come from
 * `Vk::probeCapabilities()`.
 */
#include "VkCapabilities.hpp"
#include "VkContext.hpp"
#include "VkUtil.hpp"
#include <Kaldi/KldHal.hpp>
#include <Kaldi/KaldiOperation.hpp>
#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Vk {

/** @brief One device-local image plus its view and tracked layout. */
struct VkTextureObject {
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkFormat format = VK_FORMAT_UNDEFINED;
	Kld::TextureFormat euphemyFormat {};
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	std::uint32_t levels = 1;
	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

/** @brief One host-visible buffer kept mapped for its whole life. */
struct VkBufferObject {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	void* mapped = nullptr;
	std::uint32_t size = 0;
};

/** @brief One sampler object. */
struct VkSamplerObject {
	VkSampler sampler = VK_NULL_HANDLE;
};

/** @brief One render pass + framebuffer pair built from image attachments. */
struct VkFramebufferObject {
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
};

/** @brief A descriptor slot declared by a pipeline's binding layout. */
struct VkDescriptorSlot {
	std::uint32_t binding = 0;
	VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
	std::uint32_t count = 1;
	VkShaderStageFlags stages = 0;
};

/** @brief Everything that affects a baked graphics pipeline. */
struct VkPipelineState {
	bool lines = false;
	bool blendEnabled = false;
	VkBlendFactor srcColor = VK_BLEND_FACTOR_ONE;
	VkBlendFactor dstColor = VK_BLEND_FACTOR_ZERO;
	VkBlendFactor srcAlpha = VK_BLEND_FACTOR_ONE;
	VkBlendFactor dstAlpha = VK_BLEND_FACTOR_ZERO;
	VkBlendOp colorOp = VK_BLEND_OP_ADD;
	VkBlendOp alphaOp = VK_BLEND_OP_ADD;
	VkColorComponentFlags colorMask = 0xF;
	bool depthTest = false;
	bool depthWrite = false;
	VkCompareOp depthCompare = VK_COMPARE_OP_ALWAYS;
	bool stencilTest = false;
	VkStencilOpState front {};
	VkStencilOpState back {};
	VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
	VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	bool depthClamp = false;
	bool depthBias = false;
	float biasConstant = 0.0f;
	float biasClamp = 0.0f;
	float biasSlope = 0.0f;
	bool hasDepth = false;

	/** @brief Lexicographic key for the variant cache. */
	auto asTuple() const noexcept
	{
		return std::tie(lines, blendEnabled, srcColor, dstColor, srcAlpha, dstAlpha, colorOp,
				alphaOp, colorMask, depthTest, depthWrite, depthCompare, stencilTest,
				front.failOp, front.passOp, front.depthFailOp, front.compareOp,
				front.compareMask, front.writeMask, front.reference, back.failOp,
				back.passOp, back.depthFailOp, back.compareOp, back.compareMask,
				back.writeMask, back.reference, cullMode, frontFace, depthClamp,
				depthBias, biasConstant, biasClamp, biasSlope, hasDepth);
	}
	bool operator<(const VkPipelineState& other) const noexcept
	{
		return asTuple() < other.asTuple();
	}
};

/** @brief One graphics or compute pipeline together with its variants. */
struct VkPipelineObject {
	VkPipelineLayout layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	std::vector<VkDescriptorSlot> slots;
	std::vector<std::vector<std::uint32_t>> spirv;
	std::vector<VkShaderStageFlagBits> stages;
	std::vector<Kld::AttributeDescriptor> attributes;
	std::size_t stride = 0;
	std::map<VkRenderPass, std::map<VkPipelineState, VkPipeline>> variants;
	VkPipeline computePipeline = VK_NULL_HANDLE;
	std::string name;
};

/** @brief A query pool with its Kaldi query type. */
struct VkQueryPoolObject {
	VkQueryPool pool = VK_NULL_HANDLE;
	Kld::QueryType type = Kld::QueryType::Occlusion;
	std::uint32_t count = 0;
};

/**
 * @brief Executes Kaldi command queues on a `Vk::Context`.
 */
class CommandExecutor {
public:
	CommandExecutor(Context& context, const Kld::HalCapabilities& capabilities);
	~CommandExecutor();

	CommandExecutor(const CommandExecutor&) = delete;
	CommandExecutor& operator=(const CommandExecutor&) = delete;

	/** @brief Executes @p commands; see the file comment for the submission model. */
	Kld::HalStatus process(std::span<const Kld::GfxOp> commands);

	/** @brief Releases every resource and swapchain object. Requires an idle device. */
	void releaseAll() noexcept;

	/** @brief Number of commands executed since construction. */
	std::uint64_t executedCommands() const noexcept { return executedCommands_; }

private:
	Kld::HalStatus createBuffer(const Kld::OpCreateBufferObject&);
	Kld::HalStatus destroyBuffer(const Kld::OpDestroy&);
	Kld::HalStatus resizeBuffer(const Kld::OpResizeBufferObject&);
	Kld::HalStatus updateBuffer(const Kld::OpUpdateBufferObject&);
	Kld::HalStatus mapBuffer(const Kld::OpMapBufferObject&);
	Kld::HalStatus createVertexArray(const Kld::OpCreateVertexArrayObject&);
	Kld::HalStatus createTexture1D(const Kld::OpCreateTexture1D&);
	Kld::HalStatus createTexture2D(const Kld::OpCreateTexture2D&);
	Kld::HalStatus createCompressedTexture2D(const Kld::OpCreateCompressedTexture2D&);
	Kld::HalStatus createTextureFromDecodeTarget(const Kld::OpCreateTexture2DFromDecodeTarget&);
	Kld::HalStatus createTexture3D(const Kld::OpCreateTexture3D&);
	Kld::HalStatus createTextureCube(const Kld::OpCreateTextureCube&);
	Kld::HalStatus createTexture2DArray(const Kld::OpCreateTexture2DArray&);
	Kld::HalStatus destroyTexture(const Kld::OpDestroy&);
	Kld::HalStatus resizeTexture2D(const Kld::OpResizeTexture2D&);
	Kld::HalStatus updateTexture2D(const Kld::OpUpdateTexture2D&);
	Kld::HalStatus updateCompressedTexture2D(const Kld::OpUpdateCompressedTexture2D&);
	Kld::HalStatus updateTextureCubeFace(const Kld::OpUpdateTextureCubeFace&);
	Kld::HalStatus updateTexture2DArray(const Kld::OpUpdateTexture2DArray&);
	Kld::HalStatus blitTexture2D(const Kld::OpBlitTexture2D&);
	Kld::HalStatus generateTextureMipmaps(const Kld::OpGenerateTextureMipmaps&);
	Kld::HalStatus createSampler(const Kld::OpCreateSampler&);
	Kld::HalStatus destroySampler(const Kld::OpDestroy&);
	Kld::HalStatus createImage2D(const Kld::OpCreateImage2D&);
	Kld::HalStatus resizeImage2D(const Kld::OpResizeImage2D&);
	Kld::HalStatus createMultisampledImage2D(const Kld::OpCreateMultisampledImage2D&);
	Kld::HalStatus updateImage2D(const Kld::OpUpdateImage2D&);
	Kld::HalStatus blitImage2D(const Kld::OpBlitImage2D&);
	Kld::HalStatus resolveImage2D(const Kld::OpResolveImage2D&);
	Kld::HalStatus destroyImage(const Kld::OpDestroyImage&);
	Kld::HalStatus createFramebuffer(const Kld::OpCreateFramebuffer&);
	Kld::HalStatus destroyFramebuffer(const Kld::OpDestroyFramebuffer&);
	Kld::HalStatus bindFramebuffer(const Kld::OpBindFramebuffer&);
	Kld::HalStatus beginFrame(const Kld::OpBeginFrame&);
	Kld::HalStatus endFrame(const Kld::OpEndFrame&);
	Kld::HalStatus clearColorAttachment(const Kld::OpClearColorAttachment&);
	Kld::HalStatus clearDepthStencilAttachment(const Kld::OpClearDepthStencilAttachment&);
	Kld::HalStatus present(const Kld::OpPresent&);
	Kld::HalStatus createPipeline(const Kld::OpCreatePipeline&);
	Kld::HalStatus createComputePipeline(const Kld::OpCreateComputePipeline&);
	Kld::HalStatus destroyPipeline(const Kld::OpDestroyPipeline&);
	Kld::HalStatus destroyComputePipeline(const Kld::OpDestroyComputePipeline&);
	Kld::HalStatus bindPipeline(const Kld::OpBindPipeline&);
	Kld::HalStatus bindComputePipeline(const Kld::OpBindComputePipeline&);
	Kld::HalStatus bindTexture(const Kld::OpBindTexture&);
	Kld::HalStatus bindSampler(const Kld::OpBindSampler&);
	Kld::HalStatus bindUniformBuffer(const Kld::OpBindUniformBuffer&);
	Kld::HalStatus bindStorageBuffer(const Kld::OpBindStorageBuffer&);
	Kld::HalStatus bindStorageImage(const Kld::OpBindStorageImage&);
	Kld::HalStatus bindVertexBuffer(const Kld::OpBindVertexBuffer&);
	Kld::HalStatus bindIndexBuffer(const Kld::OpBindIndexBuffer&);
	Kld::HalStatus drawTriangles(const Kld::OpDrawTriangles&);
	Kld::HalStatus drawIndexedTriangles(const Kld::OpDrawIndexedTriangles&);
	Kld::HalStatus drawLines(const Kld::OpDrawLines&);
	Kld::HalStatus drawIndexedLines(const Kld::OpDrawIndexedLines&);
	Kld::HalStatus drawTrianglesInstanced(const Kld::OpDrawTrianglesInstanced&);
	Kld::HalStatus drawIndexedTrianglesInstanced(const Kld::OpDrawIndexedTrianglesInstanced&);
	Kld::HalStatus drawLinesInstanced(const Kld::OpDrawLinesInstanced&);
	Kld::HalStatus drawIndexedLinesInstanced(const Kld::OpDrawIndexedLinesInstanced&);
	Kld::HalStatus drawIndirect(const Kld::OpDrawIndirect&);
	Kld::HalStatus drawIndexedIndirect(const Kld::OpDrawIndexedIndirect&);
	Kld::HalStatus setViewport(const Kld::OpSetViewport&);
	Kld::HalStatus setScissor(const Kld::OpSetScissor&);
	Kld::HalStatus setBlendState(const Kld::OpSetBlendState&);
	Kld::HalStatus setDepthStencilState(const Kld::OpSetDepthStencilState&);
	Kld::HalStatus setRasterizerState(const Kld::OpSetRasterizerState&);
	Kld::HalStatus transitionResource(const Kld::OpTransitionResource&);
	Kld::HalStatus bufferBarrier(const Kld::OpBufferBarrier&);
	Kld::HalStatus imageBarrier(const Kld::OpImageBarrier&);
	Kld::HalStatus createFence(const Kld::OpCreateFence&);
	Kld::HalStatus destroyFence(const Kld::OpDestroyFence&);
	Kld::HalStatus signalFence(const Kld::OpSignalFence&);
	Kld::HalStatus waitFence(const Kld::OpWaitFence&);
	Kld::HalStatus dispatchCompute(const Kld::OpDispatchCompute&);
	Kld::HalStatus dispatchComputeIndirect(const Kld::OpDispatchComputeIndirect&);
	Kld::HalStatus createQueryPool(const Kld::OpCreateQueryPool&);
	Kld::HalStatus destroyQueryPool(const Kld::OpDestroyQueryPool&);
	Kld::HalStatus resetQueryPool(const Kld::OpResetQueryPool&);
	Kld::HalStatus beginQuery(const Kld::OpBeginQuery&);
	Kld::HalStatus endQuery(const Kld::OpEndQuery&);
	Kld::HalStatus writeTimestamp(const Kld::OpWriteTimestamp&);
	Kld::HalStatus readQueryResults(const Kld::OpReadQueryResults&);
	Kld::HalStatus copyBuffer(const Kld::OpCopyBuffer&);
	Kld::HalStatus copyBufferToImage2D(const Kld::OpCopyBufferToImage2D&);
	Kld::HalStatus copyImage2DToBuffer(const Kld::OpCopyImage2DToBuffer&);
	Kld::HalStatus copyImage2D(const Kld::OpCopyImage2D&);
	Kld::HalStatus readBuffer(const Kld::OpReadBuffer&);
	Kld::HalStatus readImage2D(const Kld::OpReadImage2D&);
	Kld::HalStatus debugLabelBegin(const Kld::OpDebugLabelBegin&);
	Kld::HalStatus debugLabelEnd(const Kld::OpDebugLabelEnd&);
	Kld::HalStatus debugMarker(const Kld::OpDebugMarker&);
	Kld::HalStatus createSwapchain(const Kld::OpCreateSwapchain&);
	Kld::HalStatus destroySwapchain(const Kld::OpDestroySwapchain&);
	Kld::HalStatus resizeSwapchain(const Kld::OpResizeSwapchain&);
	Kld::HalStatus acquireSwapchainImage(const Kld::OpAcquireSwapchainImage&);
	Kld::HalStatus presentSwapchain(const Kld::OpPresentSwapchain&);
	Kld::HalStatus beginRenderPass(const Kld::OpBeginRenderPass&);
	Kld::HalStatus endRenderPass(const Kld::OpEndRenderPass&);

	// Recording, submission and helpers.
	Kld::HalStatus fail(Kld::HalStatusCode code, std::string message);
	Kld::HalStatus failUnsupported(std::string message);
	Kld::HalStatus failInvalidHandle(std::string_view kind, Kld::HandleId id);
	bool isHandleInUse(Kld::HandleId id) const;

	VkBufferObject* buffer(Kld::HandleId id);
	VkTextureObject* texture(Kld::HandleId id);
	VkTextureObject* image(Kld::HandleId id);
	VkSamplerObject* sampler(Kld::HandleId id);
	VkFramebufferObject* framebuffer(Kld::HandleId id);
	VkPipelineObject* pipeline(Kld::HandleId id);
	VkPipelineObject* computePipeline(Kld::HandleId id);
	VkQueryPoolObject* queryPool(Kld::HandleId id);

	Kld::HalStatus ensureRecording();
	Kld::HalStatus submitAndWait();
	Kld::HalStatus flushForHostAccess();
	void flushDeferred() noexcept;
	std::uint32_t findMemoryType(std::uint32_t typeBits, VkMemoryPropertyFlags properties) const;

	Kld::HalStatus ensureImageLayout(VkTextureObject& object);
	Kld::HalStatus createImageStorage(VkTextureObject& object, VkImageUsageFlags usage);
	Kld::HalStatus uploadImage(VkTextureObject& object, const void* data, std::uint32_t byteSize,
				   std::uint32_t rowPitch, std::uint32_t levelWidth,
				   std::uint32_t levelHeight);
	Kld::HalStatus generateMips(VkTextureObject& object);
	Kld::HalStatus blitImages(const VkTextureObject& source, std::uint16_t sourceX,
				  std::uint16_t sourceY, const VkTextureObject& destination,
				  std::uint16_t destinationX, std::uint16_t destinationY,
				  std::uint16_t width, std::uint16_t height);

	Kld::HalStatus createPipelineLayout(VkPipelineObject& object,
					    const Kld::PipelineBindingLayout* layout);
	Kld::HalStatus createGraphicsVariant(VkPipelineObject& object, VkRenderPass renderPass,
					     const VkPipelineState& state, VkPipeline& pipeline);
	Kld::HalStatus createComputeObject(VkPipelineObject& object);
	Kld::HalStatus updateDescriptors(VkPipelineObject& object);
	Kld::HalStatus prepareDraw(VkPipelineObject*& object, bool lines);
	VkStencilOpState toStencilState(const Kld::StencilFaceState& state) const;
	VkPipelineState currentPipelineState(bool lines) const;

	Kld::HalStatus ensureSwapchain();
	Kld::HalStatus createSwapchainObjects();
	void destroySwapchainObjects() noexcept;
	Kld::HalStatus acquireDefaultImage();
	Kld::HalStatus ensureRenderPass(Kld::HandleId framebufferId);
	void endActiveRenderPass();
	Kld::HalStatus applyClear(const Kld::OpClearColorAttachment& op);
	Context& context_;
	Kld::HalCapabilities capabilities_;
	std::string message_;
	std::uint64_t executedCommands_ = 0;

	VkCommandPool commandPool_ = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
	bool recording_ = false;

	struct DeferredDestroy {
		VkBuffer buffer_ = VK_NULL_HANDLE;
		VkDeviceMemory bufferMemory_ = VK_NULL_HANDLE;
		VkImage image_ = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory_ = VK_NULL_HANDLE;
		VkImageView view_ = VK_NULL_HANDLE;
		VkSampler sampler_ = VK_NULL_HANDLE;
		VkFramebuffer framebuffer_ = VK_NULL_HANDLE;
		VkRenderPass renderPass_ = VK_NULL_HANDLE;
		VkPipeline pipeline_ = VK_NULL_HANDLE;
		VkPipelineLayout layout_ = VK_NULL_HANDLE;
		VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
		VkQueryPool queryPool_ = VK_NULL_HANDLE;
	};
	std::vector<DeferredDestroy> deferred_;

	std::unordered_map<Kld::HandleId, VkBufferObject> buffers_;
	std::unordered_map<Kld::HandleId, VkTextureObject> textures_;
	std::unordered_map<Kld::HandleId, VkTextureObject> images_;
	std::unordered_map<Kld::HandleId, VkSamplerObject> samplers_;
	std::unordered_map<Kld::HandleId, VkFramebufferObject> framebuffers_;
	std::unordered_map<Kld::HandleId, VkPipelineObject> pipelines_;
	std::unordered_map<Kld::HandleId, VkPipelineObject> computePipelines_;
	std::unordered_map<Kld::HandleId, std::uint64_t> fences_;
	std::unordered_map<Kld::HandleId, VkQueryPoolObject> queryPools_;
	std::unordered_map<Kld::HandleId, std::uint64_t> vertexArrays_;

	struct VertexBinding {
		Kld::HandleId buffer = 0;
		std::uint64_t offset = 0;
		bool valid = false;
	};
	struct IndexBinding {
		Kld::HandleId buffer = 0;
		std::uint64_t offset = 0;
		VkIndexType type = VK_INDEX_TYPE_UINT16;
		bool valid = false;
	};
	std::vector<VertexBinding> vertexBindings_;
	IndexBinding indexBinding_;
	Kld::HandleId boundTextures_[64] {};
	Kld::HandleId boundSamplers_[64] {};
	Kld::HandleId boundUniformBuffers_[32] {};
	Kld::HandleId boundStorageBuffers_[32] {};
	Kld::HandleId boundStorageImages_[32] {};
	Kld::HandleId boundPipelineId_ = 0;
	Kld::HandleId boundComputePipelineId_ = 0;
	bool computeBound_ = false;
	Kld::HandleId boundFramebuffer_ = Kld::DefaultFramebuffer;
	VkPipelineState state_;
	VkViewport viewport_ {};
	VkRect2D scissor_ {};
	bool viewportSet_ = false;
	bool scissorSet_ = false;

	VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
	std::vector<VkImage> swapchainImages_;
	std::vector<VkImageView> swapchainViews_;
	std::vector<VkFramebuffer> swapchainFramebuffers_;
	VkRenderPass swapchainRenderPass_ = VK_NULL_HANDLE;
	VkFormat swapchainFormat_ = VK_FORMAT_UNDEFINED;
	VkExtent2D swapchainExtent_ {};
	VkFence acquireFence_ = VK_NULL_HANDLE;
	std::uint32_t currentImageIndex_ = 0;
	bool imageAcquired_ = false;
	bool frameActive_ = false;

	VkRenderPass activePass_ = VK_NULL_HANDLE;
	VkFramebuffer activeFramebuffer_ = VK_NULL_HANDLE;

	VkTextureObject dummyTexture_;
	VkSampler defaultSampler_ = VK_NULL_HANDLE;

};

}
#endif // KALDIVKCOMMANDEXECUTOR_HPP
