/**
 * @file GlCommandExecutor.cpp
 * @brief Core of the OpenGL command executor: lifetime, dispatch, state, frames.
 *
 * Resource handlers live in `GlExecutorResources.cpp`, pipeline/binding/draw
 * handlers in `GlExecutorPipelines.cpp`, and transfer/query handlers in
 * `GlExecutorTransfer.cpp`; this unit owns the dispatch itself because the
 * computed-goto structure requires every handler label in one function.
 */
#include "GlCommandExecutor.hpp"
#include "GlCapabilities.hpp"
#include "GlExecutorMapping.hpp"
#include <Kaldi/KldDispatch.hpp>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace KALDIGL_NS {

namespace {

/** @brief Maps a handler call onto one dispatch arm, aborting on failure. */
#define KALDI_GL_HANDLER_PAYLOAD(Name, payload, handler) \
	KLD_DISPATCH_ON(Name) \
	{ \
		kldStatus = handler(kldIp->opt.payload); \
		if (kldStatus.ok()) { \
			++executedCommands_; \
			KLD_DISPATCH_NEXT(); \
		} else { \
			KLD_DISPATCH_ABORT(); \
		} \
	}

/** @brief Maps a handler whose payload member is named after the opcode. */
#define KALDI_GL_HANDLER(Name, handler) KALDI_GL_HANDLER_PAYLOAD(Name, op##Name, handler)

} // namespace

CommandExecutor::CommandExecutor(Context& context, const Kld::HalCapabilities& capabilities,
				 const ExecutorConfig& config)
	: context_(context)
	, capabilities_(capabilities)
	, config_(config)
{
	debugLabelsEnabled_ = config_.debugLabels && glPushDebugGroup != nullptr && glPopDebugGroup != nullptr;
	anisotropyEnabled_ = config_.maxAnisotropy > 0.0f && hasGlExtension("GL_EXT_texture_filter_anisotropic");
#if KALDIGL_IS_GLES
	timestampQueriesEnabled_ = glQueryCounterEXT != nullptr && glGetQueryObjectui64vEXT != nullptr;
#else
	timestampQueriesEnabled_ = glQueryCounter != nullptr && glGetQueryObjectui64v != nullptr;
#endif
	maxTextureUnits_ = static_cast<GLint>(capabilities_.maxSampledTextureBindings);
	maxImageUnits_ = static_cast<GLint>(capabilities_.maxStorageImageBindings);
	vertexBindings_.resize(std::max<std::size_t>(capabilities_.maxVertexBuffers, 16));

	glGenFramebuffers(1, &scratchReadFramebuffer_);
	glGenFramebuffers(1, &scratchDrawFramebuffer_);
	glGenVertexArrays(1, &defaultVertexArray_);
	if (scratchReadFramebuffer_ == 0 || scratchDrawFramebuffer_ == 0 || defaultVertexArray_ == 0) {
		throw std::runtime_error("GlCommandExecutor: internal scratch GL objects could not be created");
	}
	glBindVertexArray(defaultVertexArray_);
}

CommandExecutor::~CommandExecutor()
{
	releaseAll();
}

