/**
 * @file GlExecutorPipelines.cpp
 * @brief Pipeline creation, resource binding and draw handlers for the GL executor.
 */
#include "GlCommandExecutor.hpp"
#include "GlExecutorMapping.hpp"
#include "GlSpirvShader.hpp"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace KALDIGL_NS {

namespace {

/** @brief Releases a GL program and VAO of a pipeline object. */
void releasePipelineObjects(GlPipelineObject& object) noexcept
{
	if (object.vao != 0) {
		glDeleteVertexArrays(1, &object.vao);
		object.vao = 0;
	}
	if (object.program != 0) {
		glDeleteProgram(object.program);
		object.program = 0;
	}
}

/** @brief Deep-copies the attribute part of a vertex descriptor (drops names). */
std::vector<Kld::AttributeDescriptor> copyAttributes(const Kld::VertexDescriptor& descriptor)
{
	std::vector<Kld::AttributeDescriptor> attributes;
	attributes.reserve(descriptor.descriptors.size());
	for (const Kld::AttributeDescriptor& entry : descriptor.descriptors) {
		Kld::AttributeDescriptor attribute = entry;
		attribute.SemanticName = {};
		attributes.push_back(attribute);
	}
	return attributes;
}

/** @brief Reads the shader/program info log into a string. */
std::string shaderInfoLog(GLuint shader)
{
	GLint length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
	if (length <= 0) {
		return {};
	}
	std::vector<char> text(static_cast<std::size_t>(length));
	glGetShaderInfoLog(shader, length, nullptr, text.data());
	return std::string(text.data());
}

/** @brief Reads the program info log into a string. */
std::string programInfoLog(GLuint program)
{
	GLint length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
	if (length <= 0) {
		return {};
	}
	std::vector<char> text(static_cast<std::size_t>(length));
	glGetProgramInfoLog(program, length, nullptr, text.data());
	return std::string(text.data());
}

} // namespace

