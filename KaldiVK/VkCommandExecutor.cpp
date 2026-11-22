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

} // namespace

CommandExecutor::CommandExecutor(Context& context, const Kld::HalCapabilities& capabilities)
	: context_(context)
	, capabilities_(capabilities)
	, memory_(context)
{
	vertexBindings_.resize(std::max<std::uint32_t>(capabilities_.maxVertexBuffers, 16));
	// Staging slices have to satisfy both the copy alignment and the atom size that
	// a non-coherent flush requires.
	const VkPhysicalDeviceLimits& limits = context_.properties().limits;
	stagingAlignment_ = std::max<std::uint32_t>(4, limits.optimalBufferCopyOffsetAlignment);
	stagingAlignment_ = std::max<std::uint32_t>(stagingAlignment_,
						    limits.nonCoherentAtomSize);
	for (FrameSlot& slot : frames_) {
		VkCommandPoolCreateInfo poolInfo {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = context_.graphicsQueueFamily();
		if (vkCreateCommandPool(context_.device(), &poolInfo, nullptr, &slot.pool)
		    != VK_SUCCESS) {
			throw std::runtime_error("VkCommandExecutor: vkCreateCommandPool failed");
		}
		VkCommandBufferAllocateInfo allocateInfo {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = slot.pool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		if (vkAllocateCommandBuffers(context_.device(), &allocateInfo, &slot.commands)
		    != VK_SUCCESS) {
			throw std::runtime_error("VkCommandExecutor: vkAllocateCommandBuffers failed");
		}
		VkFenceCreateInfo fenceInfo {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		if (vkCreateFence(context_.device(), &fenceInfo, nullptr, &slot.fence)
		    != VK_SUCCESS) {
			throw std::runtime_error("VkCommandExecutor: vkCreateFence failed");
		}
	}
	commandPool_ = frames_[0].pool;
	commandBuffer_ = frames_[0].commands;
	VkFenceCreateInfo fenceInfo {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	if (vkCreateFence(context_.device(), &fenceInfo, nullptr, &acquireFence_) != VK_SUCCESS) {
		throw std::runtime_error("VkCommandExecutor: vkCreateFence failed");
	}
	// Marker fence for the presentation queue; see notePresented().
	if (vkCreateFence(context_.device(), &fenceInfo, nullptr, &presentMarkerFence_)
	    != VK_SUCCESS) {
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
	// The placeholder exists so the descriptor path always has a valid view; its
	// single texel is uploaded so sampling it is defined behaviour.
	const Kld::HalStatus status = createImageStorage(dummyTexture_,
							 VK_IMAGE_USAGE_SAMPLED_BIT);
	if (!status.ok()) {
		throw std::runtime_error("VkCommandExecutor: the placeholder texture could not be created");
	}
	const std::uint32_t white = 0xFFFFFFFFu;
	if (ensureImageLayout(dummyTexture_).ok()) {
		(void)uploadImage(dummyTexture_, &white, sizeof(white), sizeof(white), 1, 1);
		(void)submitAndWait();
	}
	// Unbound uniform/storage slots point at this buffer so a descriptor set is
	// always complete; the draw-time validation still rejects genuinely missing
	// bindings with a precise message.
	const Kld::HalStatus bufferStatus = createMappedBuffer(65536, dummyBuffer_.buffer,
							      dummyBuffer_.allocation,
							      dummyBuffer_.mapped);
	if (!bufferStatus.ok()) {
		throw std::runtime_error("VkCommandExecutor: the placeholder buffer could not be created");
	}
	dummyBuffer_.size = 65536;
	dummyBuffer_.hostVisible = true;
	const VkPhysicalDeviceFeatures& features = context_.enabledFeatures();
	wideLines_ = features.wideLines == VK_TRUE;
	depthClampEnabled_ = features.depthClamp == VK_TRUE;
	depthBiasClampEnabled_ = features.depthBiasClamp == VK_TRUE;
	anisotropyEnabled_ = features.samplerAnisotropy == VK_TRUE;
	depthRangeUnrestricted_ = features.depthBounds == VK_TRUE;
	occlusionPrecise_ = features.occlusionQueryPrecise == VK_TRUE;
	maxAnisotropy_ = context_.properties().limits.maxSamplerAnisotropy;
	if (maxAnisotropy_ < 1.0f) {
		maxAnisotropy_ = 1.0f;
	}
	// Debug labels need the instance extension plus three device entry points;
	// without them the handlers stay no-ops and capabilities say so.
	if (context_.debugUtilsEnabled()) {
		cmdBeginDebugUtilsLabel_ = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
			vkGetDeviceProcAddr(context_.device(), "vkCmdBeginDebugUtilsLabelEXT"));
		cmdEndDebugUtilsLabel_ = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
			vkGetDeviceProcAddr(context_.device(), "vkCmdEndDebugUtilsLabelEXT"));
		cmdInsertDebugUtilsLabel_ = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(
			vkGetDeviceProcAddr(context_.device(), "vkCmdInsertDebugUtilsLabelEXT"));
		debugLabels_ = cmdBeginDebugUtilsLabel_ != nullptr && cmdEndDebugUtilsLabel_ != nullptr
			       && cmdInsertDebugUtilsLabel_ != nullptr;
	}
	// The default swapchain is created lazily: a Vulkan surface can back only one
	// swapchain, so an application that uses an explicit swapchain on the
	// context's own surface must not have the default one claimed at startup.
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
		|| vertexArrays_.count(id) != 0 || swapchains_.count(id) != 0;
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
	FrameSlot& slot = frames_[currentSlot_];
	if (slot.pending) {
		if (vkWaitForFences(context_.device(), 1, &slot.fence, VK_TRUE, UINT64_MAX)
		    != VK_SUCCESS) {
			return fail(Kld::HalStatusCode::BackendError, "vkWaitForFences failed");
		}
		slot.pending = false;
		flushDeferred(slot.deferred);
	}
	// The slot is idle: its command buffer, its descriptor sets and its staging
	// slices are no longer referenced by queued work, so the ring can start over.
	if (vkResetCommandPool(context_.device(), slot.pool, 0) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkResetCommandPool failed");
	}
	slot.staging.cursor = 0;
	for (VkDescriptorPool pool : slot.descriptorPools) {
		(void)vkResetDescriptorPool(context_.device(), pool, 0);
	}
	commandPool_ = slot.pool;
	commandBuffer_ = slot.commands;
	VkCommandBufferBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkBeginCommandBuffer failed");
	}
	recording_ = true;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::submitCurrent(bool wait)
{
	endActiveRenderPass();
	if (recording_) {
		if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
			recording_ = false;
			return fail(Kld::HalStatusCode::BackendError, "vkEndCommandBuffer failed");
		}
		FrameSlot& slot = frames_[currentSlot_];
		(void)vkResetFences(context_.device(), 1, &slot.fence);
		VkSubmitInfo submitInfo {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer_;
		if (vkQueueSubmit(context_.graphicsQueue(), 1, &submitInfo, slot.fence)
		    != VK_SUCCESS) {
			recording_ = false;
			return fail(Kld::HalStatusCode::BackendError, "vkQueueSubmit failed");
		}
		slot.pending = true;
		recording_ = false;
		// Debug labels are command-buffer scoped, so ending the buffer closes any
		// region that is still open instead of letting the depth drift.
		debugLabelDepth_ = 0;
	}
	if (wait) {
		waitForAllFrames();
	} else {
		// Keep the submission running and record into the next slot. Reusing this
		// slot waits for its fence in `ensureRecording()`, which is the ring.
		currentSlot_ = (currentSlot_ + 1) % kFramesInFlight;
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::submitAndWait()
{
	return submitCurrent(true);
}

void CommandExecutor::waitForAllFrames() noexcept
{
	if (vkWaitForFences == nullptr || context_.device() == VK_NULL_HANDLE) {
		return;
	}
	for (FrameSlot& slot : frames_) {
		if (!slot.pending) {
			continue;
		}
		(void)vkWaitForFences(context_.device(), 1, &slot.fence, VK_TRUE, UINT64_MAX);
		slot.pending = false;
		flushDeferred(slot.deferred);
	}
}

Kld::HalStatus CommandExecutor::flushForHostAccess()
{
	return submitAndWait();
}

void CommandExecutor::flushDeferred(std::vector<DeferredDestroy>& list) noexcept
{
	for (const DeferredDestroy& entry : list) {
		if (entry.buffer_ != VK_NULL_HANDLE || entry.bufferAllocation_ != VK_NULL_HANDLE) {
			vmaDestroyBuffer(memory_.handle(), entry.buffer_, entry.bufferAllocation_);
		}
		if (entry.view_ != VK_NULL_HANDLE) {
			vkDestroyImageView(context_.device(), entry.view_, nullptr);
		}
		if (entry.image_ != VK_NULL_HANDLE || entry.imageAllocation_ != VK_NULL_HANDLE) {
			vmaDestroyImage(memory_.handle(), entry.image_, entry.imageAllocation_);
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
	list.clear();
}

void CommandExecutor::notePresented() noexcept
{
	presentPending_ = true;
	if (presentMarkerArmed_) {
		// The armed marker sits *before* this present on the queue, so it no longer
		// proves that the newest present was consumed.
		presentMarkerStale_ = true;
	}
}

bool CommandExecutor::drainPresentQueue(std::uint64_t timeoutNanoseconds) noexcept
{
	if (!presentPending_) {
		return true;
	}
	if (vkWaitForFences == nullptr || context_.device() == VK_NULL_HANDLE) {
		return true;
	}
	const VkDevice device = context_.device();
	if (!presentMarkerArmed_) {
		// An empty submission on the presentation queue is a queue-ordered marker:
		// presentations are ordered with respect to other queue operations, so this
		// fence signals only after the queue consumed the presents queued before it.
		(void)vkResetFences(device, 1, &presentMarkerFence_);
		VkSubmitInfo marker {};
		marker.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		marker.commandBufferCount = 0;
		if (vkQueueSubmit(context_.presentQueue(), 1, &marker, presentMarkerFence_)
		    != VK_SUCCESS) {
			return false;
		}
		presentMarkerArmed_ = true;
		presentMarkerStale_ = false;
	}
	if (vkWaitForFences(device, 1, &presentMarkerFence_, VK_TRUE, timeoutNanoseconds)
	    != VK_SUCCESS) {
		// Still presenting. The armed marker stays, so a later call can reuse it
		// without another submission.
		return false;
	}
	presentMarkerArmed_ = false;
	(void)vkResetFences(device, 1, &presentMarkerFence_);
	if (presentMarkerStale_) {
		// A present was queued after the marker: it is drained, but the newest one
		// is not, so a fresh marker is needed before anything can be destroyed.
		presentMarkerStale_ = false;
		return false;
	}
	presentPending_ = false;
	return true;
}

void CommandExecutor::retireSwapchainHandle(VkSwapchainKHR swapchain) noexcept
{
	if (swapchain == VK_NULL_HANDLE || vkDestroySwapchainKHR == nullptr) {
		return;
	}
	if (!presentPending_) {
		// Nothing is queued on the presentation queue, so the handle is free.
		vkDestroySwapchainKHR(context_.device(), swapchain, nullptr);
		return;
	}
	retiredSwapchains_.push_back(swapchain);
}

void CommandExecutor::flushRetiredSwapchains(std::uint64_t timeoutNanoseconds) noexcept
{
	if (retiredSwapchains_.empty() || vkDestroySwapchainKHR == nullptr) {
		return;
	}
	if (presentPending_ && !drainPresentQueue(timeoutNanoseconds)) {
		return;
	}
	for (VkSwapchainKHR swapchain : retiredSwapchains_) {
		vkDestroySwapchainKHR(context_.device(), swapchain, nullptr);
	}
	retiredSwapchains_.clear();
}

void CommandExecutor::releaseAll() noexcept
{
	if (vkDeviceWaitIdle == nullptr || context_.device() == VK_NULL_HANDLE) {
		return;
	}
	(void)vkDeviceWaitIdle(context_.device());
	// The device is idle, so every present has completed and swapchain handles can
	// be destroyed immediately; the marker fence is no longer needed.
	presentPending_ = false;
	presentMarkerArmed_ = false;
	presentMarkerStale_ = false;
	flushRetiredSwapchains(0);
	endActiveRenderPass();
	for (auto& entry : swapchains_) {
		destroySwapchainObject(entry.second);
	}
	swapchains_.clear();
	destroySwapchainObjects();
	for (auto& entry : buffers_) {
		if (entry.second.buffer != VK_NULL_HANDLE
		    || entry.second.allocation != VK_NULL_HANDLE) {
			vmaDestroyBuffer(memory_.handle(), entry.second.buffer,
					 entry.second.allocation);
		}
	}
	const auto releaseTexture = [this](VkTextureObject& object) {
		if (object.view != VK_NULL_HANDLE) {
			vkDestroyImageView(context_.device(), object.view, nullptr);
		}
		if (object.image != VK_NULL_HANDLE || object.allocation != VK_NULL_HANDLE) {
			vmaDestroyImage(memory_.handle(), object.image, object.allocation);
		}
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
		for (VkDescriptorSetLayout setLayout : object.setLayouts) {
			if (setLayout != VK_NULL_HANDLE) {
				vkDestroyDescriptorSetLayout(context_.device(), setLayout, nullptr);
			}
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
	if (dummyTexture_.image != VK_NULL_HANDLE) {
		releaseTexture(dummyTexture_);
		dummyTexture_ = VkTextureObject {};
	}
	if (dummyBuffer_.buffer != VK_NULL_HANDLE) {
		destroyMappedBuffer(dummyBuffer_.buffer, dummyBuffer_.allocation,
				    dummyBuffer_.mapped);
		dummyBuffer_ = VkBufferObject {};
	}
	passInfos_.clear();
	if (defaultSampler_ != VK_NULL_HANDLE) {
		vkDestroySampler(context_.device(), defaultSampler_, nullptr);
		defaultSampler_ = VK_NULL_HANDLE;
	}
	if (acquireFence_ != VK_NULL_HANDLE) {
		vkDestroyFence(context_.device(), acquireFence_, nullptr);
		acquireFence_ = VK_NULL_HANDLE;
	}
	if (presentMarkerFence_ != VK_NULL_HANDLE) {
		vkDestroyFence(context_.device(), presentMarkerFence_, nullptr);
		presentMarkerFence_ = VK_NULL_HANDLE;
	}
	for (FrameSlot& slot : frames_) {
		releaseFrameSlot(slot);
	}
	recording_ = false;
}

void CommandExecutor::releaseFrameSlot(FrameSlot& slot) noexcept
{
	const VkDevice device = context_.device();
	flushDeferred(slot.deferred);
	if (slot.staging.buffer != VK_NULL_HANDLE || slot.staging.allocation != VK_NULL_HANDLE) {
		vmaDestroyBuffer(memory_.handle(), slot.staging.buffer, slot.staging.allocation);
		slot.staging = StagingRing {};
	}
	for (VkDescriptorPool pool : slot.descriptorPools) {
		if (pool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device, pool, nullptr);
		}
	}
	slot.descriptorPools.clear();
	if (slot.fence != VK_NULL_HANDLE) {
		vkDestroyFence(device, slot.fence, nullptr);
		slot.fence = VK_NULL_HANDLE;
	}
	if (slot.pool != VK_NULL_HANDLE) {
		// Destroying the pool also frees the command buffer it owns.
		vkDestroyCommandPool(device, slot.pool, nullptr);
		slot.pool = VK_NULL_HANDLE;
		slot.commands = VK_NULL_HANDLE;
	}
	slot.pending = false;
	commandPool_ = VK_NULL_HANDLE;
	commandBuffer_ = VK_NULL_HANDLE;
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
		const Kld::HalStatus flushed = submitCurrent(false);
		if (!flushed.ok()) {
			return flushed;
		}
	}
	// Swapchains parked by an earlier resize/destroy are released as soon as the
	// presentation queue caught up; the poll keeps that path non-blocking.
	if (kldStatus.ok()) {
		flushRetiredSwapchains(0);
	}
	return kldStatus;
}

Kld::HalStatus CommandExecutor::setViewport(const Kld::OpSetViewport& op)
{
	viewport_.x = op.x;
	// Kaldi's viewport follows the GL convention (origin at the bottom-left);
	// Vulkan's is top-left, so the rectangle is flipped to keep the same image.
	std::uint32_t height = 0;
	if (boundFramebuffer_ == Kld::DefaultFramebuffer) {
		// The default swapchain is created on first use, so its extent is only
		// known after this call.
		const Kld::HalStatus status = ensureSwapchain();
		if (!status.ok()) {
			return status;
		}
		height = swapchainExtent_.height;
	} else if (VkFramebufferObject* target = framebuffer(boundFramebuffer_)) {
		height = target->height;
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
	std::uint32_t height = 0;
	if (boundFramebuffer_ == Kld::DefaultFramebuffer) {
		const Kld::HalStatus status = ensureSwapchain();
		if (!status.ok()) {
			return status;
		}
		height = swapchainExtent_.height;
	} else if (VkFramebufferObject* target = framebuffer(boundFramebuffer_)) {
		height = target->height;
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

Kld::HalStatus CommandExecutor::debugLabelBegin(const Kld::OpDebugLabelBegin& op)
{
	if (!debugLabels_) {
		// No VK_EXT_debug_utils: capabilities() already reported
		// supportsDebugLabels == false, so the label is consumed and dropped.
		return Kld::HalOk;
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkDebugUtilsLabelEXT label {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = op.name != nullptr ? op.name : "";
	for (int channel = 0; channel < 4; ++channel) {
		label.color[channel] = op.color[channel];
	}
	cmdBeginDebugUtilsLabel_(commandBuffer_, &label);
	++debugLabelDepth_;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugLabelEnd(const Kld::OpDebugLabelEnd&)
{
	if (!debugLabels_ || debugLabelDepth_ == 0) {
		return Kld::HalOk;
	}
	if (recording_) {
		cmdEndDebugUtilsLabel_(commandBuffer_);
	}
	--debugLabelDepth_;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::debugMarker(const Kld::OpDebugMarker& op)
{
	if (!debugLabels_) {
		return Kld::HalOk;
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkDebugUtilsLabelEXT label {};
	label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
	label.pLabelName = op.name != nullptr ? op.name : "";
	for (int channel = 0; channel < 4; ++channel) {
		label.color[channel] = op.color[channel];
	}
	cmdInsertDebugUtilsLabel_(commandBuffer_, &label);
	return Kld::HalOk;
}

}
