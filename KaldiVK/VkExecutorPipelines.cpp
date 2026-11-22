/**
 * @file VkExecutorPipelines.cpp
 * @brief Pipeline creation, resource binding and draw handlers for the Vulkan executor.
 *
 * @section kaldivk_pipelines_descriptors Descriptor model
 *
 * Vulkan has one binding namespace per descriptor set, while Kaldi's slots are
 * per-kind (texture units, sampler units, uniform blocks, ...). The executor maps
 * every kind into set 0 at the portable slot number, which is exactly how a shader
 * that targets Vulkan declares its bindings - and it rejects two *different*
 * kinds claiming the same slot at pipeline creation. A texture and a sampler on
 * the same slot collapse into one `COMBINED_IMAGE_SAMPLER`, which is the Vulkan
 * equivalent of GL's "texture unit with a sampler object".
 *
 * A pipeline that declares slots requires every declared slot to be bound before
 * it draws; `prepareDraw()` reports `InvalidResourceState` otherwise, because a
 * Vulkan draw with an unwritten descriptor is undefined behaviour rather than a
 * silently ignored binding.
 *
 * @section kaldivk_pipelines_variants Pipeline variants
 *
 * Blend, depth/stencil, cull, topology and depth bias are baked into a
 * `VkPipeline`, and attachment formats come from the render pass, so every
 * (pass, state) pair gets its own variant, cached inside the pipeline object.
 */
#include "VkCommandExecutor.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

