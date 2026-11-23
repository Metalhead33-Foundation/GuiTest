#ifndef LTERESOURCEFACTORY_HPP
#define LTERESOURCEFACTORY_HPP
/**
 * @file LteResourceFactory.hpp
 * @brief The Kaldi object-oriented facade wrapped as a resource factory.
 *
 * The legacy trees had one `ResourceFactory` per backend (`GL::ResourceFactory`,
 * `Software::SoftwareRenderer`, ...): a context object that created textures,
 * buffers, pipelines and framebuffers and recorded the matching native calls.
 * Kaldi already *is* that abstraction, so this class does not re-implement any of
 * it - it is the one place where Lotte allocates Kaldi handles from a
 * `CommandBuffer`, keeps the CPU payloads the recorded commands point at alive
 * until submission, and submits the recording in one call.
 *
 * @section lte_resource_factory_lifetime Payload lifetime
 *
 * Kaldi commands store raw pointers to pixel data, descriptors and shader words;
 * the backend reads them while `Kld::Device::processCommandQueue()` runs. The
 * factory therefore cannot copy anything it cannot own:
 *
 * - every `create*` overload that takes a pointer documents that the caller keeps
 *   the data alive until @ref submit();
 * - the `std::shared_ptr` overloads (and @ref retain()) let the factory hold that
 *   promise instead, by keeping the payload alive until the next submission.
 *
 * @section lte_resource_factory_submit Submission
 *
 * Recording and submitting are one cycle: `createXxx()` appends to the write
 * buffer, @ref submit() swaps it into the read buffer and hands it to the device.
 * Call `submit()` after building a frame, not after every individual resource.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Lotte/GFX/LteClipSpaceConventions.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Kaldi/KldCommandBuffer.hpp>
#include <Kaldi/KldDevice.hpp>
#include <Kaldi/KldHal.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Lotte {
namespace Gfx {

/** @brief Options accepted by the 2D texture creation helpers. */
struct TextureCreateOptions {
	/** @brief Mip levels to allocate; at least one. */
	std::uint8_t mipLevels = 1;
	/** @brief Ask the backend to generate the mips from level 0 when it can. */
	bool generateMipmaps = false;
};

/**
 * @brief Creates Kaldi resources through `Kld::oop_detail` and submits them.
 *
 * The factory borrows both the dispatcher and the device. The dispatcher must
 * outlive the factory and every proxy created from it; the device may be set
 * later with @ref setDevice() (useful when the backend is chosen at run time).
 */
class MH_LOTTE_API ResourceFactory {
public:
	/**
	 * @brief Binds the factory to a recording dispatcher.
	 * @param commands Dispatcher that receives every create/destroy command.
	 * @param device Optional device used by @ref submit(); may be set later.
	 */
	explicit ResourceFactory(CommandBuffer& commands, Kld::Device* device = nullptr);
	~ResourceFactory();

	ResourceFactory(const ResourceFactory&) = delete;
	ResourceFactory& operator=(const ResourceFactory&) = delete;
	ResourceFactory(ResourceFactory&&) = delete;
	ResourceFactory& operator=(ResourceFactory&&) = delete;

	/** @brief The dispatcher resources are recorded into. */
	CommandBuffer& commands() const noexcept { return *commands_; }
	/** @brief The device @ref submit() hands commands to, or @c nullptr. */
	Kld::Device* device() const noexcept { return device_; }
	/** @brief Selects (or clears) the submission device, refreshing @ref conventions(). */
	void setDevice(Kld::Device* device) noexcept;
	/** @brief True when @ref submit() has somewhere to submit to. */
	bool hasDevice() const noexcept { return device_ != nullptr; }

	/**
	 * @brief The backend's fixed-function conventions, derived from @ref device().
	 *
	 * Renderers take this from the factory instead of reading capabilities, so the
	 * pixel/depth/winding/rectangle adaptation happens below whoever issues draw
	 * commands and stays correct when the backend changes. With no device attached
	 * the OpenGL/Direct3D defaults apply.
	 */
	const ClipSpaceConventions& conventions() const noexcept { return conventions_; }

	/// @name Texture creation
	///@{
	/**
	 * @brief Creates an RGBA-capable 2D texture.
	 * @param format Pixel format of @p pixels (and of the texture).
	 * @param width Width in pixels; must be non-zero.
	 * @param height Height in pixels; must be non-zero.
	 * @param pixels Optional initial content, or @c nullptr for an empty texture.
	 * @param options Mip configuration.
	 * @return The owning texture proxy.
	 *
	 * @warning @p pixels must stay valid until @ref submit() when it is not null.
	 */
	Texture createTexture2D(Kld::TextureFormat format, std::uint16_t width, std::uint16_t height,
				const void* pixels = nullptr, TextureCreateOptions options = {});
	/**
	 * @brief Creates a 2D texture from the first frame of a decoded image.
	 * @param source Decoded image; must stay alive until @ref submit().
	 * @param options Mip configuration.
	 */
	Texture createTexture2D(const Euph::Media::Image::DecodeTarget& source,
				TextureCreateOptions options = {});
	/**
	 * @brief Creates a 2D texture and keeps @p source alive until submission.
	 * @param source Decoded image, shared so the factory can retain it.
	 * @param options Mip configuration.
	 */
	Texture createTexture2D(std::shared_ptr<const Euph::Media::Image::DecodeTarget> source,
				TextureCreateOptions options = {});
	///@}

