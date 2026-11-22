#ifndef KALDIGLCOMMANDEXECUTOR_HPP
#define KALDIGLCOMMANDEXECUTOR_HPP
/**
 * @file GlCommandExecutor.hpp
 * @brief OpenGL / OpenGL ES implementation of `Kld::Device::processCommandQueue()`.
 *
 * This is the server side of the Kaldi VM: it owns the GL object namespace behind
 * the client's `Kld::HandleId`s, tracks the state a command stream expects, and
 * executes commands in span order. Both GL-family backends compile the same
 * implementation; `KALDIGL_IS_GLES` selects the few places where the dialects
 * differ (no 1D textures, no polygon mode, no base-instance draws, ...).
 *
 * @section kaldigl_executor_lifetime Resource lifetime
 *
 * Destroy commands delete the GL name immediately. That *is* the deferred
 * retirement the HAL contract asks for: OpenGL keeps the underlying object alive
 * until the last command that can reference it has executed, so a client may
 * reuse the handle as soon as the destroy command was processed. No CPU-side
 * destruction queue is needed; only explicit APIs (Vulkan, D3D12) require one.
 *
 * Handles are kind-checked: one numeric namespace is shared by every resource
 * (`Kld::HandleId`), so looking up a texture handle as a buffer reports
 * `HalStatusCode::InvalidHandle` instead of dereferencing the wrong object.
 *
 * @section kaldigl_executor_state State model
 *
 * State commands (`SetViewport`, `SetBlendState`, ...) write GL state directly.
 * Vertex-buffer and index-buffer bindings are *tracked* and re-applied whenever a
 * pipeline binds its VAO, because both are VAO state in OpenGL and the Kaldi
 * vocabulary allows binding them before or after `BindPipeline`.
 *
 * The executor assumes a current context on the calling thread for the whole
 * `process()` call; it calls `Context::makeCurrent()` once at entry so a caller
 * may submit from any thread as long as submissions do not overlap.
 *
 * @section kaldigl_executor_unsupported Deliberate gaps
 *
 * The GL backend reports `HalStatusCode::UnsupportedBaseline` (never a silent
 * no-op) for: a non-null native surface or a mailbox present mode on an explicit
 * swapchain, swapchain image bindings (GL has one backend-owned image), 1D
 * textures on GLES, `PolygonMode::Line`/`Point` on GLES, depth clamp on GLES,
 * stencil-only image readback, base-instance draws on GLES, and timestamp queries
 * on GLES without `EXT_disjoint_timer_query`. Everything else in `Kld::Opcode` is
 * implemented.
 *
 * Explicit swapchains are a *validated view* of the default drawable rather than
 * a second presentation target: `AcquireSwapchainImage` reports
 * `DefaultFramebuffer` with image index `0`, and `PresentSwapchain` swaps the
 * window's buffers. That keeps one caller protocol across GL and Vulkan.
 */
#include "GlBackend.hpp"
#if KALDIGL_IS_GLES
#include <glad/gles2.h>
#else
#include <glad/gl.h>
#endif
#include "GlDeviceConfig.hpp"
#include "GlUtil.hpp"
#include <Kaldi/KaldiOperation.hpp>
#include <Kaldi/KldHal.hpp>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace KALDIGL_NS {

/** @brief One GL buffer name plus the metadata later commands need. */
struct GlBufferObject {
	/** @brief GL buffer name. */
	GLuint name = 0;
	/** @brief Size in bytes. */
	std::uint32_t size = 0;
	/** @brief `Kld::BufferUsage` bitmask the object was created with. */
	std::uint32_t policy = 0;
	/** @brief Preferred binding target derived from @ref policy. */
	GLenum target = GL_ARRAY_BUFFER;
};

/**
 * @brief One sampled texture, 2D image or attachment texture.
 *
 * Textures and images share the structure because both are `GL_TEXTURE_*`
 * objects in OpenGL; they live in separate maps so the kind check stays exact.
 */
