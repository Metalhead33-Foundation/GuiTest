/**
 * @file VkCommandExecutor.cpp
 * @brief Core of the Vulkan command executor: lifetime, submission, frames, state.
 */
#include "VkCommandExecutor.hpp"
#include <Kaldi/KldDispatch.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace Vk {

namespace {

/** @brief Maps a handler call onto one dispatch arm, aborting on failure. */
#define KALDI_VK_HANDLER_PAYLOAD(Name, payload, handler) \
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
#define KALDI_VK_HANDLER(Name, handler) KALDI_VK_HANDLER_PAYLOAD(Name, op##Name, handler)

/** @brief Vulkan shader stage for a Kaldi stage. */
VkShaderStageFlagBits shaderStage(Kld::ShaderStage stage) noexcept
{
	switch (stage) {
		case Kld::ShaderStage::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
		case Kld::ShaderStage::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case Kld::ShaderStage::Geometry: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case Kld::ShaderStage::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case Kld::ShaderStage::TessEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case Kld::ShaderStage::Compute: return VK_SHADER_STAGE_COMPUTE_BIT;
	}
	return VK_SHADER_STAGE_VERTEX_BIT;
}

} // namespace

CommandExecutor::CommandExecutor(Context& context, const Kld::HalCapabilities& capabilities)
	: context_(context)
	, capabilities_(capabilities)
{
	vertexBindings_.resize(std::max<std::uint32_t>(capabilities_.maxVertexBuffers, 16));
	VkCommandPoolCreateInfo poolInfo {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = context_.graphicsQueueFamily();
	if (vkCreateCommandPool(context_.device(), &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
		throw std::runtime_error("VkCommandExecutor: vkCreateCommandPool failed");
	}
	VkCommandBufferAllocateInfo allocateInfo {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = commandPool_;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;
	if (vkAllocateCommandBuffers(context_.device(), &allocateInfo, &commandBuffer_) != VK_SUCCESS) {
		throw std::runtime_error("VkCommandExecutor: vkAllocateCommandBuffers failed");
	}
	VkFenceCreateInfo fenceInfo {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(context_.device(), &fenceInfo, nullptr, &acquireFence_) != VK_SUCCESS) {
		throw std::runtime_error("VkCommandExecutor: vkCreateFence failed");
	}

	VkSamplerCreateInfo samplerInfo {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	if (vkCreateSampler(context_.device(), &samplerInfo, nullptr, &defaultSampler_) != VK_SUCCESS) {
		throw std::runtime_error("VkCommandExecutor: vkCreateSampler failed");
	}

	dummyTexture_.euphemyFormat = Kld::TextureFormat::RGBA8U;
	dummyTexture_.format = VK_FORMAT_R8G8B8A8_UNORM;
	dummyTexture_.width = 1;
	dummyTexture_.height = 1;
	// The placeholder exists so the descriptor path has a valid view once it is
	// ported; uploading texels is unnecessary while draws are still unsupported.
	const Kld::HalStatus status = createImageStorage(dummyTexture_,
							 VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		throw std::runtime_error("VkCommandExecutor: the placeholder texture could not be created");
	}
	if (ensureImageLayout(dummyTexture_).ok()) {
		(void)submitAndWait();
	}
	(void)ensureSwapchain();
}

CommandExecutor::~CommandExecutor()
{
	releaseAll();
}

Kld::HalStatus CommandExecutor::fail(Kld::HalStatusCode code, std::string message)
{
	message_ = std::move(message);
	return Kld::HalStatus { code, message_.c_str() };
}

Kld::HalStatus CommandExecutor::failUnsupported(std::string message)
{
	return fail(Kld::HalStatusCode::UnsupportedBaseline, std::move(message));
}

Kld::HalStatus CommandExecutor::failInvalidHandle(std::string_view kind, Kld::HandleId id)
{
	return fail(Kld::HalStatusCode::InvalidHandle,
		    std::string(kind) + " handle " + std::to_string(id) + " does not exist");
}

bool CommandExecutor::isHandleInUse(Kld::HandleId id) const
{
	return buffers_.count(id) != 0 || textures_.count(id) != 0 || images_.count(id) != 0
		|| samplers_.count(id) != 0 || framebuffers_.count(id) != 0
		|| pipelines_.count(id) != 0 || computePipelines_.count(id) != 0
		|| fences_.count(id) != 0 || queryPools_.count(id) != 0
		|| vertexArrays_.count(id) != 0;
}

VkBufferObject* CommandExecutor::buffer(Kld::HandleId id)
{
	const auto found = buffers_.find(id);
	return found == buffers_.end() ? nullptr : &found->second;
}

VkTextureObject* CommandExecutor::texture(Kld::HandleId id)
{
	const auto found = textures_.find(id);
	return found == textures_.end() ? nullptr : &found->second;
}

VkTextureObject* CommandExecutor::image(Kld::HandleId id)
{
	const auto found = images_.find(id);
	return found == images_.end() ? nullptr : &found->second;
}

VkSamplerObject* CommandExecutor::sampler(Kld::HandleId id)
{
	const auto found = samplers_.find(id);
	return found == samplers_.end() ? nullptr : &found->second;
}

VkFramebufferObject* CommandExecutor::framebuffer(Kld::HandleId id)
{
	const auto found = framebuffers_.find(id);
	return found == framebuffers_.end() ? nullptr : &found->second;
}

VkPipelineObject* CommandExecutor::pipeline(Kld::HandleId id)
{
	const auto found = pipelines_.find(id);
	return found == pipelines_.end() ? nullptr : &found->second;
}

VkPipelineObject* CommandExecutor::computePipeline(Kld::HandleId id)
{
	const auto found = computePipelines_.find(id);
	return found == computePipelines_.end() ? nullptr : &found->second;
}

VkQueryPoolObject* CommandExecutor::queryPool(Kld::HandleId id)
{
	const auto found = queryPools_.find(id);
	return found == queryPools_.end() ? nullptr : &found->second;
}

std::uint32_t CommandExecutor::findMemoryType(std::uint32_t typeBits,
					      VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memory {};
	vkGetPhysicalDeviceMemoryProperties(context_.physicalDevice(), &memory);
	for (std::uint32_t index = 0; index < memory.memoryTypeCount; ++index) {
		if ((typeBits & (1u << index)) != 0
		    && (memory.memoryTypes[index].propertyFlags & properties) == properties) {
			return index;
		}
	}
	return UINT32_MAX;
}

VkStencilOpState CommandExecutor::toStencilState(const Kld::StencilFaceState& state) const
{
	const auto operation = [](Kld::StencilOp value) {
		switch (value) {
			case Kld::StencilOp::Keep: return VK_STENCIL_OP_KEEP;
			case Kld::StencilOp::Zero: return VK_STENCIL_OP_ZERO;
			case Kld::StencilOp::Replace: return VK_STENCIL_OP_REPLACE;
			case Kld::StencilOp::IncrementClamp: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			case Kld::StencilOp::DecrementClamp: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			case Kld::StencilOp::Invert: return VK_STENCIL_OP_INVERT;
			case Kld::StencilOp::IncrementWrap: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			case Kld::StencilOp::DecrementWrap: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		}
		return VK_STENCIL_OP_KEEP;
	};
	const auto comparison = [](Kld::CompareOp value) {
		switch (value) {
			case Kld::CompareOp::Never: return VK_COMPARE_OP_NEVER;
			case Kld::CompareOp::Less: return VK_COMPARE_OP_LESS;
			case Kld::CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
			case Kld::CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
			case Kld::CompareOp::Greater: return VK_COMPARE_OP_GREATER;
			case Kld::CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
			case Kld::CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case Kld::CompareOp::AlwaysPass: return VK_COMPARE_OP_ALWAYS;
		}
		return VK_COMPARE_OP_ALWAYS;
	};
	VkStencilOpState result {};
	result.failOp = operation(state.failOp);
	result.passOp = operation(state.passOp);
	result.depthFailOp = operation(state.depthFailOp);
	result.compareOp = comparison(state.compareOp);
	result.compareMask = state.compareMask;
	result.writeMask = state.writeMask;
	result.reference = state.reference;
	return result;
}

VkPipelineState CommandExecutor::currentPipelineState(bool lines) const
{
	VkPipelineState result = state_;
	result.lines = lines;
	return result;
}

Kld::HalStatus CommandExecutor::ensureRecording()
{
	if (recording_) {
		return Kld::HalOk;
	}
	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkBeginCommandBuffer failed");
	}
	recording_ = true;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::submitAndWait()
{
	endActiveRenderPass();
	if (!recording_) {
		flushDeferred();
		return Kld::HalOk;
	}
	if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkEndCommandBuffer failed");
	}
	VkSubmitInfo submitInfo {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer_;
	if (vkQueueSubmit(context_.graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkQueueSubmit failed");
	}
	if (vkQueueWaitIdle(context_.graphicsQueue()) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkQueueWaitIdle failed");
	}
	recording_ = false;
	(void)vkResetCommandPool(context_.device(), commandPool_, 0);
	flushDeferred();
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::flushForHostAccess()
{
	return submitAndWait();
}

void CommandExecutor::flushDeferred() noexcept
{
	for (const DeferredDestroy& entry : deferred_) {
		if (entry.buffer_ != VK_NULL_HANDLE) {
			vkDestroyBuffer(context_.device(), entry.buffer_, nullptr);
		}
		if (entry.bufferMemory_ != VK_NULL_HANDLE) {
			vkFreeMemory(context_.device(), entry.bufferMemory_, nullptr);
		}
		if (entry.view_ != VK_NULL_HANDLE) {
			vkDestroyImageView(context_.device(), entry.view_, nullptr);
		}
		if (entry.image_ != VK_NULL_HANDLE) {
			vkDestroyImage(context_.device(), entry.image_, nullptr);
		}
		if (entry.imageMemory_ != VK_NULL_HANDLE) {
			vkFreeMemory(context_.device(), entry.imageMemory_, nullptr);
		}
		if (entry.sampler_ != VK_NULL_HANDLE) {
			vkDestroySampler(context_.device(), entry.sampler_, nullptr);
		}
		if (entry.framebuffer_ != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(context_.device(), entry.framebuffer_, nullptr);
		}
		if (entry.renderPass_ != VK_NULL_HANDLE) {
			vkDestroyRenderPass(context_.device(), entry.renderPass_, nullptr);
		}
		if (entry.pipeline_ != VK_NULL_HANDLE) {
			vkDestroyPipeline(context_.device(), entry.pipeline_, nullptr);
		}
		if (entry.layout_ != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(context_.device(), entry.layout_, nullptr);
		}
		if (entry.setLayout_ != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(context_.device(), entry.setLayout_, nullptr);
		}
		if (entry.descriptorPool_ != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(context_.device(), entry.descriptorPool_, nullptr);
		}
		if (entry.queryPool_ != VK_NULL_HANDLE) {
			vkDestroyQueryPool(context_.device(), entry.queryPool_, nullptr);
		}
	}
	deferred_.clear();
}

void CommandExecutor::releaseAll() noexcept
{
	if (vkDeviceWaitIdle == nullptr || context_.device() == VK_NULL_HANDLE) {
		return;
	}
	(void)vkDeviceWaitIdle(context_.device());
	endActiveRenderPass();
	destroySwapchainObjects();
	for (auto& entry : buffers_) {
		if (entry.second.mapped != nullptr) {
			vkUnmapMemory(context_.device(), entry.second.memory);
		}
		vkDestroyBuffer(context_.device(), entry.second.buffer, nullptr);
		vkFreeMemory(context_.device(), entry.second.memory, nullptr);
	}
	const auto releaseTexture = [this](VkTextureObject& object) {
		vkDestroyImageView(context_.device(), object.view, nullptr);
		vkDestroyImage(context_.device(), object.image, nullptr);
		vkFreeMemory(context_.device(), object.memory, nullptr);
	};
	for (auto& entry : textures_) {
		releaseTexture(entry.second);
	}
	for (auto& entry : images_) {
		releaseTexture(entry.second);
	}
	for (auto& entry : samplers_) {
		vkDestroySampler(context_.device(), entry.second.sampler, nullptr);
	}
	for (auto& entry : framebuffers_) {
		vkDestroyFramebuffer(context_.device(), entry.second.framebuffer, nullptr);
		vkDestroyRenderPass(context_.device(), entry.second.renderPass, nullptr);
	}
	const auto releasePipeline = [this](VkPipelineObject& object) {
		for (auto& byPass : object.variants) {
			for (auto& variant : byPass.second) {
				vkDestroyPipeline(context_.device(), variant.second, nullptr);
			}
		}
		if (object.computePipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(context_.device(), object.computePipeline, nullptr);
		}
		if (object.descriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(context_.device(), object.descriptorPool, nullptr);
		}
		if (object.setLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(context_.device(), object.setLayout, nullptr);
		}
		if (object.layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(context_.device(), object.layout, nullptr);
		}
	};
	for (auto& entry : pipelines_) {
		releasePipeline(entry.second);
	}
	for (auto& entry : computePipelines_) {
		releasePipeline(entry.second);
	}
	for (auto& entry : queryPools_) {
		vkDestroyQueryPool(context_.device(), entry.second.pool, nullptr);
	}
	buffers_.clear();
	textures_.clear();
	images_.clear();
	samplers_.clear();
	framebuffers_.clear();
	pipelines_.clear();
	computePipelines_.clear();
	fences_.clear();
	queryPools_.clear();
	vertexArrays_.clear();
	flushDeferred();
	if (dummyTexture_.image != VK_NULL_HANDLE) {
		releaseTexture(dummyTexture_);
		dummyTexture_ = VkTextureObject {};
	}
	if (defaultSampler_ != VK_NULL_HANDLE) {
		vkDestroySampler(context_.device(), defaultSampler_, nullptr);
		defaultSampler_ = VK_NULL_HANDLE;
	}
	if (acquireFence_ != VK_NULL_HANDLE) {
		vkDestroyFence(context_.device(), acquireFence_, nullptr);
		acquireFence_ = VK_NULL_HANDLE;
	}
	if (commandPool_ != VK_NULL_HANDLE) {
		vkDestroyCommandPool(context_.device(), commandPool_, nullptr);
		commandPool_ = VK_NULL_HANDLE;
		commandBuffer_ = VK_NULL_HANDLE;
	}
	recording_ = false;
}

Kld::HalStatus CommandExecutor::process(std::span<const Kld::GfxOp> commands)
{
	if (vkBeginCommandBuffer == nullptr) {
		return fail(Kld::HalStatusCode::InitializationFailed, "Vulkan entry points are not loaded");
	}
	if (commands.empty()) {
		return Kld::HalOk;
	}
	KLD_DISPATCH_RUN(commands, KLD_OPCODE_LIST);
	KALDI_VK_HANDLER(CreateBufferObject, createBuffer)
	KALDI_VK_HANDLER_PAYLOAD(DestroyBufferObject, opDestroy, destroyBuffer)
	KALDI_VK_HANDLER(CreateVertexArrayObject, createVertexArray)
	KALDI_VK_HANDLER(CreateTexture1D, createTexture1D)
	KALDI_VK_HANDLER(CreateTexture2D, createTexture2D)
	KALDI_VK_HANDLER(CreateCompressedTexture2D, createCompressedTexture2D)
	KALDI_VK_HANDLER(CreateTexture2DFromDecodeTarget, createTextureFromDecodeTarget)
	KALDI_VK_HANDLER(CreateTexture3D, createTexture3D)
	KALDI_VK_HANDLER_PAYLOAD(DestroyTexture, opDestroy, destroyTexture)
	KALDI_VK_HANDLER(CreateSampler, createSampler)
	KALDI_VK_HANDLER_PAYLOAD(DestroySampler, opDestroy, destroySampler)
	KALDI_VK_HANDLER(CreateImage2D, createImage2D)
	KALDI_VK_HANDLER(DestroyImage, destroyImage)
	KALDI_VK_HANDLER(CreateFramebuffer, createFramebuffer)
	KALDI_VK_HANDLER(DestroyFramebuffer, destroyFramebuffer)
	KALDI_VK_HANDLER(BindFramebuffer, bindFramebuffer)
	KALDI_VK_HANDLER(BeginFrame, beginFrame)
	KALDI_VK_HANDLER(EndFrame, endFrame)
	KALDI_VK_HANDLER(ClearColorAttachment, clearColorAttachment)
	KALDI_VK_HANDLER(ClearDepthStencilAttachment, clearDepthStencilAttachment)
	KALDI_VK_HANDLER(Present, present)
	KALDI_VK_HANDLER(CreatePipeline, createPipeline)
	KALDI_VK_HANDLER(DestroyPipeline, destroyPipeline)
	KALDI_VK_HANDLER(BindPipeline, bindPipeline)
	KALDI_VK_HANDLER(BindTexture, bindTexture)
	KALDI_VK_HANDLER(BindSampler, bindSampler)
	KALDI_VK_HANDLER(BindUniformBuffer, bindUniformBuffer)
	KALDI_VK_HANDLER(BindVertexBuffer, bindVertexBuffer)
	KALDI_VK_HANDLER(BindIndexBuffer, bindIndexBuffer)
	KALDI_VK_HANDLER(DrawTriangles, drawTriangles)
	KALDI_VK_HANDLER(DrawIndexedTriangles, drawIndexedTriangles)
	KALDI_VK_HANDLER(DrawLines, drawLines)
	KALDI_VK_HANDLER(DrawIndexedLines, drawIndexedLines)
	KALDI_VK_HANDLER(ResizeBufferObject, resizeBuffer)
	KALDI_VK_HANDLER(UpdateBufferObject, updateBuffer)
	KALDI_VK_HANDLER(MapBufferObject, mapBuffer)
	KALDI_VK_HANDLER(ResizeTexture2D, resizeTexture2D)
	KALDI_VK_HANDLER(UpdateTexture2D, updateTexture2D)
	KALDI_VK_HANDLER(UpdateCompressedTexture2D, updateCompressedTexture2D)
	KALDI_VK_HANDLER(BlitTexture2D, blitTexture2D)
	KALDI_VK_HANDLER(GenerateTextureMipmaps, generateTextureMipmaps)
	KALDI_VK_HANDLER(ResizeImage2D, resizeImage2D)
	KALDI_VK_HANDLER(UpdateImage2D, updateImage2D)
	KALDI_VK_HANDLER(BlitImage2D, blitImage2D)
	KALDI_VK_HANDLER(SetViewport, setViewport)
	KALDI_VK_HANDLER(SetScissor, setScissor)
	KALDI_VK_HANDLER(SetBlendState, setBlendState)
	KALDI_VK_HANDLER(SetDepthStencilState, setDepthStencilState)
	KALDI_VK_HANDLER(SetRasterizerState, setRasterizerState)
	KALDI_VK_HANDLER(TransitionResource, transitionResource)
	KALDI_VK_HANDLER(BufferBarrier, bufferBarrier)
	KALDI_VK_HANDLER(ImageBarrier, imageBarrier)
	KALDI_VK_HANDLER(CreateFence, createFence)
	KALDI_VK_HANDLER(DestroyFence, destroyFence)
	KALDI_VK_HANDLER(SignalFence, signalFence)
	KALDI_VK_HANDLER(WaitFence, waitFence)
	KALDI_VK_HANDLER(CreateComputePipeline, createComputePipeline)
	KALDI_VK_HANDLER(DestroyComputePipeline, destroyComputePipeline)
	KALDI_VK_HANDLER(BindComputePipeline, bindComputePipeline)
	KALDI_VK_HANDLER(DispatchCompute, dispatchCompute)
	KALDI_VK_HANDLER(DispatchComputeIndirect, dispatchComputeIndirect)
	KALDI_VK_HANDLER(DrawIndirect, drawIndirect)
	KALDI_VK_HANDLER(DrawIndexedIndirect, drawIndexedIndirect)
	KALDI_VK_HANDLER(CreateQueryPool, createQueryPool)
	KALDI_VK_HANDLER(DestroyQueryPool, destroyQueryPool)
	KALDI_VK_HANDLER(ResetQueryPool, resetQueryPool)
	KALDI_VK_HANDLER(BeginQuery, beginQuery)
	KALDI_VK_HANDLER(EndQuery, endQuery)
	KALDI_VK_HANDLER(WriteTimestamp, writeTimestamp)
	KALDI_VK_HANDLER(DebugLabelBegin, debugLabelBegin)
	KALDI_VK_HANDLER(DebugLabelEnd, debugLabelEnd)
	KALDI_VK_HANDLER(DebugMarker, debugMarker)
	KALDI_VK_HANDLER(CopyBuffer, copyBuffer)
	KALDI_VK_HANDLER(CopyBufferToImage2D, copyBufferToImage2D)
	KALDI_VK_HANDLER(CopyImage2DToBuffer, copyImage2DToBuffer)
	KALDI_VK_HANDLER(CopyImage2D, copyImage2D)
	KALDI_VK_HANDLER(ReadBuffer, readBuffer)
	KALDI_VK_HANDLER(ReadImage2D, readImage2D)
	KALDI_VK_HANDLER(ReadQueryResults, readQueryResults)
	KALDI_VK_HANDLER(CreateTextureCube, createTextureCube)
	KALDI_VK_HANDLER(UpdateTextureCubeFace, updateTextureCubeFace)
	KALDI_VK_HANDLER(CreateTexture2DArray, createTexture2DArray)
	KALDI_VK_HANDLER(UpdateTexture2DArray, updateTexture2DArray)
	KALDI_VK_HANDLER(CreateMultisampledImage2D, createMultisampledImage2D)
	KALDI_VK_HANDLER(ResolveImage2D, resolveImage2D)
	KALDI_VK_HANDLER(BindStorageBuffer, bindStorageBuffer)
	KALDI_VK_HANDLER(BindStorageImage, bindStorageImage)
	KALDI_VK_HANDLER(DrawTrianglesInstanced, drawTrianglesInstanced)
	KALDI_VK_HANDLER(DrawIndexedTrianglesInstanced, drawIndexedTrianglesInstanced)
	KALDI_VK_HANDLER(DrawLinesInstanced, drawLinesInstanced)
	KALDI_VK_HANDLER(DrawIndexedLinesInstanced, drawIndexedLinesInstanced)
	KALDI_VK_HANDLER(CreateSwapchain, createSwapchain)
	KALDI_VK_HANDLER(DestroySwapchain, destroySwapchain)
	KALDI_VK_HANDLER(ResizeSwapchain, resizeSwapchain)
	KALDI_VK_HANDLER(AcquireSwapchainImage, acquireSwapchainImage)
	KALDI_VK_HANDLER(PresentSwapchain, presentSwapchain)
	KALDI_VK_HANDLER(BeginRenderPass, beginRenderPass)
	KALDI_VK_HANDLER(EndRenderPass, endRenderPass)
	KLD_DISPATCH_UNKNOWN()
	{
		kldStatus = fail(Kld::HalStatusCode::InvalidCommand,
				 std::string("unknown opcode ")
					 + std::to_string(static_cast<unsigned>(kldIp->opcode)));
		KLD_DISPATCH_ABORT();
	}
	KLD_DISPATCH_END();
	if (kldStatus.ok() && !frameActive_) {
		const Kld::HalStatus flushed = submitAndWait();
		if (!flushed.ok()) {
			return flushed;
		}
	}
	return kldStatus;
}

Kld::HalStatus CommandExecutor::setViewport(const Kld::OpSetViewport& op)
{
	viewport_.x = op.x;
	// Kaldi's viewport follows the GL convention (origin at the bottom-left);
	// Vulkan's is top-left, so the rectangle is flipped to keep the same image.
	std::uint32_t height = swapchainExtent_.height;
	if (boundFramebuffer_ != Kld::DefaultFramebuffer) {
		if (VkFramebufferObject* target = framebuffer(boundFramebuffer_)) {
			height = target->height;
		}
	}
	viewport_.y = static_cast<float>(height) - (op.y + op.height);
	viewport_.width = op.width;
	viewport_.height = op.height;
	viewport_.minDepth = op.minDepth;
	viewport_.maxDepth = op.maxDepth;
	viewportSet_ = true;
	if (recording_) {
		vkCmdSetViewport(commandBuffer_, 0, 1, &viewport_);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setScissor(const Kld::OpSetScissor& op)
{
	std::uint32_t height = swapchainExtent_.height;
	if (boundFramebuffer_ != Kld::DefaultFramebuffer) {
		if (VkFramebufferObject* target = framebuffer(boundFramebuffer_)) {
			height = target->height;
		}
	}
	scissor_.offset.x = std::max(op.x, 0);
	scissor_.offset.y = std::max(static_cast<std::int32_t>(height) - op.y
					     - static_cast<std::int32_t>(op.height),
				     0);
	scissor_.extent.width = op.width;
	scissor_.extent.height = op.height;
	scissorSet_ = true;
	if (recording_) {
		vkCmdSetScissor(commandBuffer_, 0, 1, &scissor_);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setBlendState(const Kld::OpSetBlendState& op)
{
	const auto factor = [](Kld::BlendFactor value) {
		switch (value) {
			case Kld::BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
			case Kld::BlendFactor::One: return VK_BLEND_FACTOR_ONE;
			case Kld::BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
			case Kld::BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			case Kld::BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
			case Kld::BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			case Kld::BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
			case Kld::BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case Kld::BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
			case Kld::BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		}
		return VK_BLEND_FACTOR_ONE;
	};
	const auto operation = [](Kld::BlendOp value) {
		switch (value) {
			case Kld::BlendOp::Add: return VK_BLEND_OP_ADD;
			case Kld::BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
			case Kld::BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
			case Kld::BlendOp::Min: return VK_BLEND_OP_MIN;
			case Kld::BlendOp::Max: return VK_BLEND_OP_MAX;
		}
		return VK_BLEND_OP_ADD;
	};
	state_.blendEnabled = op.enabled;
	state_.srcColor = factor(op.srcColor);
	state_.dstColor = factor(op.dstColor);
	state_.srcAlpha = factor(op.srcAlpha);
	state_.dstAlpha = factor(op.dstAlpha);
	state_.colorOp = operation(op.colorOp);
	state_.alphaOp = operation(op.alphaOp);
	VkColorComponentFlags mask = 0;
	if ((op.colorWriteMask & Kld::COLOR_WRITE_RED) != 0) mask |= VK_COLOR_COMPONENT_R_BIT;
	if ((op.colorWriteMask & Kld::COLOR_WRITE_GREEN) != 0) mask |= VK_COLOR_COMPONENT_G_BIT;
	if ((op.colorWriteMask & Kld::COLOR_WRITE_BLUE) != 0) mask |= VK_COLOR_COMPONENT_B_BIT;
	if ((op.colorWriteMask & Kld::COLOR_WRITE_ALPHA) != 0) mask |= VK_COLOR_COMPONENT_A_BIT;
	state_.colorMask = mask;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setDepthStencilState(const Kld::OpSetDepthStencilState& op)
{
	const auto comparison = [](Kld::CompareOp value) {
		switch (value) {
			case Kld::CompareOp::Never: return VK_COMPARE_OP_NEVER;
			case Kld::CompareOp::Less: return VK_COMPARE_OP_LESS;
			case Kld::CompareOp::Equal: return VK_COMPARE_OP_EQUAL;
			case Kld::CompareOp::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
			case Kld::CompareOp::Greater: return VK_COMPARE_OP_GREATER;
			case Kld::CompareOp::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
			case Kld::CompareOp::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case Kld::CompareOp::AlwaysPass: return VK_COMPARE_OP_ALWAYS;
		}
		return VK_COMPARE_OP_ALWAYS;
	};
	state_.depthTest = op.depthTestEnabled;
	state_.depthWrite = op.depthWriteEnabled;
	state_.depthCompare = comparison(op.depthCompare);
	state_.stencilTest = op.stencilTestEnabled;
	state_.front = toStencilState(op.front);
	state_.back = toStencilState(op.back);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::setRasterizerState(const Kld::OpSetRasterizerState& op)
{
	const auto cull = [](Kld::CullMode value) {
		switch (value) {
			case Kld::CullMode::NoCulling: return VK_CULL_MODE_NONE;
			case Kld::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
			case Kld::CullMode::Back: return VK_CULL_MODE_BACK_BIT;
			case Kld::CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
		}
		return VK_CULL_MODE_NONE;
	};
	state_.cullMode = cull(op.cullMode);
	state_.frontFace = op.frontFace == Kld::FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE
								    : VK_FRONT_FACE_COUNTER_CLOCKWISE;
	state_.depthClamp = op.depthClampEnabled;
	state_.depthBias = op.depthBiasEnabled;
	state_.biasConstant = op.depthBiasConstantFactor;
	state_.biasClamp = op.depthBiasClamp;
	state_.biasSlope = op.depthBiasSlopeFactor;
	// Vulkan 1.0 has no dynamic polygon mode; the pipeline variants always fill.
	if (op.polygonMode != Kld::PolygonMode::Fill) {
		return failUnsupported("Vulkan 1.0 (without VK_EXT_extended_dynamic_state) cannot switch polygon mode");
	}
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
	// Every internal image lives in VK_IMAGE_LAYOUT_GENERAL, so a coarse
	// transition only needs the execution/memory dependency barrier.
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = toVkAccessFlags(op.oldState);
	barrier.dstAccessMask = toVkAccessFlags(op.newState);
	vkCmdPipelineBarrier(commandBuffer_, toVkPipelineStageFlags(op.sourceStages),
			     toVkPipelineStageFlags(op.destinationStages), 0, 1, &barrier, 0, nullptr,
			     0, nullptr);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bufferBarrier(const Kld::OpBufferBarrier& op)
{
	VkBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkBufferMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = toVkAccessFlags(op.sourceAccess);
	barrier.dstAccessMask = toVkAccessFlags(op.destinationAccess);
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = object->buffer;
	barrier.offset = op.offset;
	barrier.size = op.size != 0 ? op.size : VK_WHOLE_SIZE;
	vkCmdPipelineBarrier(commandBuffer_, toVkPipelineStageFlags(op.sourceStages),
			     toVkPipelineStageFlags(op.destinationStages), 0, 0, nullptr, 1, &barrier,
			     0, nullptr);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::imageBarrier(const Kld::OpImageBarrier& op)
{
	VkTextureObject* object = image(op.image);
	if (object == nullptr) {
		return failInvalidHandle("image", op.image);
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = toVkAccessFlags(op.sourceAccess);
	barrier.dstAccessMask = toVkAccessFlags(op.destinationAccess);
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = object->image;
	barrier.subresourceRange.aspectMask = object->aspect;
	barrier.subresourceRange.baseMipLevel = op.baseMipLevel;
	barrier.subresourceRange.levelCount = op.mipLevelCount != 0 ? op.mipLevelCount
								    : VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = op.baseLayer;
	barrier.subresourceRange.layerCount = op.layerCount != 0 ? op.layerCount
								 : VK_REMAINING_ARRAY_LAYERS;
	vkCmdPipelineBarrier(commandBuffer_, toVkPipelineStageFlags(op.sourceStages),
			     toVkPipelineStageFlags(op.destinationStages), 0, 0, nullptr, 0, nullptr,
			     1, &barrier);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createFence(const Kld::OpCreateFence& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "fence handle is zero or already in use");
	}
	fences_.emplace(op.id, op.initialValue);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyFence(const Kld::OpDestroyFence& op)
{
	const auto found = fences_.find(op.id);
	if (found == fences_.end()) {
		return failInvalidHandle("fence", op.id);
	}
	fences_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::signalFence(const Kld::OpSignalFence& op)
{
	const auto found = fences_.find(op.id);
	if (found == fences_.end()) {
		return failInvalidHandle("fence", op.id);
	}
	// Execution is synchronous, so a signalled value is already complete.
	found->second = op.value;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::waitFence(const Kld::OpWaitFence& op)
{
	const auto found = fences_.find(op.id);
	if (found == fences_.end()) {
		return failInvalidHandle("fence", op.id);
	}
	if (op.value > found->second) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the fence value was never signalled");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugLabelBegin(const Kld::OpDebugLabelBegin&)
{
	// Vk::Context enables no debug-marker extension yet; capabilities() reports
	// supportsDebugLabels == false, so labels are consumed and dropped.
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugLabelEnd(const Kld::OpDebugLabelEnd&)
{
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugMarker(const Kld::OpDebugMarker&)
{
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createSwapchain(const Kld::OpCreateSwapchain&)
{
	return failUnsupported("explicit swapchains are not implemented by the Vulkan backend");
}

Kld::HalStatus CommandExecutor::destroySwapchain(const Kld::OpDestroySwapchain& op)
{
	if (op.id == Kld::DefaultSwapchain) {
		return Kld::HalOk;
	}
	return failUnsupported("explicit swapchains are not implemented by the Vulkan backend");
}

Kld::HalStatus CommandExecutor::resizeSwapchain(const Kld::OpResizeSwapchain& op)
{
	if (op.id != Kld::DefaultSwapchain) {
		return failUnsupported("explicit swapchains are not implemented by the Vulkan backend");
	}
	Kld::HalStatus status = submitAndWait();
	if (!status.ok()) {
		return status;
	}
	destroySwapchainObjects();
	swapchainExtent_ = VkExtent2D { op.width, op.height };
	return createSwapchainObjects();
}

Kld::HalStatus CommandExecutor::acquireSwapchainImage(const Kld::OpAcquireSwapchainImage& op)
{
	if (op.swapchain != Kld::DefaultSwapchain) {
		return failUnsupported("explicit swapchains are not implemented by the Vulkan backend");
	}
	Kld::HalStatus status = ensureSwapchain();
	if (!status.ok()) {
		return status;
	}
	if (!imageAcquired_) {
		status = acquireDefaultImage();
		if (!status.ok()) {
			return status;
		}
	}
	if (op.callback != nullptr) {
		const Kld::SwapchainAcquireInfo info { op.swapchain, Kld::SwapchainStatus::Ready,
						       currentImageIndex_, 0, 0, Kld::DefaultFramebuffer };
		op.callback(&info, op.userData);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::presentSwapchain(const Kld::OpPresentSwapchain& op)
{
	if (op.swapchain != Kld::DefaultSwapchain) {
		return failUnsupported("explicit swapchains are not implemented by the Vulkan backend");
	}
	return present(Kld::OpPresent { Kld::DefaultFramebuffer });
}

}
