/**
 * @file VkExecutorFrames.cpp
 * @brief Default swapchain, render passes, clears and frame lifecycle.
 */
#include "VkCommandExecutor.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace Vk {

Kld::HalStatus CommandExecutor::ensureSwapchain()
{
	if (swapchain_ != VK_NULL_HANDLE) {
		return Kld::HalOk;
	}
	return createSwapchainObjects();
}

Kld::HalStatus CommandExecutor::createSwapchainObjects(VkSwapchainKHR oldSwapchain)
{
	VkPhysicalDevice device = context_.physicalDevice();
	VkSurfaceKHR surface = context_.surface();
	VkSurfaceCapabilitiesKHR caps {};
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &caps) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::BackendError, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
	}
	std::uint32_t formatCount = 0;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) != VK_SUCCESS
	    || formatCount == 0) {
		return fail(Kld::HalStatusCode::BackendError, "the surface reports no formats");
	}
	std::vector<VkSurfaceFormatKHR> formats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
	VkSurfaceFormatKHR chosen = formats.front();
	for (const VkSurfaceFormatKHR& candidate : formats) {
		if ((candidate.format == VK_FORMAT_R8G8B8A8_UNORM
		     || candidate.format == VK_FORMAT_B8G8R8A8_UNORM)
		    && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			chosen = candidate;
			break;
		}
	}
	swapchainFormat_ = chosen.format;

	VkExtent2D extent = caps.currentExtent;
	if (extent.width == UINT32_MAX || extent.height == UINT32_MAX) {
		extent.width = swapchainExtent_.width != 0 ? swapchainExtent_.width : 1280;
		extent.height = swapchainExtent_.height != 0 ? swapchainExtent_.height : 720;
		extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
		extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
	}
	swapchainExtent_ = extent;

	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if ((caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0) {
		composite = static_cast<VkCompositeAlphaFlagBitsKHR>(caps.supportedCompositeAlpha
								     & (~caps.supportedCompositeAlpha + 1));
	}
	std::uint32_t imageCount = caps.minImageCount + 1;
	if (caps.maxImageCount > 0) {
		imageCount = std::min(imageCount, caps.maxImageCount);
	}
	// Two queue families need concurrent sharing; the transfer recorded on the
	// graphics queue must be visible to the presentation engine without an
	// explicit ownership transfer.
	const std::uint32_t families[2] = { context_.graphicsQueueFamily(),
					    context_.presentQueueFamily() };
	const bool separateFamilies = families[0] != families[1];
	VkSwapchainCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.minImageCount = imageCount;
	info.imageFormat = swapchainFormat_;
	info.imageColorSpace = chosen.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	info.imageSharingMode =
		separateFamilies ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	if (separateFamilies) {
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = families;
	}
	info.preTransform = caps.currentTransform;
	info.compositeAlpha = composite;
	info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	info.clipped = VK_TRUE;
	// Hand the previous swapchain over as `oldSwapchain`: the driver retires it as
	// part of the replacement, which is the only portable way to recreate on a
	// native window that can back a single swapchain.
	info.oldSwapchain = oldSwapchain;
	if (vkCreateSwapchainKHR(context_.device(), &info, nullptr, &swapchain_) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::SwapchainLost, "vkCreateSwapchainKHR failed");
	}
	imageCount = 0;
	vkGetSwapchainImagesKHR(context_.device(), swapchain_, &imageCount, nullptr);
	swapchainImages_.resize(imageCount);
	vkGetSwapchainImagesKHR(context_.device(), swapchain_, &imageCount, swapchainImages_.data());
	VkAttachmentDescription color {};
	color.format = swapchainFormat_;
	color.samples = VK_SAMPLE_COUNT_1_BIT;
	// The acquired image is transitioned to COLOR_ATTACHMENT_OPTIMAL right after
	// the acquire, and present transitions it back, so the pass itself stays in
	// that layout. LOAD (not CLEAR) keeps a pass that is re-begun after a mid-frame
	// flush from wiping the image; BeginFrame clears explicitly.
	color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	color.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference colorRef { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	VkSubpassDependency dependency {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	VkRenderPassCreateInfo passInfo {};
	passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	passInfo.attachmentCount = 1;
	passInfo.pAttachments = &color;
	passInfo.subpassCount = 1;
	passInfo.pSubpasses = &subpass;
	passInfo.dependencyCount = 1;
	passInfo.pDependencies = &dependency;
	if (vkCreateRenderPass(context_.device(), &passInfo, nullptr, &swapchainRenderPass_) != VK_SUCCESS) {
		destroySwapchainObjects();
		return fail(Kld::HalStatusCode::BackendError, "vkCreateRenderPass failed");
	}
	VkPassInfo passRecord;
	passRecord.colorFormats.push_back(swapchainFormat_);
	passInfos_[swapchainRenderPass_] = passRecord;
	swapchainViews_.resize(swapchainImages_.size());
	swapchainFramebuffers_.resize(swapchainImages_.size());
	for (std::size_t index = 0; index < swapchainImages_.size(); ++index) {
		VkImageViewCreateInfo viewInfo {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = swapchainImages_[index];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = swapchainFormat_;
		viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		if (vkCreateImageView(context_.device(), &viewInfo, nullptr, &swapchainViews_[index])
		    != VK_SUCCESS) {
			destroySwapchainObjects();
			return fail(Kld::HalStatusCode::BackendError, "vkCreateImageView failed for the swapchain");
		}
		VkFramebufferCreateInfo frameInfo {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = swapchainRenderPass_;
		frameInfo.attachmentCount = 1;
		frameInfo.pAttachments = &swapchainViews_[index];
		frameInfo.width = extent.width;
		frameInfo.height = extent.height;
		frameInfo.layers = 1;
		if (vkCreateFramebuffer(context_.device(), &frameInfo, nullptr,
					&swapchainFramebuffers_[index]) != VK_SUCCESS) {
			destroySwapchainObjects();
			return fail(Kld::HalStatusCode::BackendError, "vkCreateFramebuffer failed for the swapchain");
		}
	}
	return Kld::HalOk;
}

void CommandExecutor::releaseSwapchainObjects() noexcept
{
	// Retired through the current frame slot: the views/framebuffers only have to
	// outlive the command buffers that reference them, and this slot's fence covers
	// every submission before it.
	for (VkFramebuffer framebuffer : swapchainFramebuffers_) {
		if (framebuffer == VK_NULL_HANDLE) {
			continue;
		}
		DeferredDestroy entry {};
		entry.framebuffer_ = framebuffer;
		defer(entry);
	}
	for (VkImageView view : swapchainViews_) {
		if (view == VK_NULL_HANDLE) {
			continue;
		}
		DeferredDestroy entry {};
		entry.view_ = view;
		defer(entry);
	}
	if (swapchainRenderPass_ != VK_NULL_HANDLE) {
		passInfos_.erase(swapchainRenderPass_);
		DeferredDestroy entry {};
		entry.renderPass_ = swapchainRenderPass_;
		defer(entry);
	}
	swapchainFramebuffers_.clear();
	swapchainViews_.clear();
	swapchainImages_.clear();
	swapchainRenderPass_ = VK_NULL_HANDLE;
	imageAcquired_ = false;
	activePass_ = VK_NULL_HANDLE;
	activeFramebuffer_ = VK_NULL_HANDLE;
}

void CommandExecutor::destroySwapchainObjects() noexcept
{
	if (vkDestroySwapchainKHR == nullptr) {
		return;
	}
	releaseSwapchainObjects();
	if (swapchain_ != VK_NULL_HANDLE) {
		// The presentation queue is not covered by the submit fences, so the handle
		// is destroyed now when it is free and parked otherwise.
		retireSwapchainHandle(swapchain_);
		swapchain_ = VK_NULL_HANDLE;
	}
}

Kld::HalStatus CommandExecutor::acquireDefaultImage()
{
	if (swapchain_ == VK_NULL_HANDLE) {
		return fail(Kld::HalStatusCode::SwapchainLost, "no swapchain exists");
	}
	(void)vkResetFences(context_.device(), 1, &acquireFence_);
	VkResult result = vkAcquireNextImageKHR(context_.device(), swapchain_, UINT64_MAX,
						VK_NULL_HANDLE, acquireFence_, &currentImageIndex_);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		Kld::HalStatus status = submitAndWait();
		if (!status.ok()) {
			return status;
		}
		// Recreate through `oldSwapchain` instead of destroy-then-create: a native
		// window can back only one swapchain, and the presentation queue may still
		// hold the previous one.
		const VkSwapchainKHR previous = swapchain_;
		releaseSwapchainObjects();
		swapchain_ = VK_NULL_HANDLE;
		status = createSwapchainObjects(previous);
		retireSwapchainHandle(previous);
		if (!status.ok()) {
			return status;
		}
		(void)vkResetFences(context_.device(), 1, &acquireFence_);
		result = vkAcquireNextImageKHR(context_.device(), swapchain_, UINT64_MAX, VK_NULL_HANDLE,
					       acquireFence_, &currentImageIndex_);
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		return fail(Kld::HalStatusCode::SwapchainLost, "vkAcquireNextImageKHR failed");
	}
	(void)vkWaitForFences(context_.device(), 1, &acquireFence_, VK_TRUE, UINT64_MAX);
	(void)vkResetFences(context_.device(), 1, &acquireFence_);
	imageAcquired_ = true;
	// The presentation engine hands the image over in an undefined layout; the
	// render pass expects COLOR_ATTACHMENT_OPTIMAL. Discarding the old contents is
	// intended: BeginFrame clears the image explicitly.
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = swapchainImages_[currentImageIndex_];
	barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0,
			     nullptr, 1, &barrier);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::ensureRenderPass(Kld::HandleId framebufferId)
{
	if (activePass_ != VK_NULL_HANDLE && boundFramebuffer_ == framebufferId) {
		return Kld::HalOk;
	}
	endActiveRenderPass();
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkRenderPass pass = VK_NULL_HANDLE;
	VkFramebuffer framebufferHandle = VK_NULL_HANDLE;
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	if (framebufferId == Kld::DefaultFramebuffer) {
		status = ensureSwapchain();
		if (!status.ok()) {
			return status;
		}
		if (!imageAcquired_) {
			status = acquireDefaultImage();
			if (!status.ok()) {
				return status;
			}
		}
		pass = swapchainRenderPass_;
		framebufferHandle = swapchainFramebuffers_[currentImageIndex_];
		width = swapchainExtent_.width;
		height = swapchainExtent_.height;
	} else {
		VkFramebufferObject* target = framebuffer(framebufferId);
		if (target == nullptr) {
			return failInvalidHandle("framebuffer", framebufferId);
		}
		pass = target->renderPass;
		framebufferHandle = target->framebuffer;
		width = target->width;
		height = target->height;
	}
	VkRenderPassBeginInfo beginInfo {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = pass;
	beginInfo.framebuffer = framebufferHandle;
	beginInfo.renderArea = VkRect2D { { 0, 0 }, { width, height } };
	vkCmdBeginRenderPass(commandBuffer_, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	activePass_ = pass;
	activeFramebuffer_ = framebufferHandle;
	boundFramebuffer_ = framebufferId;

	VkViewport viewport = viewportSet_ ? viewport_
					   : VkViewport { 0.0f, 0.0f, static_cast<float>(width),
							  static_cast<float>(height), 0.0f, 1.0f };
	VkRect2D scissor = scissorSet_ ? scissor_ : VkRect2D { { 0, 0 }, { width, height } };
	// A scissor must stay inside the render area; clamp instead of failing.
	scissor.offset.x = std::max<std::int32_t>(0, std::min(scissor.offset.x,
							     static_cast<std::int32_t>(width)));
	scissor.offset.y = std::max<std::int32_t>(0, std::min(scissor.offset.y,
							     static_cast<std::int32_t>(height)));
	scissor.extent.width = std::min(scissor.extent.width,
					static_cast<std::uint32_t>(width - scissor.offset.x));
	scissor.extent.height = std::min(scissor.extent.height,
					 static_cast<std::uint32_t>(height - scissor.offset.y));
	vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);
	return Kld::HalOk;
}

void CommandExecutor::endActiveRenderPass()
{
	if (activePass_ == VK_NULL_HANDLE) {
		return;
	}
	if (recording_) {
		vkCmdEndRenderPass(commandBuffer_);
	}
	activePass_ = VK_NULL_HANDLE;
	activeFramebuffer_ = VK_NULL_HANDLE;
}

Kld::HalStatus CommandExecutor::beginFrame(const Kld::OpBeginFrame& op)
{
	if (op.framebuffer != Kld::DefaultFramebuffer) {
		return failUnsupported("BeginFrame only supports the default framebuffer in this backend");
	}
	if (frameActive_) {
		return fail(Kld::HalStatusCode::InvalidResourceState, "a frame is already active");
	}
	boundFramebuffer_ = Kld::DefaultFramebuffer;
	// A new frame starts from default dynamic state; client commands re-apply it.
	viewportSet_ = false;
	scissorSet_ = false;
	Kld::HalStatus status = ensureRenderPass(Kld::DefaultFramebuffer);
	if (!status.ok()) {
		return status;
	}
	VkClearAttachment clear {};
	clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clear.colorAttachment = 0;
	clear.clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	VkClearRect rect {};
	rect.rect = { { 0, 0 }, { swapchainExtent_.width, swapchainExtent_.height } };
	rect.layerCount = 1;
	vkCmdClearAttachments(commandBuffer_, 1, &clear, 1, &rect);
	frameActive_ = true;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::endFrame(const Kld::OpEndFrame& op)
{
	if (op.framebuffer != Kld::DefaultFramebuffer && boundFramebuffer_ != op.framebuffer) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "EndFrame does not match the framebuffer BeginFrame selected");
	}
	endActiveRenderPass();
	frameActive_ = false;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::present(const Kld::OpPresent& op)
{
	if (op.framebuffer != Kld::DefaultFramebuffer) {
		return failUnsupported("Present only supports the backend-owned default framebuffer");
	}
	if (!imageAcquired_) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "Present needs an acquired swapchain image (BeginFrame)");
	}
	endActiveRenderPass();
	// Present needs the image back in PRESENT_SRC_KHR, which the render pass
	// deliberately does not do (it ends in COLOR_ATTACHMENT_OPTIMAL so a re-begun
	// pass stays valid).
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	VkImageMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = swapchainImages_[currentImageIndex_];
	barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
			     &barrier);
	// The present queue is usually the graphics queue, and then queue order alone
	// guarantees the render finishes before the presentation engine reads the
	// image, so waiting would only remove the overlap. Two different queues need
	// the explicit drain.
	const bool sameQueue = context_.presentQueue() == context_.graphicsQueue();
	status = submitCurrent(!sameQueue);
	if (!status.ok()) {
		return status;
	}
	VkPresentInfoKHR presentInfo {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain_;
	presentInfo.pImageIndices = &currentImageIndex_;
	const VkResult result = vkQueuePresentKHR(context_.presentQueue(), &presentInfo);
	imageAcquired_ = false;
	frameActive_ = false;
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		destroySwapchainObjects();
		return fail(Kld::HalStatusCode::SwapchainLost, "vkQueuePresentKHR reported OUT_OF_DATE");
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		return fail(Kld::HalStatusCode::BackendError, "vkQueuePresentKHR failed");
	}
	notePresented();
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindFramebuffer(const Kld::OpBindFramebuffer& op)
{
	if (op.id != Kld::DefaultFramebuffer && framebuffer(op.id) == nullptr) {
		return failInvalidHandle("framebuffer", op.id);
	}
	if (boundFramebuffer_ != op.id) {
		endActiveRenderPass();
		// Different target: start from default dynamic state.
		viewportSet_ = false;
		scissorSet_ = false;
	}
	boundFramebuffer_ = op.id;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::applyClear(const Kld::OpClearColorAttachment& op)
{
	Kld::HalStatus status = ensureRenderPass(op.framebuffer);
	if (!status.ok()) {
		return status;
	}
	std::uint32_t width = swapchainExtent_.width;
	std::uint32_t height = swapchainExtent_.height;
	if (boundFramebuffer_ != Kld::DefaultFramebuffer) {
		if (VkFramebufferObject* target = framebuffer(boundFramebuffer_)) {
			width = target->width;
			height = target->height;
		}
	}
	VkClearAttachment clear {};
	clear.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clear.colorAttachment = op.colorAttachmentIndex;
	clear.clearValue.color = { { op.color[0], op.color[1], op.color[2], op.color[3] } };
	VkClearRect rect {};
	rect.rect = { { 0, 0 }, { width, height } };
	rect.layerCount = 1;
	vkCmdClearAttachments(commandBuffer_, 1, &clear, 1, &rect);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::clearColorAttachment(const Kld::OpClearColorAttachment& op)
{
	if (op.colorAttachmentIndex >= capabilities_.maxColorAttachments) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "color attachment index exceeds the probed limit");
	}
	return applyClear(op);
}

Kld::HalStatus CommandExecutor::clearDepthStencilAttachment(
	const Kld::OpClearDepthStencilAttachment& op)
{
	Kld::HalStatus status = ensureRenderPass(op.framebuffer);
	if (!status.ok()) {
		return status;
	}
	std::uint32_t width = swapchainExtent_.width;
	std::uint32_t height = swapchainExtent_.height;
	if (boundFramebuffer_ != Kld::DefaultFramebuffer) {
		if (VkFramebufferObject* target = framebuffer(boundFramebuffer_)) {
			width = target->width;
			height = target->height;
		}
	}
	VkClearAttachment clear {};
	clear.aspectMask = 0;
	if (op.clearDepth) {
		clear.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	if (op.clearStencil) {
		clear.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	if (clear.aspectMask == 0) {
		return Kld::HalOk;
	}
	clear.clearValue.depthStencil = { op.depth, op.stencil };
	VkClearRect rect {};
	rect.rect = { { 0, 0 }, { width, height } };
	rect.layerCount = 1;
	vkCmdClearAttachments(commandBuffer_, 1, &clear, 1, &rect);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::beginRenderPass(const Kld::OpBeginRenderPass& op)
{
	const Kld::RenderPassDescriptor* descriptor = op.descriptor;
	if (descriptor == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "BeginRenderPass requires a descriptor");
	}
	Kld::HalStatus status = bindFramebuffer(Kld::OpBindFramebuffer { descriptor->framebuffer });
	if (!status.ok()) {
		return status;
	}
	status = ensureRenderPass(descriptor->framebuffer);
	if (!status.ok()) {
		return status;
	}
	for (std::uint32_t index = 0; index < descriptor->colorAttachmentCount; ++index) {
		const Kld::RenderPassColorAttachment& attachment = descriptor->colorAttachments[index];
		if (attachment.loadOp != Kld::AttachmentLoadOp::Clear) {
			continue;
		}
		Kld::OpClearColorAttachment clear {};
		clear.framebuffer = descriptor->framebuffer;
		clear.colorAttachmentIndex = attachment.colorAttachmentIndex;
		for (int channel = 0; channel < 4; ++channel) {
			clear.color[channel] = attachment.clearColor[channel];
		}
		status = applyClear(clear);
		if (!status.ok()) {
			return status;
		}
	}
	const Kld::RenderPassDepthStencilAttachment& depthStencil = descriptor->depthStencilAttachment;
	if (depthStencil.enabled) {
		Kld::OpClearDepthStencilAttachment clear {};
		clear.framebuffer = descriptor->framebuffer;
		clear.clearDepth = depthStencil.depthLoadOp == Kld::AttachmentLoadOp::Clear;
		clear.clearStencil = depthStencil.stencilLoadOp == Kld::AttachmentLoadOp::Clear;
		clear.depth = depthStencil.clearDepth;
		clear.stencil = depthStencil.clearStencil;
		status = clearDepthStencilAttachment(clear);
		if (!status.ok()) {
			return status;
		}
	}
	// AttachmentStoreOp::DontCare is not acted on; the attachment is stored.
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::endRenderPass(const Kld::OpEndRenderPass&)
{
	endActiveRenderPass();
	return Kld::HalOk;
}

}