struct GlTextureObject {
	/** @brief GL texture name. */
	GLuint name = 0;
	/** @brief GL target (`GL_TEXTURE_2D`, `GL_TEXTURE_2D_ARRAY`, ...). */
	GLenum target = GL_TEXTURE_2D;
	/** @brief Sized internal format used by `glBindImageTexture`. */
	GLenum internalFormat = 0;
	/** @brief Euphemy format the object was created with. */
	Kld::TextureFormat format {};
	/** @brief Mip-0 width in texels. */
	std::uint16_t width = 0;
	/** @brief Mip-0 height in texels. */
	std::uint16_t height = 0;
	/** @brief Mip-0 depth for 3D textures. */
	std::uint16_t depth = 1;
	/** @brief Array layer or cube face count. */
	std::uint16_t layers = 1;
	/** @brief Number of allocated mip levels. */
	std::uint8_t levels = 1;
	/** @brief True for S3TC/DXT block-compressed storage. */
	bool compressed = false;
	/** @brief True for multisample textures. */
	bool multisampled = false;
};

/** @brief One GL sampler object. */
struct GlSamplerObject {
	/** @brief GL sampler name. */
	GLuint name = 0;
};

/** @brief One framebuffer attachment binding, kept for clear/pass operations. */
struct GlFramebufferAttachment {
	/** @brief Attached image handle. */
	Kld::HandleId image = 0;
	/** @brief Attachment role. */
	Kld::FramebufferAttachmentRole role = Kld::FramebufferAttachmentRole::Color;
	/** @brief Color attachment index (ignored for depth/stencil). */
	std::uint32_t index = 0;
};

/** @brief One framebuffer object and its attachment list. */
struct GlFramebufferObject {
	/** @brief GL framebuffer name. */
	GLuint name = 0;
	/** @brief Declared width in pixels. */
	std::uint16_t width = 0;
	/** @brief Declared height in pixels. */
	std::uint16_t height = 0;
	/** @brief Attachments in declaration order. */
	std::vector<GlFramebufferAttachment> attachments;
};

/**
 * @brief One explicit swapchain object over the context's default drawable.
 *
 * OpenGL has no swapchain: there is exactly one presentable image per context,
 * the window's default framebuffer (name `0`). An explicit swapchain is therefore
 * a *validated view* of that drawable - it records the requested extent, pacing
 * and depth/stencil requirement, and `AcquireSwapchainImage` reports framebuffer
 * `DefaultFramebuffer` with image index `0`. This keeps one portable caller
 * protocol (`create`, `acquire`, draw into the reported framebuffer, `present`)
 * across GL and Vulkan without pretending GL owns multiple images.
 *
 * The drawable's real size belongs to the window system, so @ref width /
 * @ref height are advisory: `ResizeSwapchain` records them and the backend keeps
 * rendering into whatever the platform gave the context.
 */
struct GlSwapchainObject {
	/** @brief Drawable framebuffer name; always 0 (the window's back buffer). */
	GLuint framebuffer = 0;
	/** @brief Advisory width in pixels. */
	std::uint16_t width = 0;
	/** @brief Advisory height in pixels. */
	std::uint16_t height = 0;
	/** @brief Recorded pacing policy. */
	Kld::PresentMode presentMode = Kld::PresentMode::Fifo;
	/** @brief True when the drawable has a depth and/or stencil buffer. */
	bool depthStencil = false;
	/** @brief True between a successful acquire and the matching present. */
	bool acquired = false;
};

/**
 * @brief One linked graphics or compute program.
 *
 * Graphics pipelines also own a VAO. Attribute descriptors are *copied* because
 * the `Kld::VertexDescriptor` the command pointed at may be transient; semantic
 * names are dropped because GL addresses attributes by location.
 */
struct GlPipelineObject {
	/** @brief Linked GL program name. */
	GLuint program = 0;
	/** @brief VAO built from the pipeline's vertex input, or 0 for compute. */
	GLuint vao = 0;
	/** @brief Copied attribute descriptors (locations are vector indices). */
	std::vector<Kld::AttributeDescriptor> attributes;
	/** @brief Byte size of one vertex. */
	std::size_t stride = 0;
	/** @brief True when the pipeline declared a vertex input. */
	bool hasVertexInput = false;
	/** @brief Optional debug name from the create command. */
	std::string name;
};

