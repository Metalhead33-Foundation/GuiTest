#ifndef LTEWRITEABLETEXTURE2D_HPP
#define LTEWRITEABLETEXTURE2D_HPP
/**
 * @file LteWriteableTexture2D.hpp
 * @brief A CPU-backed RGBA8 texture that uploads only its dirty rectangle.
 *
 * Salvaged from `GFX/GL/GlWriteableTexture2D.{hpp,cpp}` on the `GuiTest-legacy2`
 * tree, which wrapped an `Image2D` next to a GL texture name and tracked the
 * union of updated rectangles so `update()` could call `glTexSubImage2D()` per
 * changed row instead of re-uploading the whole surface.
 *
 * This version keeps the idea and replaces the GL calls with Kaldi commands:
 *
 * - the CPU side is one tightly packed `RGBA8` buffer (the format every GUI
 *   surface in this engine ends up in, and the one `UpdateTexture2D` can express
 *   without a format conversion);
 * - the GPU side is a `Lotte::Texture` proxy created through
 *   @ref Lotte::Gfx::ResourceFactory, so the destroy command is recorded
 *   automatically;
 * - `recordUpdate()` appends one `UpdateTexture2D` per dirty row, or a single
 *   command when the dirty rectangle spans the full width, exactly like the
 *   legacy implementation did for `glTexSubImage2D()`.
 *
 * @section lte_writeable_texture_atlas Texture atlases
 *
 * Glyph atlases pack many small surfaces into one page: every new glyph touches
 * a small rectangle. Writing it into `pixels()` and calling
 * `markDirty()` before `recordUpdate()` makes the upload proportional to
 * the new glyph, not to the page. `blitRgba8()` is the CPU-side "stamp a
 * decoded image into the page" operation.
 *
 * @warning The pixel buffer must stay alive until the recorded commands have
 *          been submitted. The owning `WriteableTexture2D` is normally enough;
 *          when it can be destroyed earlier, hand `pixelStorage()` to
 *          `ResourceFactory::retain()`.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/LteKld.hpp>
#include <Lotte/GFX/LteImageConvert.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <cstdint>
#include <memory>
#include <vector>

namespace Lotte {
namespace Gfx {

/**
 * @brief Mutable RGBA8 surface mirrored by a Kaldi 2D texture.
 *
 * Move-only, because the texture proxy is move-only and owns the Kaldi handle.
 */
class MH_LOTTE_API WriteableTexture2D {
public:
	/** @brief Half-open dirty rectangle in pixels (`x + width`, `y + height`). */
	struct DirtyRegion {
		/** @brief Left edge. */
		std::uint16_t x = 0;
		/** @brief Top edge. */
		std::uint16_t y = 0;
		/** @brief Width; zero when nothing is dirty. */
		std::uint16_t width = 0;
		/** @brief Height; zero when nothing is dirty. */
		std::uint16_t height = 0;

		/** @brief True when the region is empty. */
		bool empty() const noexcept { return width == 0 || height == 0; }
	};

	/** @brief Creates an empty, invalid surface. */
	WriteableTexture2D();
	~WriteableTexture2D();

	WriteableTexture2D(const WriteableTexture2D&) = delete;
	WriteableTexture2D& operator=(const WriteableTexture2D&) = delete;
	WriteableTexture2D(WriteableTexture2D&&) noexcept;
	WriteableTexture2D& operator=(WriteableTexture2D&&) noexcept;

	/**
	 * @brief Creates a transparent `width` x `height` surface and its texture.
	 * @param factory Factory that records the create/update commands.
	 * @param width Width in pixels; must be non-zero.
	 * @param height Height in pixels; must be non-zero.
	 *
	 * The initial (transparent) content is queued immediately, so the GPU texture
	 * is never uninitialised.
	 */
	static WriteableTexture2D create(ResourceFactory& factory, std::uint16_t width,
					 std::uint16_t height);
	/**
	 * @brief Creates a surface from a decoded image, converting it to RGBA8.
	 * @param factory Factory that records the create/update commands.
	 * @param source Decoded image; only frame 0 is used.
	 * @throws std::invalid_argument When the format cannot be converted.
	 */
	static WriteableTexture2D fromDecodeTarget(ResourceFactory& factory,
						   const Euph::Media::Image::DecodeTarget& source);
	/**
	 * @brief Creates a surface from an already converted image.
	 * @param factory Factory that records the create/update commands.
	 * @param image RGBA8 image to take over; must be valid.
	 */
	static WriteableTexture2D fromRgba8(ResourceFactory& factory, Rgba8Image image);

	/** @brief True when a texture handle and a matching CPU buffer exist. */
	bool isValid() const noexcept;
	/** @brief Width in pixels. */
	std::uint16_t width() const noexcept { return width_; }
	/** @brief Height in pixels. */
	std::uint16_t height() const noexcept { return height_; }
	/** @brief Bytes in one row of @ref pixels(). */
	std::size_t rowBytes() const noexcept { return std::size_t(width_) * 4u; }
	/** @brief Bytes in the whole CPU buffer. */
	std::size_t byteSize() const noexcept { return rowBytes() * std::size_t(height_); }