namespace Vk {

namespace {

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

Kld::HalStatus CommandExecutor::createPipelineLayout(VkPipelineObject& object,
						      const Kld::PipelineBindingLayout* layout)
{
	constexpr std::array<VkDescriptorType, 5> kindTypes {
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
	};
	for (std::vector<VkDescriptorSlot>& slots : object.slotsByKind) {
		slots.clear();
	}
	object.poolSizes.clear();
	const auto addSlot = [&object, &kindTypes](std::size_t kind, std::uint32_t binding,
						   std::uint32_t count, VkShaderStageFlags stages) {
		std::vector<VkDescriptorSlot>& slots = object.slotsByKind[kind];
		for (VkDescriptorSlot& slot : slots) {
			if (slot.binding == binding) {
				// The same slot declared twice (for example once per stage) widens
				// the array and merges the stage flags.
				slot.count = std::max(slot.count, std::max(count, 1u));
				slot.stages |= stages;
				return;
			}
		}
		slots.push_back(VkDescriptorSlot { binding, kindTypes[kind], std::max(count, 1u), stages });
	};
	if (layout != nullptr) {
		for (std::uint32_t index = 0; index < layout->sampledTextureCount; ++index) {
			const Kld::SampledTextureBindingLayout& entry = layout->sampledTextures[index];
			addSlot(0, entry.slot, entry.count, shaderStage(entry.stages));
		}
		for (std::uint32_t index = 0; index < layout->samplerCount; ++index) {
			const Kld::SamplerBindingLayout& entry = layout->samplers[index];
			addSlot(1, entry.slot, entry.count, shaderStage(entry.stages));
		}
		for (std::uint32_t index = 0; index < layout->uniformBufferCount; ++index) {
			const Kld::UniformBufferBindingLayout& entry = layout->uniformBuffers[index];
			addSlot(2, entry.slot, 1, shaderStage(entry.stages));
		}
		for (std::uint32_t index = 0; index < layout->storageBufferCount; ++index) {
			const Kld::StorageBufferBindingLayout& entry = layout->storageBuffers[index];
			addSlot(3, entry.slot, 1, shaderStage(entry.stages));
		}
		for (std::uint32_t index = 0; index < layout->storageImageCount; ++index) {
			const Kld::StorageImageBindingLayout& entry = layout->storageImages[index];
			addSlot(4, entry.slot, 1, shaderStage(entry.stages));
		}
	}
	std::map<VkDescriptorType, std::uint32_t> poolSizes;
	for (std::size_t kind = 0; kind < object.slotsByKind.size(); ++kind) {
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (const VkDescriptorSlot& slot : object.slotsByKind[kind]) {
			VkDescriptorSetLayoutBinding binding {};
			binding.binding = slot.binding;
			binding.descriptorType = slot.type;
			binding.descriptorCount = slot.count;
			binding.stageFlags = slot.stages == 0
				? VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT
				: slot.stages;
			bindings.push_back(binding);
			poolSizes[slot.type] += slot.count;
		}
		VkDescriptorSetLayoutCreateInfo layoutInfo {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.empty() ? nullptr : bindings.data();
		if (vkCreateDescriptorSetLayout(context_.device(), &layoutInfo, nullptr,
					       &object.setLayouts[kind]) != VK_SUCCESS) {
			for (std::size_t created = 0; created < kind; ++created) {
				vkDestroyDescriptorSetLayout(context_.device(), object.setLayouts[created],
							     nullptr);
				object.setLayouts[created] = VK_NULL_HANDLE;
			}
			return fail(Kld::HalStatusCode::PipelineCreationFailed,
				    "vkCreateDescriptorSetLayout failed");
		}
	}
	for (const auto& entry : poolSizes) {
		object.poolSizes.push_back(VkDescriptorPoolSize { entry.first, entry.second });
	}
	VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<std::uint32_t>(object.setLayouts.size());
	pipelineLayoutInfo.pSetLayouts = object.setLayouts.data();
	if (vkCreatePipelineLayout(context_.device(), &pipelineLayoutInfo, nullptr, &object.layout)
	    != VK_SUCCESS) {
		for (VkDescriptorSetLayout setLayout : object.setLayouts) {
			vkDestroyDescriptorSetLayout(context_.device(), setLayout, nullptr);
		}
		object.setLayouts.fill(VK_NULL_HANDLE);
		return fail(Kld::HalStatusCode::PipelineCreationFailed, "vkCreatePipelineLayout failed");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createGraphicsVariant(VkPipelineObject& object,
						      VkRenderPass renderPass,
						      const VkPipelineState& state,
						      VkPipeline& pipeline)
{
	const auto passFound = passInfos_.find(renderPass);
	if (passFound == passInfos_.end()) {
		return fail(Kld::HalStatusCode::BackendError,
			    "the render pass has no registered attachment formats");
	}
	const VkPassInfo& pass = passFound->second;
	std::vector<VkShaderModule> modules(object.spirv.size(), VK_NULL_HANDLE);
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	const auto cleanup = [&] {
		for (const VkShaderModule module : modules) {
			if (module != VK_NULL_HANDLE) {
				vkDestroyShaderModule(context_.device(), module, nullptr);
			}
		}
	};
	for (std::size_t index = 0; index < object.spirv.size(); ++index) {
		if (object.spirv[index].empty() || index >= object.stages.size()) {
			cleanup();
			return fail(Kld::HalStatusCode::InvalidCommand, "a shader binary has no SPIR-V words");
		}
		VkShaderModuleCreateInfo moduleInfo {};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = object.spirv[index].size() * sizeof(std::uint32_t);
		moduleInfo.pCode = object.spirv[index].data();
		if (vkCreateShaderModule(context_.device(), &moduleInfo, nullptr, &modules[index])
		    != VK_SUCCESS) {
			cleanup();
			return fail(Kld::HalStatusCode::ShaderCreationFailed, "vkCreateShaderModule failed");
		}
		VkPipelineShaderStageCreateInfo stage {};
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.stage = object.stages[index];
		stage.module = modules[index];
		stage.pName = "main";
		stages.push_back(stage);
	}
	VkVertexInputBindingDescription vertexBinding {};
	std::vector<VkVertexInputAttributeDescription> attributes;
	if (!object.attributes.empty() && object.stride != 0) {
		vertexBinding.binding = 0;
		vertexBinding.stride = static_cast<std::uint32_t>(object.stride);
		vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		for (std::size_t location = 0; location < object.attributes.size(); ++location) {
			const VkFormat format = translateToVkFormat(object.attributes[location].type);
			if (format == VK_FORMAT_UNDEFINED) {
				cleanup();
				return failUnsupported(
					"a vertex attribute uses a format Vulkan cannot express");
			}
			VkVertexInputAttributeDescription attribute {};
			attribute.location = static_cast<std::uint32_t>(location);
			attribute.binding = 0;
			attribute.format = format;
			attribute.offset = static_cast<std::uint32_t>(object.attributes[location].offset);
			attributes.push_back(attribute);
		}
	}
	VkPipelineVertexInputStateCreateInfo vertexInput {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = attributes.empty() ? 0 : 1;
	vertexInput.pVertexBindingDescriptions = attributes.empty() ? nullptr : &vertexBinding;
	vertexInput.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
	vertexInput.pVertexAttributeDescriptions = attributes.empty() ? nullptr : attributes.data();
	VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = state.lines ? VK_PRIMITIVE_TOPOLOGY_LINE_LIST
					    : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	VkPipelineViewportStateCreateInfo viewportState {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	VkPipelineRasterizationStateCreateInfo raster {};
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.depthClampEnable = state.depthClamp && depthClampEnabled_;
	raster.rasterizerDiscardEnable = VK_FALSE;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = state.cullMode;
	raster.frontFace = state.frontFace;
	raster.depthBiasEnable = state.depthBias ? VK_TRUE : VK_FALSE;
	raster.depthBiasConstantFactor = state.biasConstant;
	raster.depthBiasClamp = depthBiasClampEnabled_ ? state.biasClamp : 0.0f;
	raster.depthBiasSlopeFactor = state.biasSlope;
	raster.lineWidth = 1.0f;
	VkPipelineMultisampleStateCreateInfo multisample {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample.sampleShadingEnable = VK_FALSE;
	multisample.minSampleShading = 1.0f;
	VkPipelineDepthStencilStateCreateInfo depthStencil {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = pass.hasDepth && state.depthTest;
	depthStencil.depthWriteEnable = pass.hasDepth && state.depthWrite;
	depthStencil.depthCompareOp = state.depthCompare;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = pass.hasStencil && state.stencilTest;
	depthStencil.front = state.front;
	depthStencil.back = state.back;
	std::vector<VkPipelineColorBlendAttachmentState> blends(pass.colorFormats.size());
	for (VkPipelineColorBlendAttachmentState& blend : blends) {
		blend.blendEnable = state.blendEnabled ? VK_TRUE : VK_FALSE;
		blend.srcColorBlendFactor = state.srcColor;
		blend.dstColorBlendFactor = state.dstColor;
		blend.colorBlendOp = state.colorOp;
		blend.srcAlphaBlendFactor = state.srcAlpha;
		blend.dstAlphaBlendFactor = state.dstAlpha;
		blend.alphaBlendOp = state.alphaOp;
		blend.colorWriteMask = state.colorMask;
	}
	VkPipelineColorBlendStateCreateInfo blendState {};
	blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendState.logicOpEnable = VK_FALSE;
	blendState.attachmentCount = static_cast<std::uint32_t>(blends.size());
	blendState.pAttachments = blends.empty() ? nullptr : blends.data();
	std::vector<VkDynamicState> dynamicStates { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	if (wideLines_) {
		dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
	}
	VkPipelineDynamicStateCreateInfo dynamic {};
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamicStates.size());
	dynamic.pDynamicStates = dynamicStates.data();
	VkGraphicsPipelineCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	info.stageCount = static_cast<std::uint32_t>(stages.size());
	info.pStages = stages.data();
	info.pVertexInputState = &vertexInput;
	info.pInputAssemblyState = &inputAssembly;
	info.pViewportState = &viewportState;
	info.pRasterizationState = &raster;
	info.pMultisampleState = &multisample;
	info.pDepthStencilState = &depthStencil;
	info.pColorBlendState = &blendState;
	info.pDynamicState = &dynamic;
	info.layout = object.layout;
	info.renderPass = renderPass;
	info.subpass = 0;
	const VkResult result = vkCreateGraphicsPipelines(context_.device(), VK_NULL_HANDLE, 1, &info,
							  nullptr, &pipeline);
	cleanup();
	if (result != VK_SUCCESS) {
		pipeline = VK_NULL_HANDLE;
		return fail(Kld::HalStatusCode::PipelineCreationFailed,
			    "vkCreateGraphicsPipelines failed (" + std::string(resultName(result)) + ")");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createComputeObject(VkPipelineObject& object)
{
	if (object.spirv.size() != 1 || object.spirv.front().empty()) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a compute pipeline needs one SPIR-V shader");
	}
	VkShaderModule module = VK_NULL_HANDLE;
	VkShaderModuleCreateInfo moduleInfo {};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = object.spirv.front().size() * sizeof(std::uint32_t);
	moduleInfo.pCode = object.spirv.front().data();
	if (vkCreateShaderModule(context_.device(), &moduleInfo, nullptr, &module) != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::ShaderCreationFailed, "vkCreateShaderModule failed");
	}
	VkComputePipelineCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	info.stage.module = module;
	info.stage.pName = "main";
	info.layout = object.layout;
	const VkResult result = vkCreateComputePipelines(context_.device(), VK_NULL_HANDLE, 1, &info,
							 nullptr, &object.computePipeline);
	vkDestroyShaderModule(context_.device(), module, nullptr);
	if (result != VK_SUCCESS) {
		return fail(Kld::HalStatusCode::PipelineCreationFailed,
			    "vkCreateComputePipelines failed (" + std::string(resultName(result)) + ")");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createPipeline(const Kld::OpCreatePipeline& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "pipeline handle is zero or already in use");
	}
	if (op.shaderBinaries == nullptr || op.shaderBinaryCount == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a graphics pipeline needs at least one shader binary");
	}
	VkPipelineObject object;
	for (std::uint32_t index = 0; index < op.shaderBinaryCount; ++index) {
		const Kld::ShaderBinaryDescriptor& binary = op.shaderBinaries[index];
		if (binary.spirv == nullptr || binary.wordCount == 0) {
			return fail(Kld::HalStatusCode::InvalidCommand, "a shader binary has no SPIR-V words");
		}
		object.spirv.emplace_back(binary.spirv, binary.spirv + binary.wordCount);
		object.stages.push_back(shaderStage(binary.stage));
	}
	if (op.vertexInput != nullptr) {
		object.stride = op.vertexInput->stride;
		object.attributes.assign(op.vertexInput->descriptors.begin(),
					op.vertexInput->descriptors.end());
	}
	Kld::HalStatus status = createPipelineLayout(object, op.bindingLayout);
	if (!status.ok()) {
		if (object.layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(context_.device(), object.layout, nullptr);
		}
		for (VkDescriptorSetLayout setLayout : object.setLayouts) {
			if (setLayout != VK_NULL_HANDLE) {
				vkDestroyDescriptorSetLayout(context_.device(), setLayout, nullptr);
			}
		}
		return status;
	}
	object.name = op.name != nullptr ? op.name : "";
	pipelines_.emplace(op.id, std::move(object));
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createComputePipeline(const Kld::OpCreateComputePipeline& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "compute pipeline handle is zero or already in use");
	}
	if (op.shader == nullptr || op.shader->spirv == nullptr || op.shader->wordCount == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a compute pipeline needs a SPIR-V shader");
	}
	VkPipelineObject object;
	object.spirv.emplace_back(op.shader->spirv, op.shader->spirv + op.shader->wordCount);
	object.stages.push_back(VK_SHADER_STAGE_COMPUTE_BIT);
	Kld::HalStatus status = createPipelineLayout(object, op.bindingLayout);
	if (!status.ok()) {
		if (object.layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(context_.device(), object.layout, nullptr);
		}
		for (VkDescriptorSetLayout setLayout : object.setLayouts) {
			if (setLayout != VK_NULL_HANDLE) {
				vkDestroyDescriptorSetLayout(context_.device(), setLayout, nullptr);
			}
		}
		return status;
	}
	status = createComputeObject(object);
	if (!status.ok()) {
		vkDestroyPipelineLayout(context_.device(), object.layout, nullptr);
		for (VkDescriptorSetLayout setLayout : object.setLayouts) {
			vkDestroyDescriptorSetLayout(context_.device(), setLayout, nullptr);
		}
		return status;
	}
	object.name = op.name != nullptr ? op.name : "";
	object.compute = true;
	computePipelines_.emplace(op.id, std::move(object));
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyPipeline(const Kld::OpDestroyPipeline& op)
{
	const auto found = pipelines_.find(op.id);
	if (found == pipelines_.end()) {
		return failInvalidHandle("pipeline", op.id);
	}
	if (boundPipelineId_ == op.id) {
		boundPipelineId_ = 0;
	}
	for (auto& byPass : found->second.variants) {
		for (auto& variant : byPass.second) {
			DeferredDestroy entry {};
			entry.pipeline_ = variant.second;
			deferred_.push_back(entry);
		}
	}
	for (const VkDescriptorPool pool : found->second.descriptorPools) {
		DeferredDestroy entry {};
		entry.descriptorPool_ = pool;
		deferred_.push_back(entry);
	}
	DeferredDestroy tail {};
	tail.layout_ = found->second.layout;
	for (VkDescriptorSetLayout setLayout : found->second.setLayouts) {
		DeferredDestroy entry {};
		entry.setLayout_ = setLayout;
		deferred_.push_back(entry);
	}
	deferred_.push_back(tail);
	pipelines_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyComputePipeline(const Kld::OpDestroyComputePipeline& op)
{
	const auto found = computePipelines_.find(op.id);
	if (found == computePipelines_.end()) {
		return failInvalidHandle("compute pipeline", op.id);
	}
	if (boundComputePipelineId_ == op.id) {
		boundComputePipelineId_ = 0;
		computeBound_ = false;
	}
	DeferredDestroy entry {};
	entry.pipeline_ = found->second.computePipeline;
	deferred_.push_back(entry);
	for (const VkDescriptorPool pool : found->second.descriptorPools) {
		DeferredDestroy poolEntry {};
		poolEntry.descriptorPool_ = pool;
		deferred_.push_back(poolEntry);
	}
	DeferredDestroy tail {};
	tail.layout_ = found->second.layout;
	for (VkDescriptorSetLayout setLayout : found->second.setLayouts) {
		DeferredDestroy entry {};
		entry.setLayout_ = setLayout;
		deferred_.push_back(entry);
	}
	deferred_.push_back(tail);
	computePipelines_.erase(found);
	return Kld::HalOk;
}

void CommandExecutor::resetDescriptors() noexcept
{
	const auto reset = [this](VkPipelineObject& object) {
		for (const VkDescriptorPool pool : object.descriptorPools) {
			vkResetDescriptorPool(context_.device(), pool, 0);
		}
		object.descriptorSets.clear();
	};
	for (auto& entry : pipelines_) {
		reset(entry.second);
	}
	for (auto& entry : computePipelines_) {
		reset(entry.second);
	}
}

Kld::HalStatus CommandExecutor::bindPipeline(const Kld::OpBindPipeline& op)
{
	VkPipelineObject* object = pipeline(op.id);
	if (object == nullptr) {
		return failInvalidHandle("pipeline", op.id);
	}
	boundPipelineId_ = op.id;
	computeBound_ = false;
	// The actual vkCmdBindPipeline happens at draw time, when the render pass is
	// known; the descriptor set is (re)written per draw.
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindComputePipeline(const Kld::OpBindComputePipeline& op)
{
	VkPipelineObject* object = computePipeline(op.id);
	if (object == nullptr) {
		return failInvalidHandle("compute pipeline", op.id);
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_COMPUTE, object->computePipeline);
	boundComputePipelineId_ = op.id;
	boundPipelineId_ = 0;
	computeBound_ = true;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindTexture(const Kld::OpBindTexture& op)
{
	if (op.slot >= 64
	    || (capabilities_.maxSampledTextureBindings != 0
		&& op.slot >= capabilities_.maxSampledTextureBindings)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture slot exceeds the probed limit");
	}
	VkTextureObject* object = texture(op.texture);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.texture);
	}
	VkImageViewType expected = VK_IMAGE_VIEW_TYPE_2D;
	switch (op.type) {
		case Kld::TextureBindingType::Texture1D: expected = VK_IMAGE_VIEW_TYPE_1D; break;
		case Kld::TextureBindingType::Texture2D: expected = VK_IMAGE_VIEW_TYPE_2D; break;
		case Kld::TextureBindingType::Texture3D: expected = VK_IMAGE_VIEW_TYPE_3D; break;
		case Kld::TextureBindingType::ArrayTexture2D: expected = VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
		case Kld::TextureBindingType::TextureCube: expected = VK_IMAGE_VIEW_TYPE_CUBE; break;
	}
	if (object->viewType != expected) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "the bound texture's view type does not match the declared binding type");
	}
	boundTextures_[op.slot] = BoundTexture { op.texture, op.type, true };
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindSampler(const Kld::OpBindSampler& op)
{
	if (op.slot >= 64
	    || (capabilities_.maxSamplerBindings != 0
		&& op.slot >= capabilities_.maxSamplerBindings)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "sampler slot exceeds the probed limit");
	}
	if (sampler(op.sampler) == nullptr) {
		return failInvalidHandle("sampler", op.sampler);
	}
	boundSamplers_[op.slot] = op.sampler;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindUniformBuffer(const Kld::OpBindUniformBuffer& op)
{
	if (op.slot >= 32
	    || (capabilities_.maxUniformBufferBindings != 0
		&& op.slot >= capabilities_.maxUniformBufferBindings)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "uniform buffer slot exceeds the probed limit");
	}
	VkBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	const std::uint32_t size = op.size != 0 ? op.size
						: object->size - std::min(op.offset, object->size);
	if (op.offset > object->size || size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the uniform range is outside the buffer");
	}
	boundUniformBuffers_[op.slot] = BoundRange { op.buffer, op.offset, size,
						     Kld::StorageAccess::ReadOnly, true };
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindStorageBuffer(const Kld::OpBindStorageBuffer& op)
{
	if (op.slot >= 32
	    || (capabilities_.maxStorageBufferBindings != 0
		&& op.slot >= capabilities_.maxStorageBufferBindings)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "storage buffer slot exceeds the probed limit");
	}
	VkBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	const std::uint32_t size = op.size != 0 ? op.size
						: object->size - std::min(op.offset, object->size);
	if (op.offset > object->size || size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the storage range is outside the buffer");
	}
	boundStorageBuffers_[op.slot] = BoundRange { op.buffer, op.offset, size, op.access, true };
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindStorageImage(const Kld::OpBindStorageImage& op)
{
	if (op.slot >= 32
	    || (capabilities_.maxStorageImageBindings != 0
		&& op.slot >= capabilities_.maxStorageImageBindings)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "storage image slot exceeds the probed limit");
	}
	VkTextureObject* object = image(op.image);
	if (object == nullptr) {
		return failInvalidHandle("image", op.image);
	}
	if (object->samples != VK_SAMPLE_COUNT_1_BIT) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "a multisampled image cannot be bound as a storage image");
	}
	boundStorageImages_[op.slot] = BoundImage { op.image, op.mipLevel, op.layer, op.access, true };
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindVertexBuffer(const Kld::OpBindVertexBuffer& op)
{
	if (op.binding >= vertexBindings_.size()) {
		return fail(Kld::HalStatusCode::InvalidCommand, "vertex buffer binding index is out of range");
	}
	VkBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (op.offset > object->size) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the vertex offset is outside the buffer");
	}
	if (op.instanceDivisor > 1) {
		// Vulkan 1.0 bakes the input rate into the pipeline; a divisor is only
		// expressible through VK_EXT_vertex_attribute_divisor.
		return failUnsupported("Vulkan 1.0 cannot express a vertex instance divisor above 1");
	}
	VertexBinding& state = vertexBindings_[op.binding];
	state.buffer = op.buffer;
	state.offset = op.offset;
	state.stride = op.stride;
	state.divisor = op.instanceDivisor;
	state.valid = true;
	const VkDeviceSize offset = op.offset;
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	vkCmdBindVertexBuffers(commandBuffer_, op.binding, 1, &object->buffer, &offset);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindIndexBuffer(const Kld::OpBindIndexBuffer& op)
{
	VkBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (op.offset > object->size) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the index offset is outside the buffer");
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	indexBinding_.buffer = op.buffer;
	indexBinding_.offset = op.offset;
	indexBinding_.type = op.type == Kld::IndexType::Uint32 ? VK_INDEX_TYPE_UINT32
							      : VK_INDEX_TYPE_UINT16;
	indexBinding_.valid = true;
	vkCmdBindIndexBuffer(commandBuffer_, object->buffer, op.offset, indexBinding_.type);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::updateDescriptors(VkPipelineObject& object)
{
	bool anySlots = false;
	for (const std::vector<VkDescriptorSlot>& slots : object.slotsByKind) {
		anySlots = anySlots || !slots.empty();
	}
	if (!anySlots) {
		return Kld::HalOk;
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	const auto allocateSet = [this, &object](std::size_t kind) -> VkDescriptorSet {
		const auto createPool = [this, &object]() -> bool {
			VkDescriptorPoolCreateInfo poolInfo {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.maxSets = 256;
			poolInfo.poolSizeCount = static_cast<std::uint32_t>(object.poolSizes.size());
			poolInfo.pPoolSizes = object.poolSizes.data();
			VkDescriptorPool pool = VK_NULL_HANDLE;
			if (vkCreateDescriptorPool(context_.device(), &poolInfo, nullptr, &pool)
			    != VK_SUCCESS) {
				return false;
			}
			object.descriptorPools.push_back(pool);
			return true;
		};
		for (;;) {
			if (object.descriptorPools.empty() && !createPool()) {
				return VK_NULL_HANDLE;
			}
			VkDescriptorSetAllocateInfo allocate {};
			allocate.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocate.descriptorPool = object.descriptorPools.back();
			allocate.descriptorSetCount = 1;
			allocate.pSetLayouts = &object.setLayouts[kind];
			VkDescriptorSet set = VK_NULL_HANDLE;
			if (vkAllocateDescriptorSets(context_.device(), &allocate, &set) == VK_SUCCESS) {
				return set;
			}
			if (!createPool()) {
				return VK_NULL_HANDLE;
			}
		}
	};
	for (std::size_t kind = 0; kind < object.slotsByKind.size(); ++kind) {
		const std::vector<VkDescriptorSlot>& slots = object.slotsByKind[kind];
		if (slots.empty()) {
			continue;
		}
		const VkDescriptorSet set = allocateSet(kind);
		if (set == VK_NULL_HANDLE) {
			return fail(Kld::HalStatusCode::AllocationFailed, "vkCreateDescriptorPool failed");
		}
		object.descriptorSets.push_back(set);
		std::vector<VkDescriptorImageInfo> images;
		std::vector<VkDescriptorBufferInfo> ranges;
		std::vector<VkWriteDescriptorSet> writes;
		std::uint32_t imageDescriptorCount = 0;
		for (const VkDescriptorSlot& slot : slots) {
			if (kind != 2 && kind != 3) {
				imageDescriptorCount += slot.count;
			}
		}
		images.reserve(imageDescriptorCount);
		ranges.reserve(slots.size());
		writes.reserve(slots.size());
		for (const VkDescriptorSlot& slot : slots) {
			VkWriteDescriptorSet write {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = set;
			write.dstBinding = slot.binding;
			write.descriptorType = slot.type;
			write.descriptorCount = slot.count;
			if (kind == 2 || kind == 3) {
				const BoundRange& state = kind == 2
					? boundUniformBuffers_[std::min<std::uint32_t>(slot.binding, 31)]
					: boundStorageBuffers_[std::min<std::uint32_t>(slot.binding, 31)];
				VkBuffer handle = dummyBuffer_.buffer;
				VkDeviceSize offset = 0;
				VkDeviceSize range = dummyBuffer_.size == 0 ? VK_WHOLE_SIZE
									    : dummyBuffer_.size;
				if (state.valid) {
					if (VkBufferObject* bound = buffer(state.buffer)) {
						handle = bound->buffer;
						offset = state.offset;
						range = state.size != 0 ? state.size : VK_WHOLE_SIZE;
					}
				}
				ranges.push_back(VkDescriptorBufferInfo { handle, offset, range });
				write.pBufferInfo = &ranges.back();
			} else {
				const std::size_t base = images.size();
				for (std::uint32_t index = 0; index < slot.count; ++index) {
					const std::uint32_t slotIndex = slot.binding + index;
					VkDescriptorImageInfo info {};
					info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					if (kind == 1) {
						info.sampler = defaultSampler_;
						if (slotIndex < 64 && boundSamplers_[slotIndex] != 0) {
							if (VkSamplerObject* bound =
								sampler(boundSamplers_[slotIndex])) {
								info.sampler = bound->sampler;
							}
						}
					} else {
						info.imageView = dummyTexture_.view;
						if (kind == 0 && slotIndex < 64
						    && boundTextures_[slotIndex].valid) {
							if (VkTextureObject* bound = texture(
								boundTextures_[slotIndex].texture)) {
								info.imageView = bound->view;
							}
						}
						if (kind == 4 && slotIndex < 32
						    && boundStorageImages_[slotIndex].valid) {
							if (VkTextureObject* bound = image(
								boundStorageImages_[slotIndex].image)) {
								info.imageView = bound->view;
							}
						}
					}
					images.push_back(info);
				}
				write.pImageInfo = images.data() + base;
			}
			writes.push_back(write);
		}
		vkUpdateDescriptorSets(context_.device(), static_cast<std::uint32_t>(writes.size()),
				       writes.data(), 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer_,
					object.compute ? VK_PIPELINE_BIND_POINT_COMPUTE
						       : VK_PIPELINE_BIND_POINT_GRAPHICS,
					object.layout, static_cast<std::uint32_t>(kind), 1, &set, 0, nullptr);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindVertexInputs()
{
	std::size_t highest = 0;
	for (std::size_t binding = 0; binding < vertexBindings_.size(); ++binding) {
		if (vertexBindings_[binding].valid) {
			highest = binding + 1;
		}
	}
	if (highest == 0) {
		return Kld::HalOk;
	}
	std::vector<VkBuffer> buffers(highest, dummyBuffer_.buffer);
	std::vector<VkDeviceSize> offsets(highest, 0);
	for (std::size_t binding = 0; binding < highest; ++binding) {
		if (!vertexBindings_[binding].valid) {
			continue;
		}
		if (VkBufferObject* object = buffer(vertexBindings_[binding].buffer)) {
			buffers[binding] = object->buffer;
			offsets[binding] = vertexBindings_[binding].offset;
		}
	}
	Kld::HalStatus status = ensureRecording();
	if (!status.ok()) {
		return status;
	}
	vkCmdBindVertexBuffers(commandBuffer_, 0, static_cast<std::uint32_t>(highest), buffers.data(),
			       offsets.data());
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::prepareDraw(VkPipelineObject*& object, bool lines)
{
	VkPipelineObject* pipelineObject = pipeline(boundPipelineId_);
	if (pipelineObject == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no graphics pipeline is bound");
	}
	object = pipelineObject;
	Kld::HalStatus status = ensureRenderPass(boundFramebuffer_);
	if (!status.ok()) {
		return status;
	}
	// Every declared slot needs a valid descriptor: Vulkan reads descriptors when
	// the draw executes, so an unwritten one is undefined behaviour rather than a
	// silently ignored binding.
	for (std::size_t kind = 0; kind < object->slotsByKind.size(); ++kind) {
		for (const VkDescriptorSlot& slot : object->slotsByKind[kind]) {
			for (std::uint32_t index = 0; index < slot.count; ++index) {
				const std::uint32_t slotIndex = slot.binding + index;
				bool bound = true;
				switch (kind) {
					case 0:
						bound = slotIndex < 64
							&& boundTextures_[slotIndex].valid;
						break;
					case 2:
						bound = slotIndex < 32
							&& boundUniformBuffers_[slotIndex].valid;
						break;
					case 3:
						bound = slotIndex < 32
							&& boundStorageBuffers_[slotIndex].valid;
						break;
					case 4:
						bound = slotIndex < 32
							&& boundStorageImages_[slotIndex].valid;
						break;
					default:
						// Samplers fall back to the executor's default sampler,
						// which mirrors GL's "no sampler object" behaviour.
						bound = true;
						break;
				}
				if (!bound) {
					return fail(Kld::HalStatusCode::InvalidResourceState,
						    "the pipeline declares binding "
							    + std::to_string(slot.binding)
							    + ", but nothing is bound to it");
				}
			}
		}
	}
	VkPipelineState state = currentPipelineState(lines);
	const VkPassInfo* pass = currentPassInfo();
	state.hasDepth = pass != nullptr && pass->hasDepth;
	VkPipeline& variant = object->variants[activePass_][state];
	if (variant == VK_NULL_HANDLE) {
		status = createGraphicsVariant(*object, activePass_, state, variant);
		if (!status.ok()) {
			return status;
		}
	}
	vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, variant);
	status = bindVertexInputs();
	if (!status.ok()) {
		return status;
	}
	return updateDescriptors(*object);
}

// ─────────────────────────────────────────────────────────────────────────────
// Draws
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/** @brief Byte size of one index of @p type. */
std::uint32_t indexByteSize(VkIndexType type) noexcept
{
	return type == VK_INDEX_TYPE_UINT32 ? 4u : 2u;
}

} // namespace

Kld::HalStatus CommandExecutor::drawTriangles(const Kld::OpDrawTriangles& op)
{
	VkPipelineObject* object = nullptr;
	const Kld::HalStatus status = prepareDraw(object, false);
	if (!status.ok()) {
		return status;
	}
	if (op.vertexCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDraw(commandBuffer_, op.vertexCount, 1, op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawIndexedTriangles(const Kld::OpDrawIndexedTriangles& op)
{
	VkPipelineObject* object = nullptr;
	const Kld::HalStatus status = prepareDraw(object, false);
	if (!status.ok()) {
		return status;
	}
	if (!indexBinding_.valid) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "an indexed draw needs a bound index buffer");
	}
	const std::uint32_t size = indexByteSize(indexBinding_.type);
	if ((op.indexOffset % size) != 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the index offset is not a multiple of the index size");
	}
	if (op.indexCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDrawIndexed(commandBuffer_, op.indexCount, 1, op.indexOffset / size, op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawLines(const Kld::OpDrawLines& op)
{
	VkPipelineObject* object = nullptr;
	Kld::HalStatus status = prepareDraw(object, true);
	if (!status.ok()) {
		return status;
	}
	if (wideLines_ && op.thickness > 1.0f) {
		vkCmdSetLineWidth(commandBuffer_,
				  std::min(op.thickness, context_.properties().limits.lineWidthRange[1]));
	}
	if (op.vertexCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDraw(commandBuffer_, op.vertexCount, 1, op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawIndexedLines(const Kld::OpDrawIndexedLines& op)
{
	VkPipelineObject* object = nullptr;
	Kld::HalStatus status = prepareDraw(object, true);
	if (!status.ok()) {
		return status;
	}
	if (!indexBinding_.valid) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "an indexed draw needs a bound index buffer");
	}
	const std::uint32_t size = indexByteSize(indexBinding_.type);
	if ((op.indexOffset % size) != 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the index offset is not a multiple of the index size");
	}
	if (wideLines_ && op.thickness > 1.0f) {
		vkCmdSetLineWidth(commandBuffer_,
				  std::min(op.thickness, context_.properties().limits.lineWidthRange[1]));
	}
	if (op.indexCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDrawIndexed(commandBuffer_, op.indexCount, 1, op.indexOffset / size, op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawTrianglesInstanced(const Kld::OpDrawTrianglesInstanced& op)
{
	VkPipelineObject* object = nullptr;
	const Kld::HalStatus status = prepareDraw(object, false);
	if (!status.ok()) {
		return status;
	}
	if (op.firstInstance != 0) {
		return failUnsupported("Vulkan 1.0 has no base-instance draw parameter");
	}
	if (op.vertexCount == 0 || op.instanceCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDraw(commandBuffer_, op.vertexCount, op.instanceCount, op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawIndexedTrianglesInstanced(
	const Kld::OpDrawIndexedTrianglesInstanced& op)
{
	VkPipelineObject* object = nullptr;
	const Kld::HalStatus status = prepareDraw(object, false);
	if (!status.ok()) {
		return status;
	}
	if (op.firstInstance != 0) {
		return failUnsupported("Vulkan 1.0 has no base-instance draw parameter");
	}
	if (!indexBinding_.valid) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "an indexed draw needs a bound index buffer");
	}
	const std::uint32_t size = indexByteSize(indexBinding_.type);
	if ((op.indexOffset % size) != 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the index offset is not a multiple of the index size");
	}
	if (op.indexCount == 0 || op.instanceCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDrawIndexed(commandBuffer_, op.indexCount, op.instanceCount, op.indexOffset / size,
			 op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawLinesInstanced(const Kld::OpDrawLinesInstanced& op)
{
	VkPipelineObject* object = nullptr;
	Kld::HalStatus status = prepareDraw(object, true);
	if (!status.ok()) {
		return status;
	}
	if (op.firstInstance != 0) {
		return failUnsupported("Vulkan 1.0 has no base-instance draw parameter");
	}
	if (wideLines_ && op.thickness > 1.0f) {
		vkCmdSetLineWidth(commandBuffer_,
				  std::min(op.thickness, context_.properties().limits.lineWidthRange[1]));
	}
	if (op.vertexCount == 0 || op.instanceCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDraw(commandBuffer_, op.vertexCount, op.instanceCount, op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawIndexedLinesInstanced(
	const Kld::OpDrawIndexedLinesInstanced& op)
{
	VkPipelineObject* object = nullptr;
	Kld::HalStatus status = prepareDraw(object, true);
	if (!status.ok()) {
		return status;
	}
	if (op.firstInstance != 0) {
		return failUnsupported("Vulkan 1.0 has no base-instance draw parameter");
	}
	if (!indexBinding_.valid) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "an indexed draw needs a bound index buffer");
	}
	const std::uint32_t size = indexByteSize(indexBinding_.type);
	if ((op.indexOffset % size) != 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the index offset is not a multiple of the index size");
	}
	if (wideLines_ && op.thickness > 1.0f) {
		vkCmdSetLineWidth(commandBuffer_,
				  std::min(op.thickness, context_.properties().limits.lineWidthRange[1]));
	}
	if (op.indexCount == 0 || op.instanceCount == 0) {
		return Kld::HalOk;
	}
	vkCmdDrawIndexed(commandBuffer_, op.indexCount, op.instanceCount, op.indexOffset / size,
			 op.vertexOffset, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawIndirect(const Kld::OpDrawIndirect& op)
{
	VkPipelineObject* object = nullptr;
	const Kld::HalStatus status = prepareDraw(object, false);
	if (!status.ok()) {
		return status;
	}
	VkBufferObject* indirect = buffer(op.buffer);
	if (indirect == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (op.drawCount == 0) {
		return Kld::HalOk;
	}
	if ((op.offset % 4) != 0 || (op.stride != 0 && (op.stride % 4) != 0)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the indirect offset and stride must be multiples of four");
	}
	vkCmdDrawIndirect(commandBuffer_, indirect->buffer, op.offset, op.drawCount, op.stride);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawIndexedIndirect(const Kld::OpDrawIndexedIndirect& op)
{
	VkPipelineObject* object = nullptr;
	const Kld::HalStatus status = prepareDraw(object, false);
	if (!status.ok()) {
		return status;
	}
	VkBufferObject* indirect = buffer(op.buffer);
	if (indirect == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (!indexBinding_.valid) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "an indexed indirect draw needs a bound index buffer");
	}
	if (op.drawCount == 0) {
		return Kld::HalOk;
	}
	if ((op.offset % 4) != 0 || (op.stride != 0 && (op.stride % 4) != 0)) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the indirect offset and stride must be multiples of four");
	}
	vkCmdDrawIndexedIndirect(commandBuffer_, indirect->buffer, op.offset, op.drawCount, op.stride);
	return Kld::HalOk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Compute
// ─────────────────────────────────────────────────────────────────────────────

Kld::HalStatus CommandExecutor::dispatchCompute(const Kld::OpDispatchCompute& op)
{
	if (!computeBound_) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no compute pipeline is bound");
	}
	VkPipelineObject* object = computePipeline(boundComputePipelineId_);
	if (object == nullptr) {
		return failInvalidHandle("compute pipeline", boundComputePipelineId_);
	}
	// A dispatch cannot be recorded inside a render pass.
	endActiveRenderPass();
	Kld::HalStatus status = updateDescriptors(*object);
	if (!status.ok()) {
		return status;
	}
	if (op.groupCountX == 0 || op.groupCountY == 0 || op.groupCountZ == 0) {
		return Kld::HalOk;
	}
	vkCmdDispatch(commandBuffer_, op.groupCountX, op.groupCountY, op.groupCountZ);
	VkMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	// Compute results are visible to whatever follows in the same submission.
	vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0,
			     nullptr);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::dispatchComputeIndirect(const Kld::OpDispatchComputeIndirect& op)
{
	if (!computeBound_) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no compute pipeline is bound");
	}
	VkPipelineObject* object = computePipeline(boundComputePipelineId_);
	if (object == nullptr) {
		return failInvalidHandle("compute pipeline", boundComputePipelineId_);
	}
	VkBufferObject* indirect = buffer(op.buffer);
	if (indirect == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if ((op.offset % 4) != 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "the indirect offset must be a multiple of four");
	}
	endActiveRenderPass();
	Kld::HalStatus status = updateDescriptors(*object);
	if (!status.ok()) {
		return status;
	}
	vkCmdDispatchIndirect(commandBuffer_, indirect->buffer, op.offset);
	VkMemoryBarrier barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0,
			     nullptr);
	return Kld::HalOk;
}

}
