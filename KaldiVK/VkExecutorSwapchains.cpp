/**
 * @file VkExecutorSwapchains.cpp
 * @brief Explicit swapchain objects (HAL 1.1) for the Vulkan backend.
 *
 * @section kaldivk_explicit_swapchains Semantics
 *
 * Vulkan owns its presentable images, so they cannot be wrapped as `Image`
 * handles. Instead every image gets a framebuffer in the backend-reserved handle
 * range (`Kld::BackendHandleBase` upwards) and `AcquireSwapchainImage` reports it
 * through `SwapchainAcquireInfo`::framebuffer. The whole draw path is therefore:
 *
 * @code
 * CreateSwapchain{ id, &descriptor }
 * AcquireSwapchainImage{ id, &callback, userData }   // -> framebuffer handle
 * BindFramebuffer{ reportedFramebuffer }
 * ...draw...
 * PresentSwapchain{ id, imageIndex }
 * @endcode
 *
 * `SwapchainImageBinding` entries may only carry zero image handles, because the
 * images stay backend-owned; the default frame path (`BeginFrame`/`Present` with
 * `DefaultFramebuffer`) is unaffected and keeps working exactly as before.
 *
 * The explicit render pass mirrors the default one: a single `PRESENT_SRC_KHR`
 * color attachment with `LOAD`/`STORE`, which keeps acquire/present layouts
 * compatible without extra transitions. `depthStencilFormat` must be left
 * `Format::INVALID`: an explicit depth attachment needs an image handle the
 * backend cannot invent, so it reports `UnsupportedBaseline` instead of silently
 * dropping the request.
 */
#include "VkCommandExecutor.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace Vk {

namespace {

/** @brief True when the surface advertises @p mode. */
bool presentModeSupported(VkPhysicalDevice device, VkSurfaceKHR surface, VkPresentModeKHR mode)
{
	std::uint32_t count = 0;
	if (vkGetPhysicalDeviceSurfacePresentModesKHR == nullptr
	    || vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr)
		       != VK_SUCCESS
	    || count == 0) {
		// FIFO is mandatory, so a surface that cannot enumerate its modes still
		// supports the safe default.
		return mode == VK_PRESENT_MODE_FIFO_KHR;
	}
	std::vector<VkPresentModeKHR> modes(count);
	if (vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data())
	    != VK_SUCCESS) {
		return mode == VK_PRESENT_MODE_FIFO_KHR;
	}
	return std::find(modes.begin(), modes.begin() + count, mode) != modes.begin() + count;
}

/** @brief Translates Kaldi's pacing policy into a Vulkan present mode. */
VkPresentModeKHR toVkPresentMode(Kld::PresentMode mode) noexcept
{
	switch (mode) {
		case Kld::PresentMode::Immediate: return VK_PRESENT_MODE_IMMEDIATE_KHR;
		case Kld::PresentMode::Mailbox: return VK_PRESENT_MODE_MAILBOX_KHR;
		case Kld::PresentMode::Fifo: break;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

/** @brief Reinterprets the descriptor's opaque surface pointer as `VkSurfaceKHR`. */
VkSurfaceKHR surfaceFromOpaque(void* opaque) noexcept
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	static_assert(sizeof(surface) == sizeof(opaque),
		      "a Vulkan surface handle must fit into the opaque pointer");
	std::memcpy(&surface, &opaque, sizeof(surface));
	return surface;
}

/** @brief True when @p format can be presented on @p surface. */
bool surfaceSupportsFormat(VkPhysicalDevice device, VkSurfaceKHR surface, VkFormat format)
{
	std::uint32_t count = 0;
	if (vkGetPhysicalDeviceSurfaceFormatsKHR == nullptr
	    || vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr) != VK_SUCCESS
	    || count == 0) {
		return false;
	}
	std::vector<VkSurfaceFormatKHR> formats(count);
	if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data())
	    != VK_SUCCESS) {
		return false;
	}
	return std::any_of(formats.begin(), formats.begin() + count,
			   [format](const VkSurfaceFormatKHR& candidate) {
				   // VK_FORMAT_UNDEFINED means "the surface accepts any format".
				   return candidate.format == format
					  || candidate.format == VK_FORMAT_UNDEFINED;
			   });
}

} // namespace

VkSwapchainObject* CommandExecutor::swapchain(Kld::HandleId id)
{
	const auto found = swapchains_.find(id);
	return found == swapchains_.end() ? nullptr : &found->second;
}

