#ifndef KALDIVKCOMMANDEXECUTOR_HPP
#define KALDIVKCOMMANDEXECUTOR_HPP
/**
 * @file VkCommandExecutor.hpp
 * @brief Vulkan implementation of `Kld::Device::processCommandQueue()`.
 *
 * @section kaldivk_executor_model Execution model
 *
 * The executor keeps **three frames in flight**. `process()` records into the
 * current frame slot's command buffer and submits it when the call ends (or when a
 * frame is presented) **without waiting**; work is awaited only when a slot is
 * about to be reused, or when a command needs host-visible results (`MapBuffer`,
 * readbacks, query results, explicit barriers around a swapchain recreate).
 *
 * Each slot owns its command pool, command buffer, submit fence, a descriptor-pool
 * ring and its own deferred-destruction list, so a slot's retired objects are freed
 * as soon as its fence signals instead of when the device goes idle. A frame opened
 * with `BeginFrame` keeps its command buffer open until `Present`; a command that
 * needs host results mid-frame flushes and re-opens it, which is why the swapchain
 * render passes use `LOAD`/`STORE` and keep a stable attachment layout.
 *
 * @section kaldivk_executor_memory Memory policy
 *
 * Every buffer comes from `HOST_VISIBLE | HOST_COHERENT` memory (falling back to
 * `HOST_VISIBLE | HOST_CACHED`) and stays mapped. Images are device-local;
 * uploads use temporary host-visible staging buffers recorded into the same
 * command buffer. A production backend should move static data to device-local
 * memory through a staging ring; buffers stay host-visible because `MapBuffer`
 * hands the mapping to the caller.
 *
 * @section kaldivk_executor_scope Implemented command scope
 *
 * Implemented: buffers (create/destroy/resize/update/map); 1D, 2D, 3D, array and
 * cube textures plus 2D, multisampled and depth/stencil images
 * (create/destroy/resize/update/readback, compressed uploads, blit-chain mip
 * generation, `vkCmdResolveImage` resolves); samplers; framebuffers - including
 * multisampled attachments, each with its own render pass and sample count -
 * graphics pipelines from SPIR-V with a pipeline-variant cache and one descriptor
 * set per binding kind; compute pipelines and dispatch; texture/sampler/uniform/
 * storage-buffer/storage-image bindings; every draw and indirect form; viewport and
 * scissor; clears; barriers/transitions; buffer and image copies; occlusion and
 * timestamp queries; debug labels (`VK_EXT_debug_utils`); explicit swapchain
 * objects; and the default swapchain path (`BeginFrame`/`EndFrame`/`Present`).
 *
 * Deliberately unsupported (`HalStatusCode::UnsupportedBaseline`): base-instance
 * draws (Vulkan 1.0 has no base-instance draw parameter without
 * `VK_KHR_shader_draw_parameters`), vertex instance divisors above 1
 * (`VK_EXT_vertex_attribute_divisor`), per-sampler LOD bias (a Vulkan 1.1 feature),
 * mip generation for depth/stencil and block-compressed images, blits and readback
 * of depth/stencil images, readback of block-compressed images, block-compressed
 * uploads whose extents are not multiples of four, and explicit swapchains with a
 * depth attachment or caller-declared images. Limits come from
 * `Vk::probeCapabilities()`.
 */
#include "VkCapabilities.hpp"
#include "VkContext.hpp"
#include "VkUtil.hpp"
#include <Kaldi/KldHal.hpp>
#include <Kaldi/KaldiOperation.hpp>
#include <array>
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
	VkImageType imageType = VK_IMAGE_TYPE_2D;
	VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
	std::uint32_t width = 0;
	std::uint32_t height = 1;
	/** @brief Mip-0 depth of a 3D image, or 1. */
	std::uint32_t depth = 1;
	/** @brief Array layers, or 6 for a cube map. */
	std::uint32_t layers = 1;
	std::uint32_t levels = 1;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageUsageFlags usage = 0;
	/** @brief True for BC1/BC2/BC3 storage (row pitch is measured in blocks). */
	bool compressed = false;
	/** @brief True when the image was created with `CUBE_COMPATIBLE`. */
	bool cube = false;
};

/** @brief One host-visible buffer kept mapped for its whole life. */
struct VkBufferObject {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	void* mapped = nullptr;
	std::uint32_t size = 0;
	/** @brief True when the memory type is `HOST_COHERENT` (no flush needed). */
	bool coherent = true;
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
	bool hasDepth = false;
	bool hasStencil = false;
	/** @brief Sample count of every attachment (Vulkan needs one value). */
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	/**
	 * @brief True when the executor owns this framebuffer on behalf of a swapchain.
	 *
	 * Such entries are published under a handle from the backend-reserved range
	 * (`Kld::BackendHandleBase` upwards) so an acquire callback can hand the
	 * caller something usable with `BindFramebuffer`/`BeginRenderPass`. The
	 * caller must not destroy them: `DestroyFramebuffer` rejects them, and
	 * `DestroySwapchain` retires them.
	 */
	bool backendOwned = false;
};