/** @brief A `GLsync` plus the timeline value it represents. */
struct GlFenceObject {
	/** @brief Fence object, or `nullptr` before the first signal. */
	GLsync sync = nullptr;
	/** @brief Greatest value signalled so far. */
	std::uint64_t value = 0;
	/** @brief True once a sync object exists. */
	bool signalled = false;
};

/** @brief A homogeneous GL query pool. */
struct GlQueryPoolObject {
	/** @brief Query names, one per slot. */
	std::vector<GLuint> queries;
	/** @brief Pool query type. */
	Kld::QueryType type = Kld::QueryType::Occlusion;
};

/** @brief A standalone VAO created by `OpCreateVertexArrayObject`. */
struct GlVertexArrayObject {
	/** @brief GL vertex array name. */
	GLuint name = 0;
};

/**
 * @brief Executes Kaldi command queues on a current OpenGL / OpenGL ES context.
 */
class CommandExecutor {
public:
	/**
	 * @brief Creates an executor and its internal scratch objects.
	 * @param context Backend context; must outlive the executor and be current on
	 *        the thread that constructs it (GL names are created eagerly).
	 * @param capabilities Snapshot used to validate limits and feature gates.
	 * @param config Executor policy from @ref DeviceConfig.
	 * @throws std::runtime_error When a required scratch object cannot be created.
	 */
	CommandExecutor(Context& context, const Kld::HalCapabilities& capabilities,
			const ExecutorConfig& config = ExecutorConfig());

	/**
	 * @brief Deletes every GL object still owned by the executor.
	 *
	 * The owner should call releaseAll() while the context is current; the
	 * destructor repeats the call as a best-effort fallback.
	 */
	~CommandExecutor();

	CommandExecutor(const CommandExecutor&) = delete;
	CommandExecutor& operator=(const CommandExecutor&) = delete;
	CommandExecutor(CommandExecutor&&) = delete;
	CommandExecutor& operator=(CommandExecutor&&) = delete;

	/**
	 * @brief Executes @p commands in order.
	 * @return `HalOk`, or the first failure; processing stops at that command.
	 */
	Kld::HalStatus process(std::span<const Kld::GfxOp> commands);

	/** @brief Deletes every GL object owned by the executor. Requires a current context. */
	void releaseAll() noexcept;

	/** @brief The policy this executor was created with. */
	const ExecutorConfig& config() const noexcept { return config_; }

	/** @brief Number of commands executed since construction (diagnostics). */
	std::uint64_t executedCommands() const noexcept { return executedCommands_; }

private:
	// ── Dispatch handlers ────────────────────────────────────────────────
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
	// ── Helpers ──────────────────────────────────────────────────────────
	Kld::HalStatus fail(Kld::HalStatusCode code, std::string message);
	Kld::HalStatus failInvalidHandle(std::string_view kind, Kld::HandleId id);
	Kld::HalStatus failUnsupported(std::string message);
	bool isHandleInUse(Kld::HandleId id) const;

	GlBufferObject* buffer(Kld::HandleId id);
	GlTextureObject* texture(Kld::HandleId id);
	GlTextureObject* image(Kld::HandleId id);
	GlSamplerObject* sampler(Kld::HandleId id);
	GlFramebufferObject* framebuffer(Kld::HandleId id);
	GlPipelineObject* pipeline(Kld::HandleId id);
	GlPipelineObject* computePipeline(Kld::HandleId id);
	GlFenceObject* fence(Kld::HandleId id);
	GlQueryPoolObject* queryPool(Kld::HandleId id);

	GLuint textureName(Kld::HandleId id) const;
	GLuint imageName(Kld::HandleId id) const;

	Kld::HalStatus linkProgram(const Kld::ShaderBinaryDescriptor* binaries, std::uint32_t count,
				   GLuint& program);
	Kld::HalStatus makeVertexArray(GlPipelineObject& object, const Kld::VertexDescriptor* descriptor);
	Kld::HalStatus buildGraphicsPipeline(GlPipelineObject& object, const Kld::OpCreatePipeline& op);
	Kld::HalStatus buildComputePipeline(GlPipelineObject& object,
					    const Kld::OpCreateComputePipeline& op);