	/// @name Buffer, sampler, pipeline, image and framebuffer creation
	///@{
	/**
	 * @brief Creates a buffer object.
	 * @param size Size in bytes.
	 * @param policy Kaldi `BUFFER_USAGE_*` bitmask.
	 * @param data Optional initial content; must stay alive until @ref submit().
	 * @param keepAlive Optional owner for @p data; retained until submission.
	 */
	Buffer createBuffer(std::uint32_t size, std::uint32_t policy = 0, const void* data = nullptr,
			    std::shared_ptr<void> keepAlive = {});

	/**
	 * @brief Creates a sampler that wraps on every axis and takes the GUI defaults.
	 * @param filtering Filtering mode.
	 * @param wrapping Wrapping used for U, V and W.
	 */
	Sampler createSampler(Kld::SamplerFiltering filtering,
			      Kld::SamplerWrapping wrapping = Kld::SamplerWrapping::CLAMP_TO_EDGE);
	/**
	 * @brief Creates a sampler with full control over wrapping and LODs.
	 * @param filtering Filtering mode.
	 * @param wrapping Per-axis wrapping modes (U, V, W).
	 * @param maxAnisotropy Maximum anisotropy; 1.0 disables it.
	 * @param maxLod Highest LOD that is sampled.
	 * @param minLod Lowest LOD that is sampled.
	 * @param lodBias Constant LOD bias.
	 */
	Sampler createSampler(Kld::SamplerFiltering filtering, const Kld::SamplerWrapping (&wrapping)[3],
			      float maxAnisotropy = 1.0f, float maxLod = 1000.0f,
			      float minLod = 0.0f, float lodBias = 0.0f);

	/**
	 * @brief Records a graphics pipeline creation.
	 * @param name Debug name; must stay alive until @ref submit().
	 * @param shaderBinaries SPIR-V descriptors; must stay alive until @ref submit().
	 * @param shaderBinaryCount Number of entries in @p shaderBinaries.
	 * @param vertexInput Vertex layout, or @c nullptr for a vertex-less pipeline.
	 * @param bindingLayout Optional resource layout metadata.
	 */
	Lotte::Pipeline createPipeline(const char* name,
				      const Kld::ShaderBinaryDescriptor* shaderBinaries,
				      std::uint32_t shaderBinaryCount,
				      const Kld::VertexDescriptor* vertexInput,
				      const Kld::PipelineBindingLayout* bindingLayout = nullptr);

	/** @brief Records a storage/render image creation. */
	Image createImage2D(Kld::TextureFormat format, std::uint16_t width, std::uint16_t height,
			    std::uint8_t mipLevels, std::uint32_t usage);

	/**
	 * @brief Records a framebuffer creation.
	 * @param width Width in pixels.
	 * @param height Height in pixels.
	 * @param attachments Attachment descriptions; must stay alive until @ref submit().
	 * @param attachmentCount Number of entries in @p attachments.
	 */
	Framebuffer createFramebuffer(std::uint16_t width, std::uint16_t height,
				     const Kld::FramebufferAttachment* attachments,
				     std::uint32_t attachmentCount);
	///@}

	/**
	 * @brief Keeps @p payload alive until the next submission.
	 *
	 * Use this when a recorded command points into memory that the factory does
	 * not otherwise own, for example a `WriteableTexture2D` pixel buffer or a
	 * vertex cache that is rebuilt every frame.
	 */
	void retain(std::shared_ptr<void> payload);
	/** @brief Number of payloads waiting for the next submission. */
	std::size_t retainedPayloadCount() const noexcept { return retained_.size(); }

	/**
	 * @brief Swaps the write buffer into the read buffer and submits it.
	 * @return The device's status, or `InitializationFailed` when no device is set.
	 *
	 * Retained payloads are released afterwards: `processCommandQueue()` is
	 * synchronous, so once it returns no command points at them any more.
	 */
	Kld::HalStatus submit();

private:
	/** @brief Non-owning dispatcher the factory records into. */
	CommandBuffer* commands_;
	/** @brief Non-owning submission target, possibly null. */
	Kld::Device* device_;
	/** @brief Payloads that must outlive the pending submission. */
	std::vector<std::shared_ptr<void> > retained_;
	/** @brief Conventions derived from @ref device_ whenever it changes. */
	ClipSpaceConventions conventions_;
};

} // namespace Gfx
} // namespace Lotte

#endif // LTERESOURCEFACTORY_HPP