/**
 * @brief One explicit swapchain: images, views, backend-owned framebuffers.
 *
 * Vulkan owns the presentable images, so they are never exposed as `Image`
 * handles; instead every image gets a framebuffer handle from the
 * backend-reserved range, which is what `SwapchainAcquireInfo` reports. That is
 * the whole draw path of an explicit swapchain: acquire, bind the reported
 * framebuffer, draw, present by image index.
 *
 * The render pass mirrors the default swapchain pass: one color attachment that
 * stays in `COLOR_ATTACHMENT_OPTIMAL` with `LOAD`/`STORE`. The acquire transitions
 * the image in and the present transitions it back to `PRESENT_SRC_KHR`, which is
 * what keeps a re-begun pass valid.
 */
struct VkSwapchainObject {
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkFormat format = VK_FORMAT_UNDEFINED;
	VkExtent2D extent {};
	std::vector<VkImage> images;
	std::vector<VkImageView> views;
	std::vector<VkFramebuffer> framebuffers;
	/** @brief Handles reported through `SwapchainAcquireInfo::framebuffer`. */
	std::vector<Kld::HandleId> framebufferHandles;
	Kld::PresentMode presentMode = Kld::PresentMode::Fifo;
	std::uint32_t currentImage = 0;
	bool acquired = false;
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

/**
 * @brief One graphics or compute pipeline together with its variants.
 *
 * @section kaldivk_pipeline_sets Descriptor sets
 *
 * Vulkan gives a descriptor set one namespace for all binding kinds, while Kaldi
 * numbers each kind independently (texture unit 0, sampler unit 0, uniform block
 * 0, ...). The executor therefore maps each kind to its own set index, with the
 * portable slot number as the binding inside that set:
 *
 * | Set | Kaldi binding kind                         |
 * |-----|--------------------------------------------|
 * | 0   | sampled textures (`BindTexture`)           |
 * | 1   | samplers (`BindSampler`)                   |
 * | 2   | uniform buffers (`BindUniformBuffer`)      |
 * | 3   | storage buffers (`BindStorageBuffer`)      |
 * | 4   | storage images (`BindStorageImage`)        |
 *
 * That is exactly how compiled HLSL declares them - `[[vk::binding(0, 0)]]` for a
 * `Texture2D`, `[[vk::binding(0, 1)]]` for its `SamplerState`, `[[vk::binding(0, 2)]]`
 * for a `cbuffer` - so no renumbering of the shader is needed and two kinds can
 * never collide on one binding.
 */
struct VkPipelineObject {
	VkPipelineLayout layout = VK_NULL_HANDLE;
	std::array<VkDescriptorSetLayout, 5> setLayouts {};
	/**
	 * @brief Descriptor types/counts this pipeline needs from a pool.
	 *
	 * Sets themselves come from the frame slot's descriptor ring (see
	 * @ref FrameSlot), because a set must not be reset while the submission that
	 * reads it is still running. This vector only describes how large a pool has
	 * to be, so the first allocation for a pipeline can size one correctly.
	 */
	std::vector<VkDescriptorPoolSize> poolSizes;
	std::array<std::vector<VkDescriptorSlot>, 5> slotsByKind;
	std::vector<std::vector<std::uint32_t>> spirv;
	std::vector<VkShaderStageFlagBits> stages;
	std::vector<Kld::AttributeDescriptor> attributes;
	std::size_t stride = 0;
	std::map<VkRenderPass, std::map<VkPipelineState, VkPipeline>> variants;
	VkPipeline computePipeline = VK_NULL_HANDLE;
	/** @brief True for a compute pipeline (selects the descriptor bind point). */
	bool compute = false;
	std::string name;
};

/** @brief A query pool with its Kaldi query type. */
struct VkQueryPoolObject {
	VkQueryPool pool = VK_NULL_HANDLE;
	Kld::QueryType type = Kld::QueryType::Occlusion;
	std::uint32_t count = 0;
};

/**
 * @brief Attachment formats of one render pass.
 *
 * Vulkan bakes attachment formats into both the render pass and every graphics
 * pipeline variant that draws into it, so the executor keeps a registry keyed by
 * `VkRenderPass`. The default swapchain pass registers itself when the swapchain
 * is (re)created; `CreateFramebuffer` registers the pass it builds.
 */
struct VkPassInfo {
	std::vector<VkFormat> colorFormats;
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;
	bool hasDepth = false;
	bool hasStencil = false;
	std::uint32_t layers = 1;
	/** @brief Attachment sample count; the pipeline variant must match it. */
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
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
	/** @brief One object whose physical destruction waits for its frame slot. */
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