	void applyDefaultTextureParameters(GlTextureObject& object) const;
	void reapplyVertexState();
	Kld::HalStatus attachImage(const GlTextureObject& object, GLenum framebufferTarget,
				   GLenum attachment, std::uint8_t mipLevel, std::uint16_t layer);
	Kld::HalStatus blit2D(const GlTextureObject& source, std::uint16_t sourceX,
			      std::uint16_t sourceY, const GlTextureObject& destination,
			      std::uint16_t destinationX, std::uint16_t destinationY,
			      std::uint16_t width, std::uint16_t height);
	Kld::HalStatus readTexturePixels(const GlTextureObject& object, std::uint8_t mipLevel,
					 std::uint16_t layer, std::uint16_t x, std::uint16_t y,
					 std::uint16_t width, std::uint16_t height, void* destination,
					 std::uint32_t destinationSize, std::uint32_t* rowPitch);
	Kld::HalStatus drawArrays(GLenum mode, std::uint32_t first, std::uint32_t count,
				  std::uint32_t instances, std::uint32_t baseInstance);
	Kld::HalStatus drawElements(GLenum mode, std::uint32_t indexCount, std::uint32_t byteOffset,
				    std::uint32_t baseVertex, std::uint32_t instances,
				    std::uint32_t baseInstance);
	GLbitfield barrierBits(std::uint32_t destinationStages,
			       std::uint32_t destinationAccess) const;

	// ── State ────────────────────────────────────────────────────────────
	struct VertexBindingState {
		Kld::HandleId buffer = 0;
		std::uint32_t offset = 0;
		std::uint32_t stride = 0;
		std::uint32_t divisor = 0;
		bool valid = false;
	};
	struct IndexBindingState {
		Kld::HandleId buffer = 0;
		std::uint32_t offset = 0;
		Kld::IndexType type = Kld::IndexType::Uint16;
		bool valid = false;
	};

	Context& context_;
	Kld::HalCapabilities capabilities_;
	ExecutorConfig config_;
	std::string message_;

	std::unordered_map<Kld::HandleId, GlBufferObject> buffers_;
	std::unordered_map<Kld::HandleId, GlTextureObject> textures_;
	std::unordered_map<Kld::HandleId, GlTextureObject> images_;
	std::unordered_map<Kld::HandleId, GlSamplerObject> samplers_;
	std::unordered_map<Kld::HandleId, GlFramebufferObject> framebuffers_;
	std::unordered_map<Kld::HandleId, GlSwapchainObject> swapchains_;
	std::unordered_map<Kld::HandleId, GlPipelineObject> pipelines_;
	std::unordered_map<Kld::HandleId, GlPipelineObject> computePipelines_;
	std::unordered_map<Kld::HandleId, GlFenceObject> fences_;
	std::unordered_map<Kld::HandleId, GlQueryPoolObject> queryPools_;
	std::unordered_map<Kld::HandleId, GlVertexArrayObject> vertexArrays_;

	GLuint scratchReadFramebuffer_ = 0;
	GLuint scratchDrawFramebuffer_ = 0;
	GLuint defaultVertexArray_ = 0;

	GLuint currentProgram_ = 0;
	GLuint currentVertexArray_ = 0;
	Kld::HandleId boundPipeline_ = 0;
	bool computePipelineActive_ = false;
	Kld::HandleId boundFramebuffer_ = Kld::DefaultFramebuffer;
	std::vector<VertexBindingState> vertexBindings_;
	IndexBindingState indexBinding_;

	bool frameActive_ = false;
	std::uint64_t frameIndex_ = 0;
	std::uint64_t executedCommands_ = 0;
	unsigned debugLabelDepth_ = 0;
	bool debugLabelsEnabled_ = false;
	bool anisotropyEnabled_ = false;
	bool timestampQueriesEnabled_ = false;
	GLint maxTextureUnits_ = 0;
	GLint maxImageUnits_ = 0;

};

}

#endif // KALDIGLCOMMANDEXECUTOR_HPP