	/** @brief Mutable RGBA8 pixels, row-major, top row first; null when invalid. */
	std::uint8_t* pixels() noexcept;
	/** @brief Read-only RGBA8 pixels; null when invalid. */
	const std::uint8_t* pixels() const noexcept;
	/** @brief The owning pixel buffer, for `ResourceFactory::retain()`. */
	std::shared_ptr<std::vector<std::uint8_t> > pixelStorage() const noexcept { return pixels_; }

	/** @brief The Kaldi handle, or 0 when invalid. */
	Kld::HandleId handle() const noexcept { return texture_.id(); }
	/** @brief The texture proxy, for binding helpers that take a proxy. */
	const Lotte::Texture& texture() const noexcept { return texture_; }

	/**
	 * @brief Resizes the surface and its GPU mirror.
	 * @param width New width in pixels.
	 * @param height New height in pixels.
	 *
	 * The content is discarded (the legacy implementation resized the image and
	 * re-uploaded it whole as well) and the entire surface is marked dirty.
	 */
	void resize(std::uint16_t width, std::uint16_t height);

	/**
	 * @brief Fills the whole surface with one colour.
	 * @param red Red byte.
	 * @param green Green byte.
	 * @param blue Blue byte.
	 * @param alpha Alpha byte; 0 makes the surface transparent.
	 */
	void fill(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha);
	/**
	 * @brief Writes one pixel and marks it dirty.
	 * @param x Column; out-of-range coordinates are ignored.
	 * @param y Row; out-of-range coordinates are ignored.
	 * @param red Red byte.
	 * @param green Green byte.
	 * @param blue Blue byte.
	 * @param alpha Alpha byte; 0 makes the pixel transparent.
	 */
	void setPixel(std::uint16_t x, std::uint16_t y, std::uint8_t red, std::uint8_t green,
		      std::uint8_t blue, std::uint8_t alpha);
	/**
	 * @brief Copies a tightly packed RGBA8 image into the surface.
	 * @param source Pixel data, `width * height * 4` bytes.
	 * @param width Source width in pixels.
	 * @param height Source height in pixels.
	 * @param x Destination left edge.
	 * @param y Destination top edge.
	 * @return True when at least one pixel was written.
	 *
	 * Clipping is per pixel, so a source rectangle that hangs over the edge is
	 * copied partially instead of being rejected. This is the atlas "stamp" call.
	 */
	bool blitRgba8(const std::uint8_t* source, std::uint16_t width, std::uint16_t height,
		       std::uint16_t x = 0, std::uint16_t y = 0);

	/**
	 * @brief Extends the dirty rectangle.
	 * @param x Left edge in pixels.
	 * @param y Top edge in pixels.
	 * @param width Width in pixels.
	 * @param height Height in pixels.
	 */
	void markDirty(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height);
	/** @brief Marks the whole surface dirty. */
	void markAllDirty();
	/** @brief True when @ref recordUpdate() has work to do. */
	bool hasDirtyRegion() const noexcept { return !dirtyRegion().empty(); }
	/** @brief The union of every change since the last @ref recordUpdate(). */
	DirtyRegion dirtyRegion() const noexcept;
	/** @brief Forgets the dirty rectangle without recording anything. */
	void clearDirty() noexcept;

	/**
	 * @brief Appends `UpdateTexture2D` commands for the dirty rectangle.
	 *
	 * A rectangle that spans the full width is one command; anything narrower is
	 * one command per row, because Kaldi uploads tightly packed rows and the
	 * legacy GL path had to do the same thing.
	 *
	 * The recorded commands point into @ref pixels(), so this object (or its
	 * pixel storage) must outlive the submission.
	 */
	void recordUpdate();

private:
	WriteableTexture2D(Texture texture, std::shared_ptr<std::vector<std::uint8_t> > pixels,
			   std::uint16_t width, std::uint16_t height);

	/** @brief The GPU mirror; records `DestroyTexture` when it dies. */
	Texture texture_;
	/** @brief The CPU pixels; shared so callers can extend their lifetime. */
	std::shared_ptr<std::vector<std::uint8_t> > pixels_;
	/** @brief Surface width in pixels. */
	std::uint16_t width_ = 0;
	/** @brief Surface height in pixels. */
	std::uint16_t height_ = 0;
	/** @brief Dirty rectangle, half-open; `x1 <= x0` means empty. */
	std::int32_t dirtyX0_ = 0;
	std::int32_t dirtyY0_ = 0;
	std::int32_t dirtyX1_ = 0;
	std::int32_t dirtyY1_ = 0;
};

} // namespace Gfx
} // namespace Lotte

#endif // LTEWRITEABLETEXTURE2D_HPP