	/**
	 * @brief One frame in flight: command pool, command buffer, fence, descriptors.
	 *
	 * Submission rotates through these slots. A slot may be submitted and left
	 * running; the executor only waits for its fence when the slot is about to be
	 * reused (a real frames-in-flight ring) or when a command needs host-visible
	 * results (`Buffer::map`, readbacks, query results, and the paths that call
	 * `submitAndWait()`).
	 *
	 * Descriptor pools live here rather than in the pipeline objects: every draw
	 * writes its own descriptor sets, and a set must not be reset while the
	 * submission that reads it is still running. Resetting a slot's pools together
	 * with its command pool (on reuse, once the fence signalled) gives exactly that
	 * lifetime, so the sets behave as a per-frame ring instead of accumulating.
	 *
	 * `deferred` is the slot's retirement list: objects destroyed by a command
	 * recorded here are freed when *this* slot's fence signals, not when the whole
	 * device goes idle.
	 */
	struct FrameSlot {
		VkCommandPool pool = VK_NULL_HANDLE;
		VkCommandBuffer commands = VK_NULL_HANDLE;
		VkFence fence = VK_NULL_HANDLE;
		/** @brief True between `vkQueueSubmit` and the fence wait. */
		bool pending = false;
		std::vector<VkDescriptorPool> descriptorPools;
		std::vector<DeferredDestroy> deferred;
	};

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
	Kld::HalStatus submitCurrent(bool wait);
	Kld::HalStatus flushForHostAccess();
	void waitForAllFrames() noexcept;
	void flushDeferred(std::vector<DeferredDestroy>& list) noexcept;
	void releaseFrameSlot(FrameSlot& slot) noexcept;
	std::uint32_t findMemoryType(std::uint32_t typeBits, VkMemoryPropertyFlags properties) const;

	Kld::HalStatus ensureImageLayout(VkTextureObject& object);
	Kld::HalStatus createImageStorage(VkTextureObject& object, VkImageUsageFlags usage);
	Kld::HalStatus uploadImage(VkTextureObject& object, const void* data, std::uint32_t byteSize,
				   std::uint32_t rowPitch, std::uint32_t levelWidth,
				   std::uint32_t levelHeight);
	Kld::HalStatus uploadImageLevel(VkTextureObject& object, std::uint8_t level,
					std::uint16_t x, std::uint16_t y, std::uint16_t layer,
					std::uint16_t width, std::uint16_t height, const void* data,
					std::uint32_t rowPitch, std::uint32_t byteSize);
	Kld::HalStatus generateMips(VkTextureObject& object);
	Kld::HalStatus blitImages(VkTextureObject& source, std::uint16_t sourceX,
				  std::uint16_t sourceY, VkTextureObject& destination,
				  std::uint16_t destinationX, std::uint16_t destinationY,
				  std::uint16_t width, std::uint16_t height);
	Kld::HalStatus createHostBuffer(std::uint32_t size, VkBuffer& buffer, VkDeviceMemory& memory,
					void*& mapped, bool& coherent);
	void destroyBufferObjects(VkBuffer& buffer, VkDeviceMemory& memory, void*& mapped) noexcept;

	Kld::HalStatus createPipelineLayout(VkPipelineObject& object,
					    const Kld::PipelineBindingLayout* layout);
	Kld::HalStatus createGraphicsVariant(VkPipelineObject& object, VkRenderPass renderPass,
					     const VkPipelineState& state, VkPipeline& pipeline);
	Kld::HalStatus createComputeObject(VkPipelineObject& object);
	Kld::HalStatus updateDescriptors(VkPipelineObject& object);
	/**
	 * @brief Allocates one descriptor set from the current frame slot's ring.
	 *
	 * Existing pools are tried first; when a frame needs more sets than the base
	 * pool holds, a new pool is created from @p sizes and appended to the slot.
	 * @p sizes is the pipeline's per-kind requirement, already widened.
	 */
	VkDescriptorSet allocateDescriptorSet(VkDescriptorSetLayout layout,
					      const std::vector<VkDescriptorPoolSize>& sizes);
	Kld::HalStatus prepareDraw(VkPipelineObject*& object, bool lines);
	VkStencilOpState toStencilState(const Kld::StencilFaceState& state) const;
	VkPipelineState currentPipelineState(bool lines) const;
	Kld::HalStatus bindVertexInputs();
	const VkPassInfo* currentPassInfo() const;