Kld::HandleId CommandExecutor::allocateBackendHandle()
{
	// Stay inside the reserved range and skip anything already published, so a
	// reported framebuffer can never shadow a live resource.
	constexpr Kld::HandleId kLimit = Kld::BackendHandleBase + 0x40000000u;
	while (nextBackendHandle_ < kLimit && framebuffers_.count(nextBackendHandle_) != 0) {
		++nextBackendHandle_;
	}
	return nextBackendHandle_ < kLimit ? nextBackendHandle_++ : 0;
}

bool CommandExecutor::ownsActiveFramebuffer(VkSwapchainObject& object) const noexcept
{
	if (boundFramebuffer_ == Kld::DefaultFramebuffer) {
		return false;
	}
	return std::find(object.framebufferHandles.begin(), object.framebufferHandles.end(),
			 boundFramebuffer_)
	       != object.framebufferHandles.end();
}

Kld::HalStatus CommandExecutor::buildSwapchainObjects(VkSwapchainObject& object,
						      VkSurfaceKHR surface,
						      VkExtent2D requestedExtent,
						      VkFormat format,
						      Kld::PresentMode presentMode,
						      std::uint32_t requestedImageCount)
{
	const VkPhysicalDevice physical = context_.physicalDevice();
	const VkDevice device = context_.device();
	VkSurfaceCapabilitiesKHR caps {};
	if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the native surface is not usable by this physical device");
	}
	if ((caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0) {
		return failUnsupported("the surface cannot be used as a color attachment target");
	}
	VkExtent2D extent = requestedExtent;
	if (extent.width == 0 || extent.height == 0) {
		extent = caps.currentExtent;
	}
	if (extent.width == UINT32_MAX || extent.height == UINT32_MAX) {
		extent = VkExtent2D { 1280, 720 };
	}
	extent.width = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
	extent.height =
		std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
	if (extent.width == 0 || extent.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the swapchain extent is zero after clamping to the surface limits");
	}

	VkPresentModeKHR vkPresentMode = toVkPresentMode(presentMode);
	if (!presentModeSupported(physical, surface, vkPresentMode)) {
		if (presentMode != Kld::PresentMode::Fifo) {
			return failUnsupported("the surface does not support the requested present mode");
		}
		vkPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	}

	std::uint32_t imageCount =
		requestedImageCount != 0 ? requestedImageCount : caps.minImageCount + 1;
	imageCount = std::max(imageCount, caps.minImageCount);
	if (caps.maxImageCount > 0) {
		imageCount = std::min(imageCount, caps.maxImageCount);
	}

	VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if ((caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0) {
		composite = static_cast<VkCompositeAlphaFlagBitsKHR>(caps.supportedCompositeAlpha
								     & (~caps.supportedCompositeAlpha
									+ 1));
	}

	// Two queue families need concurrent sharing: a copy recorded on the
	// graphics queue has to be visible to the presentation engine without an
	// explicit ownership transfer.
	const std::uint32_t families[2] = { context_.graphicsQueueFamily(),
					    context_.presentQueueFamily() };
	const bool separateFamilies = families[0] != families[1];

	VkSwapchainCreateInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;
	info.minImageCount = imageCount;
	info.imageFormat = format;
	info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
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
	info.presentMode = vkPresentMode;
	info.clipped = VK_TRUE;
	const VkResult createResult = vkCreateSwapchainKHR(device, &info, nullptr,
							   &object.swapchain);
	if (createResult != VK_SUCCESS) {
		object.swapchain = VK_NULL_HANDLE;
		if (createResult == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) {
			return fail(Kld::HalStatusCode::SwapchainLost,
				    "vkCreateSwapchainKHR reported VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: "
				    "another swapchain already owns this native window (the default "
				    "frame path claims it on first use of DefaultFramebuffer)");
		}
		return fail(Kld::HalStatusCode::SwapchainLost,
			    std::string("vkCreateSwapchainKHR failed: ") + resultName(createResult));
	}
	std::uint32_t actualCount = 0;
	if (vkGetSwapchainImagesKHR(device, object.swapchain, &actualCount, nullptr) != VK_SUCCESS
	    || actualCount == 0) {
		destroySwapchainObject(object);
		return fail(Kld::HalStatusCode::SwapchainLost,
			    "vkGetSwapchainImagesKHR reported no images");
	}
	object.images.resize(actualCount);
	if (vkGetSwapchainImagesKHR(device, object.swapchain, &actualCount, object.images.data())
	    != VK_SUCCESS) {
		destroySwapchainObject(object);
		return fail(Kld::HalStatusCode::SwapchainLost, "vkGetSwapchainImagesKHR failed");
	}

	VkAttachmentDescription color {};
	color.format = format;
	color.samples = VK_SAMPLE_COUNT_1_BIT;
	color.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// Acquire transitions the image in, present transitions it back out; the pass
	// itself stays in COLOR_ATTACHMENT_OPTIMAL so re-beginning it is valid.
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
	if (vkCreateRenderPass(device, &passInfo, nullptr, &object.renderPass) != VK_SUCCESS) {
		object.renderPass = VK_NULL_HANDLE;
		destroySwapchainObject(object);
		return fail(Kld::HalStatusCode::BackendError,
			    "vkCreateRenderPass failed for the swapchain");
	}
	VkPassInfo passRecord;
	passRecord.colorFormats.push_back(format);
	passInfos_[object.renderPass] = passRecord;

	// Framebuffer handles are reused across a resize, so a caller can keep the
	// handle it learned from an acquire callback.
	std::vector<Kld::HandleId> reusable = object.framebufferHandles;
	object.framebufferHandles.assign(actualCount, 0);
	for (std::uint32_t index = 0; index < actualCount; ++index) {
		const Kld::HandleId handle =
			index < reusable.size() ? reusable[index] : allocateBackendHandle();
		if (handle == 0) {
			destroySwapchainObject(object);
			return fail(Kld::HalStatusCode::AllocationFailed,
				    "the backend handle range is exhausted");
		}
		object.framebufferHandles[index] = handle;
	}
	for (std::size_t index = actualCount; index < reusable.size(); ++index) {
		framebuffers_.erase(reusable[index]);
	}

	object.views.resize(actualCount, VK_NULL_HANDLE);
	object.framebuffers.resize(actualCount, VK_NULL_HANDLE);
	for (std::uint32_t index = 0; index < actualCount; ++index) {
		VkImageViewCreateInfo viewInfo {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = object.images[index];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		if (vkCreateImageView(device, &viewInfo, nullptr, &object.views[index])
		    != VK_SUCCESS) {
			object.views[index] = VK_NULL_HANDLE;
			destroySwapchainObject(object);
			return fail(Kld::HalStatusCode::BackendError,
				    "vkCreateImageView failed for the swapchain");
		}
		VkFramebufferCreateInfo frameInfo {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = object.renderPass;
		frameInfo.attachmentCount = 1;
		frameInfo.pAttachments = &object.views[index];
		frameInfo.width = extent.width;
		frameInfo.height = extent.height;
		frameInfo.layers = 1;
		if (vkCreateFramebuffer(device, &frameInfo, nullptr, &object.framebuffers[index])
		    != VK_SUCCESS) {
			object.framebuffers[index] = VK_NULL_HANDLE;
			destroySwapchainObject(object);
			return fail(Kld::HalStatusCode::BackendError,
				    "vkCreateFramebuffer failed for the swapchain");
		}
		VkFramebufferObject entry;
		entry.renderPass = object.renderPass;
		entry.framebuffer = object.framebuffers[index];
		entry.width = extent.width;
		entry.height = extent.height;
		entry.colorFormats.push_back(format);
		entry.backendOwned = true;
		framebuffers_[object.framebufferHandles[index]] = entry;
	}

	object.surface = surface;
	object.format = format;
	object.extent = extent;
	object.presentMode = presentMode;
	object.currentImage = 0;
	object.acquired = false;
	return Kld::HalOk;
}

void CommandExecutor::destroySwapchainObject(VkSwapchainObject& object) noexcept
{
	if (vkDestroySwapchainKHR == nullptr) {
		return;
	}
	// A presentation still in flight keeps the swapchain busy; the submit fence
	// does not cover vkQueuePresentKHR.
	waitForPresentQueue();
	const VkDevice device = context_.device();
	for (VkFramebuffer framebuffer : object.framebuffers) {
		if (framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}
	}
	for (VkImageView view : object.views) {
		if (view != VK_NULL_HANDLE) {
			vkDestroyImageView(device, view, nullptr);
		}
	}
	if (object.renderPass != VK_NULL_HANDLE) {
		passInfos_.erase(object.renderPass);
		vkDestroyRenderPass(device, object.renderPass, nullptr);
	}
	if (object.swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, object.swapchain, nullptr);
	}
	for (Kld::HandleId handle : object.framebufferHandles) {
		framebuffers_.erase(handle);
	}
	object.swapchain = VK_NULL_HANDLE;
	object.renderPass = VK_NULL_HANDLE;
	object.images.clear();
	object.views.clear();
	object.framebuffers.clear();
	object.framebufferHandles.clear();
	object.acquired = false;
	object.currentImage = 0;
	// `surface`, `format`, `extent` and `presentMode` survive so a resize can
	// rebuild in place without asking the caller to repeat the descriptor.
}

Kld::HalStatus CommandExecutor::createSwapchain(const Kld::OpCreateSwapchain& op)
{
	if (op.id == Kld::DefaultSwapchain) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "DefaultSwapchain is reserved for the backend-owned path; use a nonzero id");
	}
	if (op.descriptor == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "CreateSwapchain requires a descriptor");
	}
	const Kld::SwapchainDescriptor& descriptor = *op.descriptor;
	if (isHandleInUse(op.id) || swapchains_.count(op.id) != 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "swapchain handle is already in use");
	}
	if (descriptor.depthStencilFormat != Kld::TextureFormat::INVALID) {
		return failUnsupported("explicit swapchains are color-only here; leave "
				       "depthStencilFormat unset and render depth into your own image");
	}
	for (std::uint32_t index = 0; index < descriptor.imageBindingCount; ++index) {
		const Kld::SwapchainImageBinding& binding = descriptor.imageBindings[index];
		if (binding.colorImage != 0 || binding.depthStencilImage != 0) {
			return failUnsupported("Vulkan owns its swapchain images, so colorImage and "
					       "depthStencilImage bindings must be zero; use the "
					       "framebuffer reported by AcquireSwapchainImage");
		}
	}
	VkSurfaceKHR surface = descriptor.nativeSurface != nullptr
				 ? surfaceFromOpaque(descriptor.nativeSurface)
				 : context_.surface();
	if (surface == VK_NULL_HANDLE) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "CreateSwapchain needs a native surface; the context has none");
	}
	const VkFormat format = translateToVkFormat(descriptor.colorFormat);
	if (format == VK_FORMAT_UNDEFINED) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the requested swapchain color format has no Vulkan equivalent");
	}
	if (!surfaceSupportsFormat(context_.physicalDevice(), surface, format)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the surface does not support the requested color format");
	}
	// A retargeted surface must not be recreated while earlier work still reads
	// the old swapchain.
	Kld::HalStatus status = submitAndWait();
	if (!status.ok()) {
		return status;
	}
	VkSwapchainObject object;
	status = buildSwapchainObjects(object, surface,
				       VkExtent2D { descriptor.width, descriptor.height }, format,
				       descriptor.presentMode, descriptor.imageCount);
	if (!status.ok()) {
		return status;
	}
	swapchains_.emplace(op.id, std::move(object));
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroySwapchain(const Kld::OpDestroySwapchain& op)
{
	if (op.id == Kld::DefaultSwapchain) {
		return Kld::HalOk; // The default target is backend-owned.
	}
	const auto found = swapchains_.find(op.id);
	if (found == swapchains_.end()) {
		return failInvalidHandle("swapchain", op.id);
	}
	Kld::HalStatus status = submitAndWait();
	if (!status.ok()) {
		return status;
	}
	if (ownsActiveFramebuffer(found->second)) {
		endActiveRenderPass();
		boundFramebuffer_ = Kld::DefaultFramebuffer;
	}
	destroySwapchainObject(found->second);
	swapchains_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resizeSwapchain(const Kld::OpResizeSwapchain& op)
{
	if (op.id == Kld::DefaultSwapchain) {
		Kld::HalStatus status = submitAndWait();
		if (!status.ok()) {
			return status;
		}
		destroySwapchainObjects();
		swapchainExtent_ = VkExtent2D { op.width, op.height };
		return createSwapchainObjects();
	}
	VkSwapchainObject* object = swapchain(op.id);
	if (object == nullptr) {
		return failInvalidHandle("swapchain", op.id);
	}
	for (std::uint32_t index = 0; index < op.imageBindingCount; ++index) {
		const Kld::SwapchainImageBinding& binding = op.imageBindings[index];
		if (binding.colorImage != 0 || binding.depthStencilImage != 0) {
			return failUnsupported("Vulkan owns its swapchain images, so colorImage and "
					       "depthStencilImage bindings must be zero");
		}
	}
	Kld::HalStatus status = submitAndWait();
	if (!status.ok()) {
		return status;
	}
	if (ownsActiveFramebuffer(*object)) {
		endActiveRenderPass();
		boundFramebuffer_ = Kld::DefaultFramebuffer;
	}
	// Rebuild in place, keeping the published framebuffer handles stable.
	const std::vector<Kld::HandleId> handles = object->framebufferHandles;
	destroySwapchainObject(*object);
	object->framebufferHandles = handles;
	status = buildSwapchainObjects(*object, object->surface,
				       VkExtent2D { op.width, op.height }, object->format,
				       object->presentMode, 0);
	if (!status.ok()) {
		// A half-rebuilt swapchain is useless; drop it instead of leaving a
		// handle that cannot present.
		swapchains_.erase(op.id);
		return status;
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::acquireSwapchainImage(const Kld::OpAcquireSwapchainImage& op)
{
	if (op.swapchain == Kld::DefaultSwapchain) {
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
			const Kld::SwapchainAcquireInfo info { op.swapchain,
							       Kld::SwapchainStatus::Ready,
							       currentImageIndex_, 0, 0,
							       Kld::DefaultFramebuffer };
			op.callback(&info, op.userData);
		}
		return Kld::HalOk;
	}
	VkSwapchainObject* object = swapchain(op.swapchain);
	if (object == nullptr) {
		return failInvalidHandle("swapchain", op.swapchain);
	}
	if (object->swapchain == VK_NULL_HANDLE) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "the swapchain has no images; recreate it");
	}
	// The previously acquired image may still be presented by queued work.
	Kld::HalStatus status = submitAndWait();
	if (!status.ok()) {
		return status;
	}
	(void)vkResetFences(context_.device(), 1, &acquireFence_);
	std::uint32_t index = 0;
	const VkResult result = vkAcquireNextImageKHR(context_.device(), object->swapchain,
						      UINT64_MAX, VK_NULL_HANDLE, acquireFence_, &index);
	Kld::SwapchainStatus state = Kld::SwapchainStatus::Ready;
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		state = Kld::SwapchainStatus::Lost;
	} else if (result == VK_SUBOPTIMAL_KHR) {
		state = Kld::SwapchainStatus::Suboptimal;
	} else if (result != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::SwapchainLost,
			    "vkAcquireNextImageKHR failed for the explicit swapchain");
	}
	Kld::HandleId framebuffer = 0;
	if (state != Kld::SwapchainStatus::Lost) {
		(void)vkWaitForFences(context_.device(), 1, &acquireFence_, VK_TRUE, UINT64_MAX);
		(void)vkResetFences(context_.device(), 1, &acquireFence_);
		object->currentImage = index;
		object->acquired = true;
		if (index < object->framebufferHandles.size()) {
			framebuffer = object->framebufferHandles[index];
		}
		// Hand the image to the render pass in the layout it expects.
		status = ensureRecording();
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
		barrier.image = object->images[index];
		barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		vkCmdPipelineBarrier(commandBuffer_,
				     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr,
				     0, nullptr, 1, &barrier);
	}
	if (op.callback != nullptr) {
		const Kld::SwapchainAcquireInfo info { op.swapchain, state, index, 0, 0, framebuffer };
		op.callback(&info, op.userData);
	}
	if (state == Kld::SwapchainStatus::Lost) {
		object->acquired = false;
		return fail(Kld::HalStatusCode::SwapchainLost,
			    "the explicit swapchain is out of date; resize it");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::presentSwapchain(const Kld::OpPresentSwapchain& op)
{
	if (op.swapchain == Kld::DefaultSwapchain) {
		return present(Kld::OpPresent { Kld::DefaultFramebuffer });
	}
	VkSwapchainObject* object = swapchain(op.swapchain);
	if (object == nullptr) {
		return failInvalidHandle("swapchain", op.swapchain);
	}
	if (!object->acquired) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "PresentSwapchain needs an acquired image (AcquireSwapchainImage)");
	}
	if (op.imageIndex != object->currentImage) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the image index does not match the image AcquireSwapchainImage reported");
	}
	if (ownsActiveFramebuffer(*object)) {
		endActiveRenderPass();
	}
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
	barrier.image = object->images[object->currentImage];
	barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
			     &barrier);
	const bool sameQueue = context_.presentQueue() == context_.graphicsQueue();
	status = submitCurrent(!sameQueue);
	if (!status.ok()) {
		return status;
	}
	VkPresentInfoKHR presentInfo {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &object->swapchain;
	presentInfo.pImageIndices = &object->currentImage;
	const VkResult result = vkQueuePresentKHR(context_.presentQueue(), &presentInfo);
	object->acquired = false;
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return fail(Kld::HalStatusCode::SwapchainLost,
			    "vkQueuePresentKHR reported OUT_OF_DATE for the explicit swapchain");
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		return fail(Kld::HalStatusCode::BackendError, "vkQueuePresentKHR failed");
	}
	return Kld::HalOk;
}

} // namespace Vk