void CommandExecutor::releaseAll() noexcept
{
	if (glDeleteTextures == nullptr) {
		return;
	}
	for (auto& entry : buffers_) {
		glDeleteBuffers(1, &entry.second.name);
	}
	for (auto& entry : textures_) {
		glDeleteTextures(1, &entry.second.name);
	}
	for (auto& entry : images_) {
		glDeleteTextures(1, &entry.second.name);
	}
	for (auto& entry : samplers_) {
		glDeleteSamplers(1, &entry.second.name);
	}
	for (auto& entry : framebuffers_) {
		glDeleteFramebuffers(1, &entry.second.name);
	}
	const auto releasePipeline = [](GlPipelineObject& object) {
		if (object.vao != 0) {
			glDeleteVertexArrays(1, &object.vao);
			object.vao = 0;
		}
		if (object.program != 0) {
			glDeleteProgram(object.program);
			object.program = 0;
		}
	};
	for (auto& entry : pipelines_) {
		releasePipeline(entry.second);
	}
	for (auto& entry : computePipelines_) {
		releasePipeline(entry.second);
	}
	for (auto& entry : vertexArrays_) {
		glDeleteVertexArrays(1, &entry.second.name);
	}
	for (auto& entry : fences_) {
		if (entry.second.sync != nullptr) {
			glDeleteSync(entry.second.sync);
		}
	}
	for (auto& entry : queryPools_) {
		if (!entry.second.queries.empty()) {
			glDeleteQueries(static_cast<GLsizei>(entry.second.queries.size()),
					entry.second.queries.data());
		}
	}
	buffers_.clear();
	textures_.clear();
	images_.clear();
	samplers_.clear();
	framebuffers_.clear();
	pipelines_.clear();
	computePipelines_.clear();
	vertexArrays_.clear();
	fences_.clear();
	queryPools_.clear();
	vertexBindings_.assign(vertexBindings_.size(), VertexBindingState{});
	indexBinding_ = IndexBindingState{};
	currentProgram_ = 0;
	currentVertexArray_ = 0;
	boundPipeline_ = 0;
	boundFramebuffer_ = Kld::DefaultFramebuffer;
	debugLabelDepth_ = 0;
	frameActive_ = false;
	for (GLuint* framebuffer : { &scratchReadFramebuffer_, &scratchDrawFramebuffer_ }) {
		if (*framebuffer != 0) {
			glDeleteFramebuffers(1, framebuffer);
			*framebuffer = 0;
		}
	}
	if (defaultVertexArray_ != 0) {
		glDeleteVertexArrays(1, &defaultVertexArray_);
		defaultVertexArray_ = 0;
	}
}

Kld::HalStatus CommandExecutor::fail(Kld::HalStatusCode code, std::string message)
{
	message_ = std::move(message);
	return Kld::HalStatus { code, message_.c_str() };
}

Kld::HalStatus CommandExecutor::failInvalidHandle(std::string_view kind, Kld::HandleId id)
{
	std::string message(kind);
	message += " handle ";
	message += std::to_string(id);
	message += " does not exist";
	return fail(Kld::HalStatusCode::InvalidHandle, std::move(message));
}

Kld::HalStatus CommandExecutor::failUnsupported(std::string message)
{
	return fail(Kld::HalStatusCode::UnsupportedBaseline, std::move(message));
}

bool CommandExecutor::isHandleInUse(Kld::HandleId id) const
{
	return buffers_.find(id) != buffers_.end() || textures_.find(id) != textures_.end()
		|| images_.find(id) != images_.end() || samplers_.find(id) != samplers_.end()
		|| framebuffers_.find(id) != framebuffers_.end() || pipelines_.find(id) != pipelines_.end()
		|| computePipelines_.find(id) != computePipelines_.end() || fences_.find(id) != fences_.end()
		|| queryPools_.find(id) != queryPools_.end() || vertexArrays_.find(id) != vertexArrays_.end();
}

GlBufferObject* CommandExecutor::buffer(Kld::HandleId id)
{
	const auto found = buffers_.find(id);
	return found == buffers_.end() ? nullptr : &found->second;
}

GlTextureObject* CommandExecutor::texture(Kld::HandleId id)
{
	const auto found = textures_.find(id);
	return found == textures_.end() ? nullptr : &found->second;
}

GlTextureObject* CommandExecutor::image(Kld::HandleId id)
{
	const auto found = images_.find(id);
	return found == images_.end() ? nullptr : &found->second;
}

GlSamplerObject* CommandExecutor::sampler(Kld::HandleId id)
{
	const auto found = samplers_.find(id);
	return found == samplers_.end() ? nullptr : &found->second;
}

GlFramebufferObject* CommandExecutor::framebuffer(Kld::HandleId id)
{
	const auto found = framebuffers_.find(id);
	return found == framebuffers_.end() ? nullptr : &found->second;
}

GlPipelineObject* CommandExecutor::pipeline(Kld::HandleId id)
{
	const auto found = pipelines_.find(id);
	return found == pipelines_.end() ? nullptr : &found->second;
}

GlPipelineObject* CommandExecutor::computePipeline(Kld::HandleId id)
{
	const auto found = computePipelines_.find(id);
	return found == computePipelines_.end() ? nullptr : &found->second;
}

GlFenceObject* CommandExecutor::fence(Kld::HandleId id)
{
	const auto found = fences_.find(id);
	return found == fences_.end() ? nullptr : &found->second;
}

GlQueryPoolObject* CommandExecutor::queryPool(Kld::HandleId id)
{
	const auto found = queryPools_.find(id);
	return found == queryPools_.end() ? nullptr : &found->second;
}