Kld::HalStatus CommandExecutor::linkProgram(const Kld::ShaderBinaryDescriptor* binaries,
					    std::uint32_t count, GLuint& program)
{
	if (binaries == nullptr || count == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a pipeline needs at least one shader binary");
	}
	program = glCreateProgram();
	if (program == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glCreateProgram returned no name");
	}
	std::vector<GLuint> shaders;
	const auto cleanup = [&] {
		for (const GLuint shader : shaders) {
			glDetachShader(program, shader);
			glDeleteShader(shader);
		}
		glDeleteProgram(program);
		program = 0;
	};
	for (std::uint32_t index = 0; index < count; ++index) {
		const Kld::ShaderBinaryDescriptor& binary = binaries[index];
		if (binary.spirv == nullptr || binary.wordCount == 0) {
			cleanup();
			return fail(Kld::HalStatusCode::InvalidCommand, "a shader binary has no SPIR-V words");
		}
		std::string source;
		try {
			source = spirvToGlsl(binary);
		} catch (const std::exception& error) {
			cleanup();
			return fail(Kld::HalStatusCode::ShaderCreationFailed, error.what());
		}
		const GLuint shader = glCreateShader(detail::shaderStage(binary.stage));
		const char* text = source.c_str();
		glShaderSource(shader, 1, &text, nullptr);
		glCompileShader(shader);
		GLint compiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled == GL_FALSE) {
			const std::string log = shaderInfoLog(shader);
			glDeleteShader(shader);
			cleanup();
			return fail(Kld::HalStatusCode::ShaderCreationFailed,
				    log.empty() ? "glCompileShader failed" : log);
		}
		glAttachShader(program, shader);
		shaders.push_back(shader);
	}
	glLinkProgram(program);
	GLint linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE) {
		const std::string log = programInfoLog(program);
		cleanup();
		return fail(Kld::HalStatusCode::PipelineCreationFailed,
			    log.empty() ? "glLinkProgram failed" : log);
	}
	for (const GLuint shader : shaders) {
		glDetachShader(program, shader);
		glDeleteShader(shader);
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::makeVertexArray(GlPipelineObject& object,
						const Kld::VertexDescriptor* descriptor)
{
	if (descriptor == nullptr || descriptor->descriptors.empty()) {
		return Kld::HalOk; // A pipeline without vertex input (fullscreen pass, compute-like).
	}
	object.stride = descriptor->stride;
	object.attributes = copyAttributes(*descriptor);
	glGenVertexArrays(1, &object.vao);
	if (object.vao == 0) {
		return fail(Kld::HalStatusCode::AllocationFailed, "glGenVertexArrays returned no name");
	}
	glBindVertexArray(object.vao);
	const Kld::VertexDescriptor applied {
		object.stride, std::span<const Kld::AttributeDescriptor>(object.attributes.data(),
									 object.attributes.size())
	};
	const bool complete = applyVertexDescriptor(applied, 0);
	glBindVertexArray(currentVertexArray_ != 0 ? currentVertexArray_ : defaultVertexArray_);
	if (!complete) {
		releasePipelineObjects(object);
		return failUnsupported("the vertex descriptor uses an attribute format this dialect cannot express");
	}
	object.hasVertexInput = true;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::buildGraphicsPipeline(GlPipelineObject& object,
						      const Kld::OpCreatePipeline& op)
{
	if (op.shaderBinaries == nullptr || op.shaderBinaryCount == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand,
			    "a graphics pipeline needs at least one shader binary");
	}
	Kld::HalStatus status = linkProgram(op.shaderBinaries, op.shaderBinaryCount, object.program);
	if (!status.ok()) {
		return status;
	}
	status = makeVertexArray(object, op.vertexInput);
	if (!status.ok()) {
		releasePipelineObjects(object);
		return status;
	}
	// The GLSL emitted by GlSpirvShader carries explicit layout(binding = N)
	// decorations, so the binding layout is advisory here - but a layout that
	// cannot fit the probed limits is still worth rejecting early.
	const Kld::PipelineBindingLayout* layout = op.bindingLayout;
	const auto exceeds = [](std::uint32_t slot, std::uint32_t count, std::uint32_t limit) {
		return limit != 0 && slot + count > limit;
	};
	if (layout != nullptr) {
		for (std::uint32_t index = 0; index < layout->sampledTextureCount; ++index) {
			const Kld::SampledTextureBindingLayout& entry = layout->sampledTextures[index];
			if (exceeds(entry.slot, entry.count, capabilities_.maxSampledTextureBindings)) {
				releasePipelineObjects(object);
				return failUnsupported("the pipeline's sampled texture layout exceeds the limit");
			}
		}
		for (std::uint32_t index = 0; index < layout->samplerCount; ++index) {
			const Kld::SamplerBindingLayout& entry = layout->samplers[index];
			if (exceeds(entry.slot, entry.count, capabilities_.maxSamplerBindings)) {
				releasePipelineObjects(object);
				return failUnsupported("the pipeline's sampler layout exceeds the limit");
			}
		}
		for (std::uint32_t index = 0; index < layout->uniformBufferCount; ++index) {
			const Kld::UniformBufferBindingLayout& entry = layout->uniformBuffers[index];
			if (exceeds(entry.slot, 1, capabilities_.maxUniformBufferBindings)) {
				releasePipelineObjects(object);
				return failUnsupported("the pipeline's uniform buffer layout exceeds the limit");
			}
		}
		for (std::uint32_t index = 0; index < layout->storageBufferCount; ++index) {
			const Kld::StorageBufferBindingLayout& entry = layout->storageBuffers[index];
			if (exceeds(entry.slot, 1, capabilities_.maxStorageBufferBindings)) {
				releasePipelineObjects(object);
				return failUnsupported("the pipeline's storage buffer layout exceeds the limit");
			}
		}
		for (std::uint32_t index = 0; index < layout->storageImageCount; ++index) {
			const Kld::StorageImageBindingLayout& entry = layout->storageImages[index];
			if (exceeds(entry.slot, 1, capabilities_.maxStorageImageBindings)) {
				releasePipelineObjects(object);
				return failUnsupported("the pipeline's storage image layout exceeds the limit");
			}
		}
	}
	object.name = op.name != nullptr ? op.name : "";
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::buildComputePipeline(GlPipelineObject& object,
						     const Kld::OpCreateComputePipeline& op)
{
	if (op.shader == nullptr || op.shader->spirv == nullptr || op.shader->wordCount == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a compute pipeline needs a SPIR-V shader");
	}
	const Kld::HalStatus status = linkProgram(op.shader, 1, object.program);
	if (!status.ok()) {
		return status;
	}
	object.name = op.name != nullptr ? op.name : "";
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createPipeline(const Kld::OpCreatePipeline& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "pipeline handle is zero or already in use");
	}
	GlPipelineObject object;
	const Kld::HalStatus status = buildGraphicsPipeline(object, op);
	if (!status.ok()) {
		return status;
	}
	if (debugLabelsEnabled_ && glObjectLabel != nullptr && !object.name.empty()) {
		glObjectLabel(GL_PROGRAM, object.program, -1, object.name.c_str());
	}
	pipelines_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createComputePipeline(const Kld::OpCreateComputePipeline& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "pipeline handle is zero or already in use");
	}
	GlPipelineObject object;
	const Kld::HalStatus status = buildComputePipeline(object, op);
	if (!status.ok()) {
		return status;
	}
	if (debugLabelsEnabled_ && glObjectLabel != nullptr && !object.name.empty()) {
		glObjectLabel(GL_PROGRAM, object.program, -1, object.name.c_str());
	}
	computePipelines_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyPipeline(const Kld::OpDestroyPipeline& op)
{
	const auto found = pipelines_.find(op.id);
	if (found == pipelines_.end()) {
		return failInvalidHandle("pipeline", op.id);
	}
	if (boundPipeline_ == op.id) {
		glUseProgram(0);
		glBindVertexArray(defaultVertexArray_);
		boundPipeline_ = 0;
		currentProgram_ = 0;
		currentVertexArray_ = defaultVertexArray_;
	}
	releasePipelineObjects(found->second);
	pipelines_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyComputePipeline(const Kld::OpDestroyComputePipeline& op)
{
	const auto found = computePipelines_.find(op.id);
	if (found == computePipelines_.end()) {
		return failInvalidHandle("compute pipeline", op.id);
	}
	if (currentProgram_ == found->second.program) {
		glUseProgram(0);
		currentProgram_ = 0;
		computePipelineActive_ = false;
	}
	releasePipelineObjects(found->second);
	computePipelines_.erase(found);
	return Kld::HalOk;
}

void CommandExecutor::reapplyVertexState()
{
	for (std::size_t binding = 0; binding < vertexBindings_.size(); ++binding) {
		const VertexBindingState& state = vertexBindings_[binding];
		if (!state.valid) {
			continue;
		}
		GlBufferObject* object = buffer(state.buffer);
		if (object == nullptr) {
			continue;
		}
		glBindVertexBuffer(static_cast<GLuint>(binding), object->name, state.offset, state.stride);
		if (glVertexBindingDivisor != nullptr) {
			glVertexBindingDivisor(static_cast<GLuint>(binding), state.divisor);
		}
	}
	if (indexBinding_.valid) {
		GlBufferObject* object = buffer(indexBinding_.buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object != nullptr ? object->name : 0);
	}
}

Kld::HalStatus CommandExecutor::bindPipeline(const Kld::OpBindPipeline& op)
{
	GlPipelineObject* object = pipeline(op.id);
	if (object == nullptr) {
		return failInvalidHandle("pipeline", op.id);
	}
	glUseProgram(object->program);
	glBindVertexArray(object->vao != 0 ? object->vao : defaultVertexArray_);
	currentProgram_ = object->program;
	currentVertexArray_ = object->vao != 0 ? object->vao : defaultVertexArray_;
	boundPipeline_ = op.id;
	computePipelineActive_ = false;
	// Vertex and index buffer bindings are VAO state, so a pipeline switch has to
	// reapply what the stream recorded.
	reapplyVertexState();
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindComputePipeline(const Kld::OpBindComputePipeline& op)
{
	GlPipelineObject* object = computePipeline(op.id);
	if (object == nullptr) {
		return failInvalidHandle("compute pipeline", op.id);
	}
	glUseProgram(object->program);
	currentProgram_ = object->program;
	boundPipeline_ = 0;
	computePipelineActive_ = true;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindTexture(const Kld::OpBindTexture& op)
{
	if (maxTextureUnits_ > 0 && op.slot >= static_cast<std::uint32_t>(maxTextureUnits_)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "texture unit exceeds the probed limit");
	}
	GlTextureObject* object = texture(op.texture);
	if (object == nullptr) {
		return failInvalidHandle("texture", op.texture);
	}
	const GLenum target = detail::textureTarget(op.type);
	if (target == 0) {
		return failUnsupported("OpenGL ES has no 1D texture targets");
	}
	if (object->target != target) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "the bound texture's target does not match the declared binding type");
	}
	glActiveTexture(GL_TEXTURE0 + op.slot);
	glBindTexture(target, object->name);
	glActiveTexture(GL_TEXTURE0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindSampler(const Kld::OpBindSampler& op)
{
	if (capabilities_.maxSamplerBindings != 0 && op.slot >= capabilities_.maxSamplerBindings) {
		return fail(Kld::HalStatusCode::InvalidCommand, "sampler slot exceeds the probed limit");
	}
	GlSamplerObject* object = sampler(op.sampler);
	if (object == nullptr) {
		return failInvalidHandle("sampler", op.sampler);
	}
	glBindSampler(op.slot, object->name);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindUniformBuffer(const Kld::OpBindUniformBuffer& op)
{
	if (capabilities_.maxUniformBufferBindings != 0
	    && op.slot >= capabilities_.maxUniformBufferBindings) {
		return fail(Kld::HalStatusCode::InvalidCommand, "uniform buffer slot exceeds the probed limit");
	}
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	const std::uint32_t size = op.size != 0 ? op.size : object->size - std::min(op.offset, object->size);
	if (op.offset > object->size || size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the uniform range is outside the buffer");
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, op.slot, object->name, op.offset, size);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindStorageBuffer(const Kld::OpBindStorageBuffer& op)
{
	if (capabilities_.maxStorageBufferBindings != 0
	    && op.slot >= capabilities_.maxStorageBufferBindings) {
		return fail(Kld::HalStatusCode::InvalidCommand, "storage buffer slot exceeds the probed limit");
	}
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	const std::uint32_t size = op.size != 0 ? op.size : object->size - std::min(op.offset, object->size);
	if (op.offset > object->size || size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the storage range is outside the buffer");
	}
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, op.slot, object->name, op.offset, size);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindStorageImage(const Kld::OpBindStorageImage& op)
{
	if (maxImageUnits_ > 0 && op.slot >= static_cast<std::uint32_t>(maxImageUnits_)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "image unit exceeds the probed limit");
	}
	GlTextureObject* object = image(op.image);
	if (object == nullptr) {
		return failInvalidHandle("image", op.image);
	}
	if (object->multisampled) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "a multisampled image cannot be bound as a storage image");
	}
	// Kaldi has no "layer is an array slice vs. whole layer" flag; single-layer
	// binding is the conservative interpretation.
	glBindImageTexture(op.slot, object->name, op.mipLevel, GL_FALSE, op.layer,
			   detail::storageAccess(op.access), object->internalFormat);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindVertexBuffer(const Kld::OpBindVertexBuffer& op)
{
	if (op.binding >= vertexBindings_.size()) {
		return fail(Kld::HalStatusCode::InvalidCommand, "vertex buffer binding index is out of range");
	}
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	VertexBindingState& state = vertexBindings_[op.binding];
	state.buffer = op.buffer;
	state.offset = op.offset;
	state.stride = op.stride;
	state.divisor = op.instanceDivisor;
	state.valid = true;
	if (currentVertexArray_ != 0) {
		glBindVertexBuffer(op.binding, object->name, op.offset, op.stride);
		if (glVertexBindingDivisor != nullptr) {
			glVertexBindingDivisor(op.binding, op.instanceDivisor);
		}
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::bindIndexBuffer(const Kld::OpBindIndexBuffer& op)
{
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	indexBinding_.buffer = op.buffer;
	indexBinding_.offset = op.offset;
	indexBinding_.type = op.type;
	indexBinding_.valid = true;
	if (currentVertexArray_ != 0) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object->name);
	}
	return Kld::HalOk;
}

namespace {

/** @brief Applies a Kaldi line thickness; GL/GLES may clamp it to 1.0. */
void applyLineWidth(float thickness) noexcept
{
	if (thickness > 0.0f) {
		glLineWidth(thickness);
	}
}

/** @brief True when a graphics draw can execute with the tracked state. */
bool graphicsPipelineBound(GLuint program, bool computePipelineActive) noexcept
{
	return program != 0 && !computePipelineActive;
}

} // namespace

Kld::HalStatus CommandExecutor::drawArrays(GLenum mode, std::uint32_t first, std::uint32_t count,
					   std::uint32_t instances, std::uint32_t baseInstance)
{
	if (!graphicsPipelineBound(currentProgram_, computePipelineActive_)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no graphics pipeline is bound");
	}
	if (count == 0 || instances == 0) {
		return Kld::HalOk;
	}
#if KALDIGL_IS_GLES
	if (baseInstance != 0) {
		return failUnsupported("OpenGL ES has no base-instance draws");
	}
	if (instances == 1) {
		glDrawArrays(mode, static_cast<GLint>(first), static_cast<GLsizei>(count));
	} else {
		glDrawArraysInstanced(mode, static_cast<GLint>(first), static_cast<GLsizei>(count),
				      static_cast<GLsizei>(instances));
	}
#else
	if (instances == 1 && baseInstance == 0) {
		glDrawArrays(mode, static_cast<GLint>(first), static_cast<GLsizei>(count));
	} else if (baseInstance == 0) {
		glDrawArraysInstanced(mode, static_cast<GLint>(first), static_cast<GLsizei>(count),
				      static_cast<GLsizei>(instances));
	} else {
		glDrawArraysInstancedBaseInstance(mode, static_cast<GLint>(first),
						  static_cast<GLsizei>(count),
						  static_cast<GLsizei>(instances), baseInstance);
	}
#endif
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawElements(GLenum mode, std::uint32_t indexCount,
					     std::uint32_t byteOffset, std::uint32_t baseVertex,
					     std::uint32_t instances, std::uint32_t baseInstance)
{
	if (!graphicsPipelineBound(currentProgram_, computePipelineActive_)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no graphics pipeline is bound");
	}
	if (!indexBinding_.valid) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "an indexed draw needs a bound index buffer");
	}
	if (indexCount == 0 || instances == 0) {
		return Kld::HalOk;
	}
	const GLenum type = detail::indexType(indexBinding_.type);
	const auto offset = reinterpret_cast<const void*>(
		static_cast<std::uintptr_t>(indexBinding_.offset) + byteOffset);
#if KALDIGL_IS_GLES
	if (baseInstance != 0) {
		return failUnsupported("OpenGL ES has no base-instance draws");
	}
	if (instances == 1) {
		glDrawElementsBaseVertex(mode, static_cast<GLsizei>(indexCount), type, offset,
					 static_cast<GLint>(baseVertex));
	} else {
		glDrawElementsInstancedBaseVertex(mode, static_cast<GLsizei>(indexCount), type, offset,
						  static_cast<GLsizei>(instances),
						  static_cast<GLint>(baseVertex));
	}
#else
	if (instances == 1 && baseInstance == 0) {
		glDrawElementsBaseVertex(mode, static_cast<GLsizei>(indexCount), type, offset,
					 static_cast<GLint>(baseVertex));
	} else if (baseInstance == 0) {
		glDrawElementsInstancedBaseVertex(mode, static_cast<GLsizei>(indexCount), type, offset,
						  static_cast<GLsizei>(instances),
						  static_cast<GLint>(baseVertex));
	} else {
		glDrawElementsInstancedBaseVertexBaseInstance(
			mode, static_cast<GLsizei>(indexCount), type, offset,
			static_cast<GLsizei>(instances), static_cast<GLint>(baseVertex), baseInstance);
	}
#endif
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawTriangles(const Kld::OpDrawTriangles& op)
{
	return drawArrays(GL_TRIANGLES, op.vertexOffset, op.vertexCount, 1, 0);
}

Kld::HalStatus CommandExecutor::drawIndexedTriangles(const Kld::OpDrawIndexedTriangles& op)
{
	return drawElements(GL_TRIANGLES, op.indexCount, op.indexOffset, op.vertexOffset, 1, 0);
}

Kld::HalStatus CommandExecutor::drawLines(const Kld::OpDrawLines& op)
{
	applyLineWidth(op.thickness);
	return drawArrays(GL_LINES, op.vertexOffset, op.vertexCount, 1, 0);
}

Kld::HalStatus CommandExecutor::drawIndexedLines(const Kld::OpDrawIndexedLines& op)
{
	applyLineWidth(op.thickness);
	return drawElements(GL_LINES, op.indexCount, op.indexOffset, op.vertexOffset, 1, 0);
}

Kld::HalStatus CommandExecutor::drawTrianglesInstanced(const Kld::OpDrawTrianglesInstanced& op)
{
	return drawArrays(GL_TRIANGLES, op.vertexOffset, op.vertexCount, op.instanceCount,
			  op.firstInstance);
}

Kld::HalStatus CommandExecutor::drawIndexedTrianglesInstanced(
	const Kld::OpDrawIndexedTrianglesInstanced& op)
{
	return drawElements(GL_TRIANGLES, op.indexCount, op.indexOffset, op.vertexOffset,
			    op.instanceCount, op.firstInstance);
}

Kld::HalStatus CommandExecutor::drawLinesInstanced(const Kld::OpDrawLinesInstanced& op)
{
	applyLineWidth(op.thickness);
	return drawArrays(GL_LINES, op.vertexOffset, op.vertexCount, op.instanceCount, op.firstInstance);
}

Kld::HalStatus CommandExecutor::drawIndexedLinesInstanced(
	const Kld::OpDrawIndexedLinesInstanced& op)
{
	applyLineWidth(op.thickness);
	return drawElements(GL_LINES, op.indexCount, op.indexOffset, op.vertexOffset, op.instanceCount,
			    op.firstInstance);
}

Kld::HalStatus CommandExecutor::drawIndirect(const Kld::OpDrawIndirect& op)
{
	if (!graphicsPipelineBound(currentProgram_, computePipelineActive_)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no graphics pipeline is bound");
	}
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (op.drawCount == 0) {
		return Kld::HalOk;
	}
	// Non-indexed indirect commands are four 32-bit words (count, instanceCount,
	// firstVertex, baseInstance).
	if (op.stride != 0 && op.stride < 16) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the indirect stride is smaller than one command");
	}
	const std::uint32_t stride = op.stride != 0 ? op.stride : 16u;
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, object->name);
#if KALDIGL_IS_GLES
	// GLES 3.1 has single indirect draws only; a multi-draw range becomes a loop.
	for (std::uint32_t index = 0; index < op.drawCount; ++index) {
		glDrawArraysIndirect(GL_TRIANGLES,
				     reinterpret_cast<const void*>(static_cast<std::uintptr_t>(op.offset)
								   + static_cast<std::uintptr_t>(index) * stride));
	}
#else
	if (op.drawCount > 1) {
		glMultiDrawArraysIndirect(GL_TRIANGLES,
					  reinterpret_cast<const void*>(static_cast<std::uintptr_t>(op.offset)),
					  static_cast<GLsizei>(op.drawCount), static_cast<GLsizei>(stride));
	} else {
		glDrawArraysIndirect(GL_TRIANGLES,
				     reinterpret_cast<const void*>(static_cast<std::uintptr_t>(op.offset)));
	}
#endif
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::drawIndexedIndirect(const Kld::OpDrawIndexedIndirect& op)
{
	if (!graphicsPipelineBound(currentProgram_, computePipelineActive_)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no graphics pipeline is bound");
	}
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (op.drawCount == 0) {
		return Kld::HalOk;
	}
	if (!indexBinding_.valid) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "an indexed indirect draw needs a bound index buffer");
	}
	// Indexed indirect commands are five 32-bit words.
	if (op.stride != 0 && op.stride < 20) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the indirect stride is smaller than one command");
	}
	const std::uint32_t stride = op.stride != 0 ? op.stride : 20u;
	const GLenum type = detail::indexType(indexBinding_.type);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, object->name);
