/**
 * @file GlExecutorTransfer.cpp
 * @brief Fence, query, copy and readback handlers for the GL executor.
 */
#include "GlCommandExecutor.hpp"
#include "GlExecutorMapping.hpp"
#include <algorithm>
#include <cstdio>
#include <vector>

namespace KALDIGL_NS {

namespace {

/** @brief Query target for a Kaldi occlusion query (`GL_ANY_SAMPLES_PASSED`). */
constexpr GLenum occlusionQueryTarget = GL_ANY_SAMPLES_PASSED;

} // namespace

Kld::HalStatus CommandExecutor::createFence(const Kld::OpCreateFence& op)
{
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "fence handle is zero or already in use");
	}
	GlFenceObject object;
	object.value = op.initialValue;
	fences_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyFence(const Kld::OpDestroyFence& op)
{
	const auto found = fences_.find(op.id);
	if (found == fences_.end()) {
		return failInvalidHandle("fence", op.id);
	}
	if (found->second.sync != nullptr) {
		glDeleteSync(found->second.sync);
	}
	fences_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::signalFence(const Kld::OpSignalFence& op)
{
	GlFenceObject* object = fence(op.id);
	if (object == nullptr) {
		return failInvalidHandle("fence", op.id);
	}
	if (object->sync != nullptr) {
		glDeleteSync(object->sync);
		object->sync = nullptr;
	}
	object->sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	if (object->sync == nullptr) {
		object->signalled = false;
		return fail(Kld::HalStatusCode::BackendError, "glFenceSync failed");
	}
	object->value = op.value;
	object->signalled = true;
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::waitFence(const Kld::OpWaitFence& op)
{
	GlFenceObject* object = fence(op.id);
	if (object == nullptr) {
		return failInvalidHandle("fence", op.id);
	}
	if (op.value > object->value) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the fence value was never signalled");
	}
	if (!object->signalled || object->sync == nullptr) {
		// Waiting for the initial value of a fence that has never been signalled is
		// trivially satisfied.
		return Kld::HalOk;
	}
	// A GL sync has no timeline value; the newest sync covers every value the
	// client signalled so far, so one wait suffices.
	const GLenum result = glClientWaitSync(object->sync, GL_SYNC_FLUSH_COMMANDS_BIT,
					       1000000000ull); // 1 second
	if (result == GL_TIMEOUT_EXPIRED) {
		return fail(Kld::HalStatusCode::BackendError, "glClientWaitSync timed out");
	}
	if (result == GL_WAIT_FAILED) {
		return fail(Kld::HalStatusCode::BackendError, "glClientWaitSync failed");
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::createQueryPool(const Kld::OpCreateQueryPool& op)
{
	if (op.queryCount == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a query pool needs at least one slot");
	}
	if (op.type == Kld::QueryType::Timestamp && !timestampQueriesEnabled_) {
		return failUnsupported("timestamp queries need glQueryCounter, which this dialect lacks");
	}
	if (op.id == 0 || isHandleInUse(op.id)) {
		return fail(Kld::HalStatusCode::InvalidCommand, "query pool handle is zero or already in use");
	}
	GlQueryPoolObject object;
	object.type = op.type;
	object.queries.resize(op.queryCount);
	glGenQueries(static_cast<GLsizei>(object.queries.size()), object.queries.data());
	for (const GLuint name : object.queries) {
		if (name == 0) {
			glDeleteQueries(static_cast<GLsizei>(object.queries.size()), object.queries.data());
			return fail(Kld::HalStatusCode::AllocationFailed, "glGenQueries returned no names");
		}
	}
	queryPools_.emplace(op.id, object);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::destroyQueryPool(const Kld::OpDestroyQueryPool& op)
{
	const auto found = queryPools_.find(op.id);
	if (found == queryPools_.end()) {
		return failInvalidHandle("query pool", op.id);
	}
	if (!found->second.queries.empty()) {
		glDeleteQueries(static_cast<GLsizei>(found->second.queries.size()),
				found->second.queries.data());
	}
	queryPools_.erase(found);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::resetQueryPool(const Kld::OpResetQueryPool& op)
{
	GlQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (op.firstQuery > pool->queries.size() || op.queryCount > pool->queries.size() - op.firstQuery) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query range is outside the pool");
	}
	// GL resets an occlusion query when glBeginQuery starts it, and a timestamp
	// query is written by glQueryCounter; there is no separate reset entry point.
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::beginQuery(const Kld::OpBeginQuery& op)
{
	GlQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (pool->type != Kld::QueryType::Occlusion) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "only occlusion queries use begin/end; timestamps use WriteTimestamp");
	}
	if (op.query >= pool->queries.size()) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query index is outside the pool");
	}
	glBeginQuery(occlusionQueryTarget, pool->queries[op.query]);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::endQuery(const Kld::OpEndQuery& op)
{
	GlQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (pool->type != Kld::QueryType::Occlusion) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "only occlusion queries use begin/end; timestamps use WriteTimestamp");
	}
	if (op.query >= pool->queries.size()) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query index is outside the pool");
	}
	glEndQuery(occlusionQueryTarget);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::writeTimestamp(const Kld::OpWriteTimestamp& op)
{
	GlQueryPoolObject* pool = queryPool(op.id);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.id);
	}
	if (pool->type != Kld::QueryType::Timestamp || !timestampQueriesEnabled_) {
		return fail(Kld::HalStatusCode::InvalidResourceState,
			    "the pool is not a supported timestamp pool");
	}
	if (op.query >= pool->queries.size()) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query index is outside the pool");
	}