GLuint CommandExecutor::textureName(Kld::HandleId id) const
{
	const auto found = textures_.find(id);
	return found == textures_.end() ? 0 : found->second.name;
}

GLuint CommandExecutor::imageName(Kld::HandleId id) const
{
	const auto found = images_.find(id);
	return found == images_.end() ? 0 : found->second.name;
}

GLbitfield CommandExecutor::barrierBits(std::uint32_t destinationStages,
					std::uint32_t destinationAccess) const
{
	return detail::memoryBarrierBits(destinationStages, destinationAccess);
}

Kld::HalStatus CommandExecutor::process(std::span<const Kld::GfxOp> commands)
{
	if (glGetIntegerv == nullptr) {
		return fail(Kld::HalStatusCode::InitializationFailed, "GL entry points are not loaded");
	}
	if (commands.empty()) {
		return Kld::HalOk;
	}
	context_.makeCurrent();

	KLD_DISPATCH_RUN(commands, KLD_OPCODE_LIST);
	KALDI_GL_HANDLER(CreateBufferObject, createBuffer)
	KALDI_GL_HANDLER_PAYLOAD(DestroyBufferObject, opDestroy, destroyBuffer)
	KALDI_GL_HANDLER(CreateVertexArrayObject, createVertexArray)
	KALDI_GL_HANDLER(CreateTexture1D, createTexture1D)
	KALDI_GL_HANDLER(CreateTexture2D, createTexture2D)
	KALDI_GL_HANDLER(CreateCompressedTexture2D, createCompressedTexture2D)
	KALDI_GL_HANDLER(CreateTexture2DFromDecodeTarget, createTextureFromDecodeTarget)
	KALDI_GL_HANDLER(CreateTexture3D, createTexture3D)
	KALDI_GL_HANDLER_PAYLOAD(DestroyTexture, opDestroy, destroyTexture)
	KALDI_GL_HANDLER(CreateSampler, createSampler)
	KALDI_GL_HANDLER_PAYLOAD(DestroySampler, opDestroy, destroySampler)
	KALDI_GL_HANDLER(CreateImage2D, createImage2D)
	KALDI_GL_HANDLER(DestroyImage, destroyImage)
	KALDI_GL_HANDLER(CreateFramebuffer, createFramebuffer)
	KALDI_GL_HANDLER(DestroyFramebuffer, destroyFramebuffer)
	KALDI_GL_HANDLER(BindFramebuffer, bindFramebuffer)
	KALDI_GL_HANDLER(BeginFrame, beginFrame)
	KALDI_GL_HANDLER(EndFrame, endFrame)
	KALDI_GL_HANDLER(ClearColorAttachment, clearColorAttachment)
	KALDI_GL_HANDLER(ClearDepthStencilAttachment, clearDepthStencilAttachment)
	KALDI_GL_HANDLER(Present, present)
	KALDI_GL_HANDLER(CreatePipeline, createPipeline)
	KALDI_GL_HANDLER(DestroyPipeline, destroyPipeline)
	KALDI_GL_HANDLER(BindPipeline, bindPipeline)
	KALDI_GL_HANDLER(BindTexture, bindTexture)
	KALDI_GL_HANDLER(BindSampler, bindSampler)
	KALDI_GL_HANDLER(BindUniformBuffer, bindUniformBuffer)
	KALDI_GL_HANDLER(BindVertexBuffer, bindVertexBuffer)
	KALDI_GL_HANDLER(BindIndexBuffer, bindIndexBuffer)
	KALDI_GL_HANDLER(DrawTriangles, drawTriangles)
	KALDI_GL_HANDLER(DrawIndexedTriangles, drawIndexedTriangles)
	KALDI_GL_HANDLER(DrawLines, drawLines)
	KALDI_GL_HANDLER(DrawIndexedLines, drawIndexedLines)
	KALDI_GL_HANDLER(ResizeBufferObject, resizeBuffer)
	KALDI_GL_HANDLER(UpdateBufferObject, updateBuffer)
	KALDI_GL_HANDLER(MapBufferObject, mapBuffer)
	KALDI_GL_HANDLER(ResizeTexture2D, resizeTexture2D)
	KALDI_GL_HANDLER(UpdateTexture2D, updateTexture2D)
	KALDI_GL_HANDLER(UpdateCompressedTexture2D, updateCompressedTexture2D)
	KALDI_GL_HANDLER(BlitTexture2D, blitTexture2D)
	KALDI_GL_HANDLER(GenerateTextureMipmaps, generateTextureMipmaps)
	KALDI_GL_HANDLER(ResizeImage2D, resizeImage2D)
	KALDI_GL_HANDLER(UpdateImage2D, updateImage2D)
	KALDI_GL_HANDLER(BlitImage2D, blitImage2D)
	KALDI_GL_HANDLER(SetViewport, setViewport)
	KALDI_GL_HANDLER(SetScissor, setScissor)
	KALDI_GL_HANDLER(SetBlendState, setBlendState)
	KALDI_GL_HANDLER(SetDepthStencilState, setDepthStencilState)
	KALDI_GL_HANDLER(SetRasterizerState, setRasterizerState)
	KALDI_GL_HANDLER(TransitionResource, transitionResource)
	KALDI_GL_HANDLER(BufferBarrier, bufferBarrier)
	KALDI_GL_HANDLER(ImageBarrier, imageBarrier)
	KALDI_GL_HANDLER(CreateFence, createFence)
	KALDI_GL_HANDLER(DestroyFence, destroyFence)
	KALDI_GL_HANDLER(SignalFence, signalFence)
	KALDI_GL_HANDLER(WaitFence, waitFence)
	KALDI_GL_HANDLER(CreateComputePipeline, createComputePipeline)
	KALDI_GL_HANDLER(DestroyComputePipeline, destroyComputePipeline)
	KALDI_GL_HANDLER(BindComputePipeline, bindComputePipeline)
	KALDI_GL_HANDLER(DispatchCompute, dispatchCompute)
	KALDI_GL_HANDLER(DispatchComputeIndirect, dispatchComputeIndirect)
	KALDI_GL_HANDLER(DrawIndirect, drawIndirect)
	KALDI_GL_HANDLER(DrawIndexedIndirect, drawIndexedIndirect)
	KALDI_GL_HANDLER(CreateQueryPool, createQueryPool)
	KALDI_GL_HANDLER(DestroyQueryPool, destroyQueryPool)
	KALDI_GL_HANDLER(ResetQueryPool, resetQueryPool)
	KALDI_GL_HANDLER(BeginQuery, beginQuery)
	KALDI_GL_HANDLER(EndQuery, endQuery)
	KALDI_GL_HANDLER(WriteTimestamp, writeTimestamp)
	KALDI_GL_HANDLER(DebugLabelBegin, debugLabelBegin)
	KALDI_GL_HANDLER(DebugLabelEnd, debugLabelEnd)
	KALDI_GL_HANDLER(DebugMarker, debugMarker)
	KALDI_GL_HANDLER(CopyBuffer, copyBuffer)
	KALDI_GL_HANDLER(CopyBufferToImage2D, copyBufferToImage2D)
	KALDI_GL_HANDLER(CopyImage2DToBuffer, copyImage2DToBuffer)
	KALDI_GL_HANDLER(CopyImage2D, copyImage2D)
	KALDI_GL_HANDLER(ReadBuffer, readBuffer)
	KALDI_GL_HANDLER(ReadImage2D, readImage2D)
	KALDI_GL_HANDLER(ReadQueryResults, readQueryResults)
	KALDI_GL_HANDLER(CreateTextureCube, createTextureCube)
	KALDI_GL_HANDLER(UpdateTextureCubeFace, updateTextureCubeFace)
	KALDI_GL_HANDLER(CreateTexture2DArray, createTexture2DArray)
	KALDI_GL_HANDLER(UpdateTexture2DArray, updateTexture2DArray)
	KALDI_GL_HANDLER(CreateMultisampledImage2D, createMultisampledImage2D)
	KALDI_GL_HANDLER(ResolveImage2D, resolveImage2D)
	KALDI_GL_HANDLER(BindStorageBuffer, bindStorageBuffer)
	KALDI_GL_HANDLER(BindStorageImage, bindStorageImage)
	KALDI_GL_HANDLER(DrawTrianglesInstanced, drawTrianglesInstanced)
	KALDI_GL_HANDLER(DrawIndexedTrianglesInstanced, drawIndexedTrianglesInstanced)
	KALDI_GL_HANDLER(DrawLinesInstanced, drawLinesInstanced)
	KALDI_GL_HANDLER(DrawIndexedLinesInstanced, drawIndexedLinesInstanced)
	KALDI_GL_HANDLER(CreateSwapchain, createSwapchain)
	KALDI_GL_HANDLER(DestroySwapchain, destroySwapchain)
	KALDI_GL_HANDLER(ResizeSwapchain, resizeSwapchain)
	KALDI_GL_HANDLER(AcquireSwapchainImage, acquireSwapchainImage)
	KALDI_GL_HANDLER(PresentSwapchain, presentSwapchain)
	KALDI_GL_HANDLER(BeginRenderPass, beginRenderPass)
	KALDI_GL_HANDLER(EndRenderPass, endRenderPass)
	KLD_DISPATCH_UNKNOWN()
	{
		kldStatus = fail(Kld::HalStatusCode::InvalidCommand,
				 std::string("unknown opcode ")
					 + std::to_string(static_cast<unsigned>(kldIp->opcode)));
		KLD_DISPATCH_ABORT();
	}
	KLD_DISPATCH_END();
	return kldStatus;
}

