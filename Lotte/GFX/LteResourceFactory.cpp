#include "LteResourceFactory.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace Lotte {
namespace Gfx {

namespace {

/** @brief Throws when @p width or @p height cannot fit a Kaldi 2D extent. */
void requireTextureExtents(std::uint16_t width, std::uint16_t height)
{
	if (width == 0 || height == 0) {
		throw std::invalid_argument("ResourceFactory: a 2D texture needs non-zero extents");
	}
}

} // namespace

ResourceFactory::ResourceFactory(CommandBuffer& commands, Kld::Device* device)
	: commands_(&commands), device_(device)
{
}

ResourceFactory::~ResourceFactory() = default;

Texture ResourceFactory::createTexture2D(Kld::TextureFormat format, std::uint16_t width,
					 std::uint16_t height, const void* pixels,
					 TextureCreateOptions options)
{
	requireTextureExtents(width, height);
	return KldOopDetail::createTexture2D(*commands_, format,
			std::max<std::uint8_t>(1, options.mipLevels), options.generateMipmaps,
			width, height, pixels);
}

Texture ResourceFactory::createTexture2D(const Euph::Media::Image::DecodeTarget& source,
					 TextureCreateOptions options)
{
	if (source.getFrames().empty()) {
		throw std::invalid_argument("ResourceFactory: the decode target has no frames");
	}
	const Euph::Media::Image::Frame& frame = source.getFrame(0);
	if (frame.width > 0xFFFFu || frame.height > 0xFFFFu) {
		throw std::invalid_argument("ResourceFactory: the frame exceeds the 16-bit extent limit");
	}
	if (frame.data.empty()) {
		throw std::invalid_argument("ResourceFactory: the frame has no pixel storage");
	}
	return createTexture2D(source.getFormat(), static_cast<std::uint16_t>(frame.width),
			static_cast<std::uint16_t>(frame.height), frame.data.data(), options);
}

Texture ResourceFactory::createTexture2D(
	std::shared_ptr<const Euph::Media::Image::DecodeTarget> source,
	TextureCreateOptions options)
{
	if (!source) {
		throw std::invalid_argument("ResourceFactory: the decode target is null");
	}
	Texture texture = createTexture2D(*source, options);
	// Drop the constness only for ownership; the payload is never mutated here.
	retain(std::const_pointer_cast<Euph::Media::Image::DecodeTarget>(std::move(source)));
	return texture;
}

Buffer ResourceFactory::createBuffer(std::uint32_t size, std::uint32_t policy,
				     const void* data, std::shared_ptr<void> keepAlive)
{
	if (size == 0) {
		throw std::invalid_argument("ResourceFactory: a buffer needs a non-zero size");
	}
	if (keepAlive) {
		retain(std::move(keepAlive));
	}
	return KldOopDetail::createBuffer(*commands_, size, policy, data);
}

Sampler ResourceFactory::createSampler(Kld::SamplerFiltering filtering,
				       Kld::SamplerWrapping wrapping)
{
	const Kld::SamplerWrapping wrappingAll[3] = { wrapping, wrapping, wrapping };
	return createSampler(filtering, wrappingAll);
}

Sampler ResourceFactory::createSampler(Kld::SamplerFiltering filtering,
				       const Kld::SamplerWrapping (&wrapping)[3],
				       float maxAnisotropy, float maxLod, float minLod,
				       float lodBias)
{
	return KldOopDetail::createSampler(*commands_, filtering, wrapping, maxAnisotropy,
					   maxLod, minLod, lodBias);
}

Lotte::Pipeline ResourceFactory::createPipeline(const char* name,
						const Kld::ShaderBinaryDescriptor* shaderBinaries,
						std::uint32_t shaderBinaryCount,
						const Kld::VertexDescriptor* vertexInput,
						const Kld::PipelineBindingLayout* bindingLayout)
{
	if (name == nullptr || shaderBinaries == nullptr || shaderBinaryCount == 0) {
		throw std::invalid_argument("ResourceFactory: a pipeline needs a name and shaders");
	}
	return KldOopDetail::createPipeline(*commands_, name, shaderBinaries, shaderBinaryCount,
					    vertexInput, bindingLayout);
}

Image ResourceFactory::createImage2D(Kld::TextureFormat format, std::uint16_t width,
				     std::uint16_t height, std::uint8_t mipLevels,
				     std::uint32_t usage)
{
	requireTextureExtents(width, height);
	return KldOopDetail::createImage2D(*commands_, format, width, height,
			std::max<std::uint8_t>(1, mipLevels), usage);
}

Framebuffer ResourceFactory::createFramebuffer(std::uint16_t width, std::uint16_t height,
					       const Kld::FramebufferAttachment* attachments,
					       std::uint32_t attachmentCount)
{
	requireTextureExtents(width, height);
	if (attachments == nullptr || attachmentCount == 0) {
		throw std::invalid_argument("ResourceFactory: a framebuffer needs attachments");
	}
	return KldOopDetail::createFramebuffer(*commands_, width, height, attachments,
					      attachmentCount);
}

void ResourceFactory::retain(std::shared_ptr<void> payload)
{
	if (payload) {
		retained_.push_back(std::move(payload));
	}
}

Kld::HalStatus ResourceFactory::submit()
{
	if (commands_ == nullptr) {
		return { Kld::HalStatusCode::InitializationFailed,
			 "ResourceFactory: no command buffer" };
	}
	if (device_ == nullptr) {
		return { Kld::HalStatusCode::InitializationFailed,
			 "ResourceFactory: no device to submit to" };
	}
	commands_->getCommandBuffer().swapBuffers();
	const Kld::HalStatus status = commands_->submit(*device_);
	// `processCommandQueue()` is synchronous, so nothing in the submitted command
	// span can still point at these payloads.
	retained_.clear();
	return status;
}

} // namespace Gfx
} // namespace Lotte