#if KALDIGL_IS_GLES
	glQueryCounterEXT(pool->queries[op.query], GL_TIMESTAMP_EXT);
#else
	(void)op.pipelineStage; // GL timestamps are recorded where the command is issued.
	glQueryCounter(pool->queries[op.query], GL_TIMESTAMP);
#endif
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::readQueryResults(const Kld::OpReadQueryResults& op)
{
	GlQueryPoolObject* pool = queryPool(op.queryPool);
	if (pool == nullptr) {
		return failInvalidHandle("query pool", op.queryPool);
	}
	if (op.callback == nullptr) {
		return fail(Kld::HalStatusCode::InvalidCommand, "ReadQueryResults needs a callback");
	}
	if (op.firstQuery > pool->queries.size() || op.queryCount > pool->queries.size() - op.firstQuery) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the query range is outside the pool");
	}
	std::vector<std::uint64_t> values(op.queryCount, 0);
	for (std::uint32_t index = 0; index < op.queryCount; ++index) {
		const GLuint name = pool->queries[op.firstQuery + index];
		if (pool->type == Kld::QueryType::Occlusion) {
			GLuint value = 0;
			glGetQueryObjectuiv(name, GL_QUERY_RESULT, &value);
			values[index] = value;
		} else {
#if KALDIGL_IS_GLES
			GLuint64 value = 0;
			glGetQueryObjectui64vEXT(name, GL_QUERY_RESULT, &value);
			values[index] = static_cast<std::uint64_t>(value);
#else
			GLuint64 value = 0;
			glGetQueryObjectui64v(name, GL_QUERY_RESULT, &value);
			values[index] = static_cast<std::uint64_t>(value);
#endif
		}
	}
	op.callback(values.data(), op.queryCount, op.userData);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::readBuffer(const Kld::OpReadBuffer& op)
{
	GlBufferObject* object = buffer(op.buffer);
	if (object == nullptr) {
		return failInvalidHandle("buffer", op.buffer);
	}
	if (op.callback == nullptr || op.offset > object->size || op.size > object->size - op.offset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the readback range or callback is invalid");
	}
	glBindBuffer(object->target, object->name);
	void* mapped = glMapBufferRange(object->target, op.offset, op.size, GL_MAP_READ_BIT);
	if (mapped == nullptr) {
		glBindBuffer(object->target, 0);
		return fail(Kld::HalStatusCode::BackendError, "glMapBufferRange failed for readback");
	}
	op.callback(mapped, op.size, op.userData);
	glUnmapBuffer(object->target);
	glBindBuffer(object->target, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::readTexturePixels(const GlTextureObject& object,
						  std::uint8_t mipLevel, std::uint16_t layer,
						  std::uint16_t x, std::uint16_t y,
						  std::uint16_t width, std::uint16_t height,
						  void* destination, std::uint32_t destinationSize,
						  std::uint32_t* rowPitch)
{
	if (detail::isDepthStencilFormat(object.format)) {
		return failUnsupported("readback of depth/stencil images is not implemented by the GL backend");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(object.format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("readback needs an uncompressed, colour-renderable format");
	}
	const std::uint32_t pixelSize = detail::clientPixelSize(translated->format, translated->type);
	const std::uint32_t pitch = static_cast<std::uint32_t>(width) * pixelSize;
	if (destination == nullptr || destinationSize < static_cast<std::uint64_t>(pitch) * height) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the readback destination is too small");
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, scratchReadFramebuffer_);
	(void)attachImage(object, GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mipLevel, layer);
	const GLenum framebufferStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
	const bool complete = framebufferStatus == GL_FRAMEBUFFER_COMPLETE;
	if (!complete) {
		char text[96];
		std::snprintf(text, sizeof(text),
			      "the image is not colour-renderable (framebuffer status 0x%x)",
			      static_cast<unsigned>(framebufferStatus));
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, object.target, 0, 0);
		if (boundFramebuffer_ == Kld::DefaultFramebuffer) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			GlFramebufferObject* framebufferObject = framebuffer(boundFramebuffer_);
			glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject != nullptr ? framebufferObject->name : 0);
		}
		return failUnsupported(text);
	}
	if (complete) {
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(x, y, width, height, translated->format, translated->type, destination);
	}
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, object.target, 0, 0);
	if (boundFramebuffer_ == Kld::DefaultFramebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else {
		GlFramebufferObject* framebufferObject = framebuffer(boundFramebuffer_);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject != nullptr ? framebufferObject->name : 0);
	}
	if (!complete) {
		return failUnsupported("the image is not colour-renderable, so glReadPixels cannot read it");
	}
	if (rowPitch != nullptr) {
		*rowPitch = pitch;
	}
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::readImage2D(const Kld::OpReadImage2D& op)
{
	GlTextureObject* object = image(op.image);
	if (object == nullptr) {
		return failInvalidHandle("image", op.image);
	}
	if (op.callback == nullptr || op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the readback extent or callback is invalid");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(object->format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("readback needs an uncompressed, colour-renderable format");
	}
	const std::uint32_t pixelSize = detail::clientPixelSize(translated->format, translated->type);
	std::vector<std::uint8_t> pixels(static_cast<std::size_t>(op.width) * op.height * pixelSize);
	std::uint32_t rowPitch = 0;
	const Kld::HalStatus status =
		readTexturePixels(*object, op.mipLevel, op.layer, op.x, op.y, op.width, op.height,
				  pixels.data(), static_cast<std::uint32_t>(pixels.size()), &rowPitch);
	if (!status.ok()) {
		return status;
	}
	op.callback(pixels.data(), static_cast<std::uint32_t>(pixels.size()), op.width, op.height,
		    rowPitch, op.userData);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::copyBuffer(const Kld::OpCopyBuffer& op)
{
	GlBufferObject* source = buffer(op.source);
	GlBufferObject* destination = buffer(op.destination);
	if (source == nullptr) {
		return failInvalidHandle("buffer", op.source);
	}
	if (destination == nullptr) {
		return failInvalidHandle("buffer", op.destination);
	}
	if (op.sourceOffset > source->size || op.size > source->size - op.sourceOffset
	    || op.destinationOffset > destination->size
	    || op.size > destination->size - op.destinationOffset) {
		return fail(Kld::HalStatusCode::InvalidCommand, "the copy range is outside a buffer");
	}
	glBindBuffer(GL_COPY_READ_BUFFER, source->name);
	glBindBuffer(GL_COPY_WRITE_BUFFER, destination->name);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, op.sourceOffset,
			    op.destinationOffset, op.size);
	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::copyImage2D(const Kld::OpCopyImage2D& op)
{
	GlTextureObject* source = image(op.sourceImage);
	GlTextureObject* destination = image(op.destinationImage);
	if (source == nullptr) {
		return failInvalidHandle("image", op.sourceImage);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destinationImage);
	}
	if (source->multisampled || destination->multisampled) {
		return failUnsupported("image-to-image copies need single-sample images; use ResolveImage2D");
	}
	if (op.width == 0 || op.height == 0) {
		return fail(Kld::HalStatusCode::InvalidCommand, "a copy needs a non-zero extent");
	}
	glCopyImageSubData(source->name, source->target, op.sourceMipLevel, op.sourceX, op.sourceY,
			   op.sourceLayer, destination->name, destination->target,
			   op.destinationMipLevel, op.destinationX, op.destinationY, op.destinationLayer,
			   op.width, op.height, 1);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::copyBufferToImage2D(const Kld::OpCopyBufferToImage2D& op)
{
	GlBufferObject* source = buffer(op.sourceBuffer);
	GlTextureObject* destination = image(op.destinationImage);
	if (source == nullptr) {
		return failInvalidHandle("buffer", op.sourceBuffer);
	}
	if (destination == nullptr) {
		return failInvalidHandle("image", op.destinationImage);
	}
	if (destination->multisampled || op.layer != 0) {
		return failUnsupported("buffer uploads target single-layer, single-sample 2D images");
	}
	if (detail::isDepthStencilFormat(destination->format)) {
		return failUnsupported("buffer uploads into depth/stencil images are not implemented");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(destination->format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("buffer uploads need an uncompressed image format");
	}
	const std::uint32_t pixelSize = detail::clientPixelSize(translated->format, translated->type);
	if (op.bufferRowPitch != 0 && pixelSize != 0) {
		glPixelStorei(GL_UNPACK_ROW_LENGTH,
			      static_cast<GLint>(op.bufferRowPitch / pixelSize));
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, source->name);
	glBindTexture(GL_TEXTURE_2D, destination->name);
	glTexSubImage2D(GL_TEXTURE_2D, op.mipLevel, op.x, op.y, op.width, op.height,
			translated->format, translated->type,
			reinterpret_cast<const void*>(static_cast<std::uintptr_t>(op.bufferOffset)));
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	return Kld::HalOk;
}

Kld::HalStatus CommandExecutor::copyImage2DToBuffer(const Kld::OpCopyImage2DToBuffer& op)
{
	GlTextureObject* source = image(op.sourceImage);
	GlBufferObject* destination = buffer(op.destinationBuffer);
	if (source == nullptr) {
		return failInvalidHandle("image", op.sourceImage);
	}
	if (destination == nullptr) {
		return failInvalidHandle("buffer", op.destinationBuffer);
	}
	if (op.layer != 0 || detail::isDepthStencilFormat(source->format)) {
		return failUnsupported("image readback into a buffer supports single-layer colour images");
	}
	const std::optional<TextureFormat> translated = translateTextureFormat(source->format);
	if (!translated.has_value() || translated->compressed) {
		return failUnsupported("buffer readback needs an uncompressed image format");
	}
	const std::uint32_t pixelSize = detail::clientPixelSize(translated->format, translated->type);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, scratchReadFramebuffer_);
	(void)attachImage(*source, GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, op.mipLevel, 0);
	const bool complete = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	if (complete) {
		if (op.bufferRowPitch != 0 && pixelSize != 0) {
			glPixelStorei(GL_PACK_ROW_LENGTH,
				      static_cast<GLint>(op.bufferRowPitch / pixelSize));
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER, destination->name);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(op.x, op.y, op.width, op.height, translated->format, translated->type,
			     reinterpret_cast<void*>(static_cast<std::uintptr_t>(op.bufferOffset)));
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	}
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, source->target, 0, 0);
	if (boundFramebuffer_ == Kld::DefaultFramebuffer) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	} else {
		GlFramebufferObject* framebufferObject = framebuffer(boundFramebuffer_);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject != nullptr ? framebufferObject->name : 0);
	}
	if (!complete) {
		return failUnsupported("the image is not colour-renderable, so glReadPixels cannot read it");
	}
	return Kld::HalOk;
}

}