	Kld::HalStatus ensureSwapchain();
	/** @brief Creates the default swapchain; @p oldSwapchain is retired as part of it. */
	Kld::HalStatus createSwapchainObjects(VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	/**
	 * @brief Destroys the default swapchain's views/framebuffers/render pass.
	 *
	 * Keeps `swapchain_` for the caller, so a recreate can pass it as
	 * `oldSwapchain` - a native window backs one swapchain at a time.
	 */
	void releaseSwapchainObjects() noexcept;
	void destroySwapchainObjects() noexcept;
	/**
	 * @brief Notes that a present was queued, so retirement has to wait for it.
	 *
	 * `vkQueuePresentKHR` is not covered by the submit fences, and a presentation
	 * engine may keep a swapchain busy for an unbounded time (a minimised window, a
	 * headless X server, a slow compositor). Blocking the queue would hang the whole
	 * backend there, so a present only *arms a marker*: an empty submission on the
	 * presentation queue, whose fence signals once the queue got past the present.
	 */
	void notePresented() noexcept;
	/**
	 * @brief Waits (bounded) until the presentation queue consumed its last present.
	 * @param timeoutNanoseconds Upper bound for the wait; `0` polls once.
	 * @return True when nothing is pending on the presentation queue any more.
	 */
	bool drainPresentQueue(std::uint64_t timeoutNanoseconds) noexcept;
	/**
	 * @brief Destroys retired swapchains whose present completed; never blocks.
	 * @param timeoutNanoseconds Upper bound for the drain attempt.
	 */
	void flushRetiredSwapchains(std::uint64_t timeoutNanoseconds) noexcept;
	/**
	 * @brief Hands a swapchain handle to the deferred-destruction list.
	 *
	 * A handle whose last present is still on the queue cannot be destroyed, so it
	 * is parked here and released by @ref flushRetiredSwapchains once the marker
	 * fence signals - or by `releaseAll()`, after the device is idle.
	 */
	void retireSwapchainHandle(VkSwapchainKHR swapchain) noexcept;
	/**
	 * @brief Destroys the views/framebuffers/render pass of @p object and its handles.
	 *
	 * Leaves `swapchain`, `surface`, `format`, `extent` and `presentMode` alone: those
	 * are the pieces a resize carries over to the replacement swapchain.
	 */
	void releaseSwapchainResources(VkSwapchainObject& object) noexcept;
	/** @brief How long a swapchain destroy/resize may block on the present queue. */
	static constexpr std::uint64_t kPresentDrainTimeout = 100000000ull;
	Kld::HalStatus acquireDefaultImage();
	Kld::HalStatus ensureRenderPass(Kld::HandleId framebufferId);
	void endActiveRenderPass();
	Kld::HalStatus applyClear(const Kld::OpClearColorAttachment& op);

	// Explicit swapchains (HAL 1.1).
	VkSwapchainObject* swapchain(Kld::HandleId id);
	Kld::HalStatus buildSwapchainObjects(VkSwapchainObject& object, VkSurfaceKHR surface,
					     VkExtent2D requestedExtent, VkFormat format,
					     Kld::PresentMode presentMode,
					     std::uint32_t requestedImageCount,
					     VkSwapchainKHR oldSwapchain);
	void destroySwapchainObject(VkSwapchainObject& object) noexcept;
	Kld::HandleId allocateBackendHandle();
	bool ownsActiveFramebuffer(VkSwapchainObject& object) const noexcept;
	Context& context_;
	Kld::HalCapabilities capabilities_;
	std::string message_;
	std::uint64_t executedCommands_ = 0;

	/** @brief Frames the executor keeps in flight. */
	static constexpr std::size_t kFramesInFlight = 3;

	std::array<FrameSlot, kFramesInFlight> frames_ {};
	std::size_t currentSlot_ = 0;

	/** @brief Aliases of the current slot, so handler code needs no indirection. */
	VkCommandPool commandPool_ = VK_NULL_HANDLE;
	VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
	bool recording_ = false;

	/** @brief Queues @p entry for destruction when this slot's fence signals. */
	void defer(DeferredDestroy entry) { frames_[currentSlot_].deferred.push_back(entry); }

	std::unordered_map<Kld::HandleId, VkBufferObject> buffers_;
	std::unordered_map<Kld::HandleId, VkTextureObject> textures_;
	std::unordered_map<Kld::HandleId, VkTextureObject> images_;
	std::unordered_map<Kld::HandleId, VkSamplerObject> samplers_;
	std::unordered_map<Kld::HandleId, VkFramebufferObject> framebuffers_;
	std::unordered_map<Kld::HandleId, VkSwapchainObject> swapchains_;
	/** @brief Next handle to hand out from the backend-reserved range. */
	Kld::HandleId nextBackendHandle_ = Kld::BackendHandleBase;
	std::unordered_map<Kld::HandleId, VkPipelineObject> pipelines_;
	std::unordered_map<Kld::HandleId, VkPipelineObject> computePipelines_;
	std::unordered_map<Kld::HandleId, std::uint64_t> fences_;
	std::unordered_map<Kld::HandleId, VkQueryPoolObject> queryPools_;
	std::unordered_map<Kld::HandleId, std::uint64_t> vertexArrays_;

	struct VertexBinding {
		Kld::HandleId buffer = 0;
		std::uint64_t offset = 0;
		std::uint32_t stride = 0;
		std::uint32_t divisor = 0;
		bool valid = false;
	};
	struct IndexBinding {
		Kld::HandleId buffer = 0;
		std::uint64_t offset = 0;
		VkIndexType type = VK_INDEX_TYPE_UINT16;
		bool valid = false;
	};
	struct BoundTexture {
		Kld::HandleId texture = 0;
		Kld::TextureBindingType type = Kld::TextureBindingType::Texture2D;
		bool valid = false;
	};
	struct BoundRange {
		Kld::HandleId buffer = 0;
		std::uint32_t offset = 0;
		std::uint32_t size = 0;
		Kld::StorageAccess access = Kld::StorageAccess::ReadOnly;
		bool valid = false;
	};
	struct BoundImage {
		Kld::HandleId image = 0;
		std::uint8_t mipLevel = 0;
		std::uint16_t layer = 0;
		Kld::StorageAccess access = Kld::StorageAccess::ReadOnly;
		bool valid = false;
	};
	std::vector<VertexBinding> vertexBindings_;
	IndexBinding indexBinding_;
	BoundTexture boundTextures_[64] {};
	Kld::HandleId boundSamplers_[64] {};
	BoundRange boundUniformBuffers_[32] {};
	BoundRange boundStorageBuffers_[32] {};
	BoundImage boundStorageImages_[32] {};
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
	/**
	 * @brief Fence of the present marker; signals once the present queue advanced.
	 *
	 * Armed lazily, because it costs an empty submission. `presentMarkerStale_`
	 * records the case where another present was queued after the armed marker:
	 * that marker no longer proves anything about the newest present, so the drain
	 * reports "still pending" and a fresh marker is armed on the next attempt.
	 */
	VkFence presentMarkerFence_ = VK_NULL_HANDLE;
	/** @brief True while a present may still be unprocessed by the present queue. */
	bool presentPending_ = false;
	/** @brief True while a marker submission is waiting for its fence. */
	bool presentMarkerArmed_ = false;
	/** @brief True when a present was queued after the armed marker. */
	bool presentMarkerStale_ = false;
	/** @brief Swapchain handles whose last present has not finished yet. */
	std::vector<VkSwapchainKHR> retiredSwapchains_;
	std::uint32_t currentImageIndex_ = 0;
	bool imageAcquired_ = false;
	bool frameActive_ = false;

	VkRenderPass activePass_ = VK_NULL_HANDLE;
	VkFramebuffer activeFramebuffer_ = VK_NULL_HANDLE;

	std::map<VkRenderPass, VkPassInfo> passInfos_;

	VkTextureObject dummyTexture_;
	VkBufferObject dummyBuffer_;
	VkSampler defaultSampler_ = VK_NULL_HANDLE;

	/** @brief Feature gates captured once at construction. */
	bool wideLines_ = false;
	bool depthClampEnabled_ = false;
	bool depthBiasClampEnabled_ = false;
	bool anisotropyEnabled_ = false;
	bool depthRangeUnrestricted_ = false;
	bool occlusionPrecise_ = false;
	float maxAnisotropy_ = 1.0f;

	/** @brief `VK_EXT_debug_utils` label entry points; null when unavailable. */
	PFN_vkCmdBeginDebugUtilsLabelEXT cmdBeginDebugUtilsLabel_ = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT cmdEndDebugUtilsLabel_ = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT cmdInsertDebugUtilsLabel_ = nullptr;
	/** @brief True when all three label entry points were resolved. */
	bool debugLabels_ = false;
	/** @brief Open label regions, so an unbalanced End cannot underflow. */
	std::uint32_t debugLabelDepth_ = 0;
};

}
#endif // KALDIVKCOMMANDEXECUTOR_HPP