#if KALDIGL_IS_GLES
	for (std::uint32_t index = 0; index < op.drawCount; ++index) {
		glDrawElementsIndirect(GL_TRIANGLES, type,
				       reinterpret_cast<const void*>(static_cast<std::uintptr_t>(op.offset)
								     + static_cast<std::uintptr_t>(index) * stride));
	}
#else
	if (op.drawCount > 1) {
		glMultiDrawElementsIndirect(
			GL_TRIANGLES, type,
			reinterpret_cast<const void*>(static_cast<std::uintptr_t>(op.offset)),
			static_cast<GLsizei>(op.drawCount), static_cast<GLsizei>(stride));
	} else {
		glDrawElementsIndirect(GL_TRIANGLES, type,
				       reinterpret_cast<const void*>(static_cast<std::uintptr_t>(op.offset)));
	}
#endif
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::dispatchCompute(const Kld::OpDispatchCompute& op)
{
	if (currentProgram_ == 0 || !computePipelineActive_) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no compute pipeline is bound");
	}
	glDispatchCompute(op.groupCountX, op.groupCountY, op.groupCountZ);
	// GL does not track resource states; a full barrier is the portable
	// conservative choice before following commands observe the dispatch.
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::dispatchComputeIndirect(
	const Kld::OpDispatchComputeIndirect& op)
{
	if (currentProgram_ == 0 || !computePipelineActive_) {
		return fail(Kld::HalStatusCode::InvalidCommand, "no compute pipeline is bound");
	}
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, object->name);
	glDispatchComputeIndirect(op.offset);
	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	return Kld::HalOk;
}

}