Kld::HalStatus CommandExecutor::setViewport(const Kld::OpSetViewport& op)
{
	glViewport(static_cast<GLint>(op.x), static_cast<GLint>(op.y),
		   static_cast<GLsizei>(op.width), static_cast<GLsizei>(op.height));
	glDepthRangef(op.minDepth, op.maxDepth);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setScissor(const Kld::OpSetScissor& op)
{
	// Kaldi has no separate "enable scissor test" command, so using a scissor
	// rectangle implies the test is on.
	glEnable(GL_SCISSOR_TEST);
	glScissor(op.x, op.y, static_cast<GLsizei>(op.width), static_cast<GLsizei>(op.height));
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setBlendState(const Kld::OpSetBlendState& op)
{
	if (op.enabled) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}
	glBlendFuncSeparate(detail::blendFactor(op.srcColor), detail::blendFactor(op.dstColor),
			    detail::blendFactor(op.srcAlpha), detail::blendFactor(op.dstAlpha));
	glBlendEquationSeparate(detail::blendOp(op.colorOp), detail::blendOp(op.alphaOp));
	glColorMask((op.colorWriteMask & Kld::COLOR_WRITE_RED) != 0,
		    (op.colorWriteMask & Kld::COLOR_WRITE_GREEN) != 0,
		    (op.colorWriteMask & Kld::COLOR_WRITE_BLUE) != 0,
		    (op.colorWriteMask & Kld::COLOR_WRITE_ALPHA) != 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setDepthStencilState(const Kld::OpSetDepthStencilState& op)
{
	if (op.depthTestEnabled) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
	glDepthMask(op.depthWriteEnabled ? GL_TRUE : GL_FALSE);
	glDepthFunc(detail::compareOp(op.depthCompare));
	if (op.stencilTestEnabled) {
		glEnable(GL_STENCIL_TEST);
	} else {
		glDisable(GL_STENCIL_TEST);
	}
	const auto applyFace = [](GLenum face, const Kld::StencilFaceState& state) {
		glStencilFuncSeparate(face, detail::compareOp(state.compareOp),
				      static_cast<GLint>(state.reference),
				      static_cast<GLuint>(state.compareMask));
		glStencilMaskSeparate(face, static_cast<GLuint>(state.writeMask));
		glStencilOpSeparate(face, detail::stencilOp(state.failOp),
				    detail::stencilOp(state.depthFailOp),
				    detail::stencilOp(state.passOp));
	};
	applyFace(GL_FRONT, op.front);
	applyFace(GL_BACK, op.back);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setRasterizerState(const Kld::OpSetRasterizerState& op)
{
	const GLenum face = detail::cullFace(op.cullMode);
	if (face == 0) {
		glDisable(GL_CULL_FACE);
	} else {
		glEnable(GL_CULL_FACE);
		glCullFace(face);
	}
	glFrontFace(detail::frontFace(op.frontFace));
#if !KALDIGL_IS_GLES
	switch (op.polygonMode) {
		case Kld::PolygonMode::Fill: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
		case Kld::PolygonMode::Line: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
		case Kld::PolygonMode::Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
	}
	if (op.depthClampEnabled) {
		glEnable(GL_DEPTH_CLAMP);
	} else {
		glDisable(GL_DEPTH_CLAMP);
	}
#else
	if (op.polygonMode != Kld::PolygonMode::Fill) {
		return failUnsupported("OpenGL ES has no glPolygonMode; only fill rasterisation is available");
	}
	if (op.depthClampEnabled) {
		return failUnsupported("OpenGL ES has no depth clamp");
	}
#endif
	if (op.depthBiasEnabled) {
		glEnable(GL_POLYGON_OFFSET_FILL);
		// GL has one constant and one slope factor; Kaldi's depthBiasClamp has no
		// OpenGL equivalent and is ignored (documented).
		glPolygonOffset(op.depthBiasSlopeFactor, op.depthBiasConstantFactor);
	} else {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindFramebuffer(const Kld::OpBindFramebuffer& op)
{
	if (op.id == Kld::DefaultFramebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		boundFramebuffer_ = op.id;
		return Kld::HalOk;
	}
	GlFramebufferObject* target = framebuffer(op.id);
	if (target == nullptr) {
		return failInvalidHandle("framebuffer", op.id);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, target->name);
	boundFramebuffer_ = op.id;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::beginFrame(const Kld::OpBeginFrame& op)
{
	Kld::HalStatus status = bindFramebuffer(Kld::OpBindFramebuffer { op.framebuffer });
	if (!status.ok()) {
		return status;
	}
	frameActive_ = true;
	++frameIndex_;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::endFrame(const Kld::OpEndFrame& op)
{
	if (op.framebuffer != Kld::DefaultFramebuffer && boundFramebuffer_ != op.framebuffer) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "EndFrame does not match the framebuffer BeginFrame selected");
	}
	frameActive_ = false;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::clearColorAttachment(const Kld::OpClearColorAttachment& op)
{
	if (op.colorAttachmentIndex >= capabilities_.maxColorAttachments) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "color attachment index exceeds the probed limit");
	}
	Kld::HalStatus status = bindFramebuffer(Kld::OpBindFramebuffer { op.framebuffer });
	if (!status.ok()) {
		return status;
	}
	glClearBufferfv(GL_COLOR, static_cast<GLint>(op.colorAttachmentIndex), op.color);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::clearDepthStencilAttachment(
	const Kld::OpClearDepthStencilAttachment& op)
{
	Kld::HalStatus status = bindFramebuffer(Kld::OpBindFramebuffer { op.framebuffer });
	if (!status.ok()) {
		return status;
	}
	const GLint stencil = static_cast<GLint>(op.stencil);
	if (op.clearDepth && op.clearStencil) {
		glClearBufferfi(GL_DEPTH_STENCIL, 0, op.depth, stencil);
	} else if (op.clearDepth) {
		glClearBufferfv(GL_DEPTH, 0, &op.depth);
	} else if (op.clearStencil) {
		glClearBufferiv(GL_STENCIL, 0, &stencil);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::present(const Kld::OpPresent& op)
{
	if (op.framebuffer != Kld::DefaultFramebuffer) {
		return failUnsupported("Present only supports the backend-owned default framebuffer");
	}
	try {
		context_.swapBuffers();
	} catch (const std::exception& error) {
		frameActive_ = false;
		return fail(Kld::HalStatusCode::SwapchainLost, error.what());
	}
	frameActive_ = false;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::transitionResource(const Kld::OpTransitionResource& op)
{
	switch (op.kind) {
		case Kld::ResourceKind::Buffer:
			if (buffer(op.id) == nullptr) {
				return failInvalidHandle("buffer", op.id);
			}
			break;
		case Kld::ResourceKind::Texture:
			if (texture(op.id) == nullptr) {
				return failInvalidHandle("texture", op.id);
			}
			break;
		case Kld::ResourceKind::Image:
			if (image(op.id) == nullptr) {
				return failInvalidHandle("image", op.id);
			}
			break;
		case Kld::ResourceKind::Framebuffer:
			if (op.id != Kld::DefaultFramebuffer && framebuffer(op.id) == nullptr) {
				return failInvalidHandle("framebuffer", op.id);
			}
			break;
	}
	// GL has no resource states. The destination stage still needs visibility of
	// earlier writes, which is exactly what glMemoryBarrier provides.
	glMemoryBarrier(barrierBits(op.destinationStages, Kld::ACCESS_NONE));
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bufferBarrier(const Kld::OpBufferBarrier& op)
{
	if (buffer(op.buffer) == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	glMemoryBarrier(barrierBits(op.destinationStages, op.destinationAccess));
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::imageBarrier(const Kld::OpImageBarrier& op)
{
	if (image(op.image) == nullptr) {
		return failInvalidHandle("image", op.image);
	}
	glMemoryBarrier(barrierBits(op.destinationStages, op.destinationAccess));
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugLabelBegin(const Kld::OpDebugLabelBegin& op)
{
	if (!debugLabelsEnabled_) {
		return Kld::HalOk;
	}
	// GL debug groups carry a name, not a colour; op.color is intentionally unused.
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, op.name != nullptr ? op.name : "");
	++debugLabelDepth_;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugLabelEnd(const Kld::OpDebugLabelEnd&)
{
	if (debugLabelsEnabled_ && debugLabelDepth_ > 0) {
		glPopDebugGroup();
		--debugLabelDepth_;
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugMarker(const Kld::OpDebugMarker& op)
{
	if (!debugLabelsEnabled_) {
		return Kld::HalOk;
	}
	glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER, 0,
			     GL_DEBUG_SEVERITY_NOTIFICATION, -1,
			     op.name != nullptr ? op.name : "");
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createSwapchain(const Kld::OpCreateSwapchain&)
{
	return failUnsupported("explicit swapchains are not implemented by the GL backend; "
			       "use BeginFrame/EndFrame/Present with DefaultFramebuffer");
}

Kld::HalStatus CommandExecutor::destroySwapchain(const Kld::OpDestroySwapchain& op)
{
	if (op.id == Kld::DefaultSwapchain) {
		return Kld::HalOk; // The default target is backend-owned; nothing to destroy.
	}
	return failUnsupported("explicit swapchains are not implemented by the GL backend");
}

Kld::HalStatus CommandExecutor::resizeSwapchain(const Kld::OpResizeSwapchain&)
{
	return failUnsupported("explicit swapchains are not implemented by the GL backend");
}

Kld::HalStatus CommandExecutor::acquireSwapchainImage(const Kld::OpAcquireSwapchainImage& op)
{
	if (op.swapchain != Kld::DefaultSwapchain) {
		return failUnsupported("explicit swapchains are not implemented by the GL backend");
	}
	if (op.callback != nullptr) {
		const Kld::SwapchainAcquireInfo info { op.swapchain, Kld::SwapchainStatus::Ready, 0, 0, 0,
						       Kld::DefaultFramebuffer };
		op.callback(&info, op.userData);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::presentSwapchain(const Kld::OpPresentSwapchain& op)
{
	if (op.swapchain != Kld::DefaultSwapchain) {
		return failUnsupported("explicit swapchains are not implemented by the GL backend");
	}
	return present(Kld::OpPresent { Kld::DefaultFramebuffer });
}

Kld::HalStatus CommandExecutor::beginRenderPass(const Kld::OpBeginRenderPass& op)
{
	const Kld::RenderPassDescriptor* descriptor = op.descriptor;
	if (descriptor == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "BeginRenderPass requires a descriptor");
	}
	const Kld::HalStatus bound = bindFramebuffer(Kld::OpBindFramebuffer { descriptor->framebuffer });
	if (!bound.ok()) {
		return bound;
	}
	for (std::uint32_t index = 0; index < descriptor->colorAttachmentCount; ++index) {
		const Kld::RenderPassColorAttachment& attachment = descriptor->colorAttachments[index];
		if (attachment.loadOp == Kld::AttachmentLoadOp::Clear) {
			glClearBufferfv(GL_COLOR, static_cast<GLint>(attachment.colorAttachmentIndex),
					attachment.clearColor);
		}
	}
	const Kld::RenderPassDepthStencilAttachment& depthStencil = descriptor->depthStencilAttachment;
	if (depthStencil.enabled) {
		const bool clearDepth = depthStencil.depthLoadOp == Kld::AttachmentLoadOp::Clear;
		const bool clearStencil = depthStencil.stencilLoadOp == Kld::AttachmentLoadOp::Clear;
		const GLint stencil = static_cast<GLint>(depthStencil.clearStencil);
		if (clearDepth && clearStencil) {
			glClearBufferfi(GL_DEPTH_STENCIL, 0, depthStencil.clearDepth, stencil);
		} else if (clearDepth) {
			glClearBufferfv(GL_DEPTH, 0, &depthStencil.clearDepth);
		} else if (clearStencil) {
			glClearBufferiv(GL_STENCIL, 0, &stencil);
		}
	}
	// AttachmentStoreOp::DontCare is not acted on: GL cannot discard a window
	// system attachment, and for textures the write-back is deferred by the
	// driver anyway. Correctness is unaffected.
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::endRenderPass(const Kld::OpEndRenderPass&)
{
	return Kld::HalOk;
}

}
