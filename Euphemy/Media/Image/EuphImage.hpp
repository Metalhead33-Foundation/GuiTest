#ifndef EUPHIMAGE_HPP
#define EUPHIMAGE_HPP
/**
 * @file EuphImage.hpp
 * @brief Declares the EuphImage API in the Euphemy/Media/Image module.
 *
 * This header is part of the public declaration surface for Euphemy/Media/Image.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */

#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Media/Image/EuphImageType.hpp>
#include <Euphemy/Media/Image/EuphPixelFormat.hpp>
#include <Elvavena/Util/ElvEdgeFunction.hpp>
#include <Elvavena/Util/ElvSpanHelpers.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <memory_resource>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <optional>

namespace Euph {
namespace Media {
namespace Image {

/**
 * @struct ImageDimensions
 * @brief Stores image extents together with cached values used for sampling.
 */
struct ImageDimensions {
	unsigned width;  /**< Width in pixels. */
	unsigned height; /**< Height in pixels. */
	unsigned stride; /**< Row stride in bytes. */
	float widthF;	 /**< width - 1 (maximum texel X coordinate). */
	float heightF;	 /**< height - 1 (maximum texel Y coordinate). */
	float widthR;	 /**< Reciprocal of widthF. */
	float heightR;	 /**< Reciprocal of heightF. */

	/**
	 * @brief Recomputes row stride using a specific image format.
	 * @param format Format used to compute bytes-per-pixel.
	 */
	inline void recalculateStride(Format format) {
		stride = width * pixelByteSize(format);
	}

	/**
	 * @brief Recomputes cached floating-point values used during texture sampling.
	 */
	inline void recalculateFloats() {
		widthF = (width > 1) ? static_cast<float>(width - 1) : 0.0f;
		heightF = (height > 1) ? static_cast<float>(height - 1) : 0.0f;
		widthR = (widthF > 0.0f) ? (1.0f / widthF) : 0.0f;
		heightR = (heightF > 0.0f) ? (1.0f / heightF) : 0.0f;
	}
};

/**
 * @concept TextureTypeConcept
 * @brief Constrains image-like types that can be sampled by this module.
 */
template <typename T>
concept TextureTypeConcept =
	requires(const T& image, const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) {
		{ image.getDimensions() } -> std::same_as<const ImageDimensions&>;
		{ image.getPixel(pos, colourKernel, wrap) } -> std::same_as<void>;
	};

/**
 * @concept ColourProgramConcept
 * @brief Constrains generators used by `clearToColour` overloads.
 */
template <typename F>
concept ColourProgramConcept =
	std::is_invocable_v<std::remove_reference_t<F>, glm::uvec2> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::uvec2, glm::fvec4> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::fvec2> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::fvec2, glm::fvec4>;

/**
 * @concept ColourIteratorConcept
 * @brief Constrains callbacks used to iterate decoded texel colours.
 */
template <typename F>
concept ColourIteratorConcept =
	std::is_invocable_v<std::remove_reference_t<F>, glm::uvec2, glm::fvec4> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::fvec2, glm::fvec4>;

/**
 * @brief Samples a texture using the selected filtering and wrapping modes.
 * @tparam ImageType Type satisfying TextureTypeConcept.
 * @param image Source image.
 * @param pos Normalized texture coordinate in [0,1] space.
 * @param screenpos Screen-space pixel coordinate used by dithered modes.
 * @param colourKernel Output colour in kernel representation.
 * @param filteringType Filtering algorithm to apply.
 * @param wrap Wrapping mode for out-of-range coordinates.
 */
template <TextureTypeConcept ImageType>
inline void sampleTexture(const ImageType& image, const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap);

namespace detail {

/** @brief Documents the always_false_v declaration. */
template <typename> inline constexpr bool always_false_v = false;

/** @brief Documents the wrapCoordinate declaration. */
inline unsigned wrapCoordinate(unsigned coord, unsigned size, Wrap wrap) {
	if(size == 0) {
		return 0;
	}
	switch(wrap) {
		case Wrap::REPEAT:
			return coord % size;
		case Wrap::MIRRORED_REPEAT: {
			const unsigned doubleSize = 2 * size;
			const unsigned m = coord % doubleSize;
			return (m < size) ? m : (doubleSize - 1 - m);
		}
		case Wrap::CLAMP_TO_BORDER:
		case Wrap::CLAMP_TO_EDGE:
		default:
			return std::min(coord, size - 1);
	}
}

template <typename F>
/** @brief Documents the evalClearProgram declaration. */
inline glm::fvec4 evalClearProgram(F&& program, const glm::uvec2& pos, float widthR, float heightR, const glm::fvec4& existing) {
	using Fn = std::remove_reference_t<F>;
	const glm::fvec2 normalizedPos(static_cast<float>(pos.x) * widthR, static_cast<float>(pos.y) * heightR);
	// Prefer normalized coordinates when both forms are implicitly invocable.
	if constexpr (std::is_invocable_v<Fn, glm::fvec2>) {
		return program(normalizedPos);
	} else if constexpr (std::is_invocable_v<Fn, glm::fvec2, glm::fvec4>) {
		return program(normalizedPos, existing);
	} else if constexpr (std::is_invocable_v<Fn, glm::uvec2>) {
		return program(pos);
	} else if constexpr (std::is_invocable_v<Fn, glm::uvec2, glm::fvec4>) {
		return program(pos, existing);
	} else {
		static_assert(always_false_v<Fn>, "clearToColour program must be invocable with (uvec2), (uvec2,fvec4), (fvec2), or (fvec2,fvec4)");
	}
}

template <typename F>
/** @brief Documents the invokeIterator declaration. */
inline void invokeIterator(F&& program, const glm::uvec2& pos, const glm::fvec4& colourKernel, float widthR, float heightR) {
	using Fn = std::remove_reference_t<F>;
	// Prefer normalized coordinates when both forms are implicitly invocable.
	if constexpr (std::is_invocable_v<Fn, glm::fvec2, glm::fvec4>) {
		program(glm::fvec2(static_cast<float>(pos.x) * widthR, static_cast<float>(pos.y) * heightR), colourKernel);
	} else if constexpr (std::is_invocable_v<Fn, glm::uvec2, glm::fvec4>) {
		program(pos, colourKernel);
	} else {
		static_assert(always_false_v<Fn>, "iterator must be invocable with (uvec2,fvec4) or (fvec2,fvec4)");
	}
}

} // namespace detail

/**
 * @class Image2D
 * @brief Owns a 2D image buffer with typed pixel storage.
 * @tparam PixelType Pixel storage type.
 */
template <PixelConcept PixelType> class Image2D {
private:
	std::pmr::vector<PixelType> pixels;
	ImageDimensions dimensions;

	template <typename F>
	static inline void clearToColourImpl(std::span<PixelType> px, unsigned width, unsigned height, float widthR, float heightR, F&& program, const glm::uvec2& offset, const glm::uvec2& affectedDimensions, bool dither) {
		Elv::Util::over_2d_span_mut<PixelType>(px, [&](PixelType& dst, const glm::uvec2& pos) {
			glm::fvec4 result;
			constexpr glm::fvec4 unusedExisting(0.0f);
			if constexpr (
				std::is_invocable_v<std::remove_reference_t<F>, glm::uvec2, glm::fvec4> ||
				std::is_invocable_v<std::remove_reference_t<F>, glm::fvec2, glm::fvec4>) {
				glm::fvec4 existing;
				dst.toKernel(existing);
				result = detail::evalClearProgram(program, pos, widthR, heightR, existing);
			} else {
				result = detail::evalClearProgram(program, pos, widthR, heightR, unusedExisting);
			}

			if(dither) {
				dst.fromKernelDithered(result, pos);
			} else {
				dst.fromKernel(result);
			}
		}, glm::uvec2(width, height), offset, affectedDimensions);
	}

public:
	/**
	 * @brief Constructs an empty image using a specific allocator resource.
	 * @param memResource Polymorphic allocator resource.
	 */
	explicit Image2D(std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), dimensions{0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f} {}

	/**
	 * @brief Constructs an image of the requested dimensions.
	 * @param width Width in pixels.
	 * @param height Height in pixels.
	 * @param memResource Polymorphic allocator resource.
	 */
	Image2D(unsigned width, unsigned height, std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), dimensions{width, height, 0, 0.0f, 0.0f, 0.0f, 0.0f} {
		dimensions.recalculateStride(PixelType::FMT_ID);
		dimensions.recalculateFloats();
		pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
	}

	Image2D(const Image2D&) = default;
	/** @brief Documents the Image2D type or declaration. */
	Image2D(Image2D&&) noexcept = default;
	/** @brief Documents the operator= helper. */
	Image2D& operator=(const Image2D&) = default;
	/** @brief Documents the operator= helper. */
	Image2D& operator=(Image2D&&) noexcept = default;

	/**
	 * @brief Copy-constructs this image into a new allocator resource.
	 * @param other Source image.
	 * @param memResource Destination allocator resource.
	 */
	Image2D(const Image2D& other, std::pmr::memory_resource* memResource)
		: pixels(other.pixels, memResource), dimensions(other.dimensions) {}

	/**
	 * @brief Move-constructs this image into a new allocator resource.
	 * @param other Source image.
	 * @param memResource Destination allocator resource.
	 */
	Image2D(Image2D&& other, std::pmr::memory_resource* memResource)
		: pixels(std::move(other.pixels), memResource), dimensions(other.dimensions) {}

	/**
	 * @brief Returns mutable access to the raw pixel buffer.
	 * @return Mutable span of pixels.
	 */
	inline std::span<PixelType> getPixels() { return pixels; }
	/**
	 * @brief Returns read-only access to the raw pixel buffer.
	 * @return Const span of pixels.
	 */
	inline std::span<const PixelType> getPixels() const { return pixels; }

	/**
	 * @brief Creates a lightweight non-owning image view.
	 * @return ImageView describing this image buffer.
	 */
	inline ImageView toImageView() const {
		return {
			.data = pixels.data(),
			.width = dimensions.width,
			.height = dimensions.height,
			.stride = dimensions.stride,
			.format = PixelType::FMT_ID
		};
	}

	/**
	 * @brief Returns image dimensions and cached sampling values.
	 * @return Const reference to dimensions.
	 */
	inline const ImageDimensions& getDimensions() const {
		return dimensions;
	}

	/**
	 * @brief Returns the allocator resource used by the pixel storage.
	 * @return Polymorphic memory resource pointer.
	 */
	inline std::pmr::memory_resource* getMemoryResource() const {
		return pixels.get_allocator().resource();
	}

	/**
	 * @brief Writes a pixel at the given coordinate.
	 * @param pos Pixel position.
	 * @param pix Pixel value in storage format.
	 */
	void setPixel(const glm::uvec2& pos, PixelType pix) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)] = pix;
	}

	/**
	 * @brief Writes a pixel converted from a colour kernel value.
	 * @param pos Pixel position.
	 * @param colourKernel Colour value in kernel representation.
	 */
	void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)].fromKernel(colourKernel);
	}

	/**
	 * @brief Writes a pixel from kernel colour with optional quantization dithering.
	 * @param pos Pixel position.
	 * @param colourKernel Colour value in kernel representation.
	 */
	void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)].fromKernelDithered(colourKernel, pos);
	}

	/**
	 * @brief Returns this image pixel format.
	 * @return Pixel format identifier.
	 */
	Format getFormat() const {
		return PixelType::FMT_ID;
	}

	/**
	 * @brief Samples a pixel at an integer coordinate using wrap mode.
	 * @param pos Input pixel coordinate.
	 * @param colourKernel Output colour in kernel representation.
	 * @param wrap Wrapping mode for out-of-range coordinates.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
		const unsigned x = detail::wrapCoordinate(pos.x, dimensions.width, wrap);
		const unsigned y = detail::wrapCoordinate(pos.y, dimensions.height, wrap);
		pixels[toLinearIndex(dimensions.width, x, y)].toKernel(colourKernel);
	}

	/**
	 * @brief Iterates over all pixels and invokes a callback with decoded colour values.
	 * @tparam F Callback type constrained by ColourIteratorConcept.
	 * @param program Callback to invoke for each pixel.
	 */
	template <typename F> requires ColourIteratorConcept<F>
	void iterateOverPixels(F&& program) const {
		Elv::Util::over_2d_span<PixelType>(pixels, [&](const PixelType& px, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			px.toKernel(kernel);
			detail::invokeIterator(program, pos, kernel, dimensions.widthR, dimensions.heightR);
		}, glm::uvec2(dimensions.width, dimensions.height));
	}

	/**
	 * @brief Iterates over a sub-rectangle and invokes a callback with decoded colours.
	 * @tparam F Callback type constrained by ColourIteratorConcept.
	 * @param program Callback to invoke for each pixel.
	 * @param offset Top-left region offset.
	 * @param affectedDimensions Region size.
	 */
	template <typename F> requires ColourIteratorConcept<F>
	void iterateOverPixels(F&& program, const glm::uvec2& offset, const glm::uvec2& affectedDimensions) const {
		Elv::Util::over_2d_span<PixelType>(pixels, [&](const PixelType& px, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			px.toKernel(kernel);
			detail::invokeIterator(program, pos, kernel, dimensions.widthR, dimensions.heightR);
		}, glm::uvec2(dimensions.width, dimensions.height), offset, affectedDimensions);
	}

	/**
	 * @brief Fills the whole image with a constant kernel colour.
	 * @param colourKernel Fill colour in kernel representation.
	 * @param dither Enables pixel-position-dependent dithering.
	 */
	void clearToColour(const glm::fvec4& colourKernel, bool dither) {
		if(dither) {
			Elv::Util::over_2d_span_mut<PixelType>(pixels, [&](PixelType& px, const glm::uvec2& pos) {
				px.fromKernelDithered(colourKernel, pos);
			}, glm::uvec2(dimensions.width, dimensions.height));
		} else {
			PixelType source;
			source.fromKernel(colourKernel);
			std::fill(pixels.begin(), pixels.end(), source);
		}
	}

	/**
	 * @brief Fills the whole image using a generated colour program.
	 * @tparam F Generator type constrained by ColourProgramConcept.
	 * @param program Colour generator callback.
	 * @param dither Enables pixel-position-dependent dithering.
	 */
	template <typename F> requires ColourProgramConcept<F>
	void clearToColour(F&& program, bool dither) {
		clearToColourImpl(
			pixels,
			dimensions.width,
			dimensions.height,
			dimensions.widthR,
			dimensions.heightR,
			std::forward<F>(program),
			glm::uvec2(0, 0),
			glm::uvec2(dimensions.width, dimensions.height),
			dither);
	}

	/**
	 * @brief Fills a sub-rectangle with a constant kernel colour.
	 * @param colourKernel Fill colour in kernel representation.
	 * @param offset Top-left region offset.
	 * @param affectedDimensions Region size.
	 * @param dither Enables pixel-position-dependent dithering.
	 */
	void clearToColour(const glm::fvec4& colourKernel, const glm::uvec2& offset, const glm::uvec2& affectedDimensions, bool dither) {
		if(dither) {
			Elv::Util::over_2d_span_mut<PixelType>(pixels, [&](PixelType& px, const glm::uvec2& pos) {
				px.fromKernelDithered(colourKernel, pos);
			}, glm::uvec2(dimensions.width, dimensions.height), offset, affectedDimensions);
		} else {
			PixelType source;
			source.fromKernel(colourKernel);
			Elv::Util::over_2d_span_mut<PixelType>(pixels, [source](PixelType& px, const glm::uvec2&) {
				px = source;
			}, glm::uvec2(dimensions.width, dimensions.height), offset, affectedDimensions);
		}
	}

	/**
	 * @brief Fills a sub-rectangle using a generated colour program.
	 * @tparam F Generator type constrained by ColourProgramConcept.
	 * @param program Colour generator callback.
	 * @param offset Top-left region offset.
	 * @param affectedDimensions Region size.
	 * @param dither Enables pixel-position-dependent dithering.
	 */
	template <typename F> requires ColourProgramConcept<F>
	void clearToColour(F&& program, const glm::uvec2& offset, const glm::uvec2& affectedDimensions, bool dither) {
		clearToColourImpl(
			pixels,
			dimensions.width,
			dimensions.height,
			dimensions.widthR,
			dimensions.heightR,
			std::forward<F>(program),
			offset,
			affectedDimensions,
			dither);
	}

	/**
	 * @brief Resizes the image preserving overlapping content.
	 * @param newWidth New width in pixels.
	 * @param newHeight New height in pixels.
	 */
	void resize(unsigned newWidth, unsigned newHeight) {
		std::pmr::vector<PixelType> newPixels(pixels.get_allocator().resource());
		newPixels.resize(static_cast<size_t>(newWidth) * static_cast<size_t>(newHeight));

		const unsigned copyWidth = std::min(dimensions.width, newWidth);
		const unsigned copyHeight = std::min(dimensions.height, newHeight);

		for(unsigned y = 0; y < copyHeight; ++y) {
			PixelType* dst = &newPixels[y * newWidth];
			const PixelType* src = &pixels[y * dimensions.width];
			if constexpr (std::is_trivially_copyable_v<PixelType>) {
				std::memcpy(dst, src, static_cast<size_t>(copyWidth) * sizeof(PixelType));
			} else {
				std::copy_n(src, copyWidth, dst);
			}
		}

		pixels = std::move(newPixels);
		dimensions.width = newWidth;
		dimensions.height = newHeight;
		dimensions.recalculateStride(PixelType::FMT_ID);
		dimensions.recalculateFloats();
	}

	/**
	 * @brief Samples this image as a texture using the generic sampler.
	 * @param pos Normalized texture coordinate in [0,1] space.
	 * @param screenpos Screen-space pixel coordinate used by dithered modes.
	 * @param colourKernel Output colour in kernel representation.
	 * @param filteringType Filtering algorithm to apply.
	 * @param wrap Wrapping mode for out-of-range coordinates.
	 */
	void sampleTexture(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap) const {
		Euph::Media::Image::sampleTexture(*this, pos, screenpos, colourKernel, filteringType, wrap);
	}
};

/**
 * @class PalettedImage2D
 * @brief Owns an indexed image buffer with an external typed palette.
 * @tparam PixelType Palette entry type.
 */
template <PixelConcept PixelType> class PalettedImage2D {
public:
	/** @brief Palette storage type used by paletted images. */
	using Palette = std::array<PixelType, 256>;

private:
	std::pmr::vector<uint8_t> pixels;
	std::shared_ptr<const Palette> palette;
	int transparentColorIndex;
	ImageDimensions dimensions;

public:
	/**
	 * @brief Constructs an empty paletted image.
	 * @param memResource Polymorphic allocator resource.
	 */
	explicit PalettedImage2D(std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), palette(), transparentColorIndex(-1), dimensions{0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f} {}

	/**
	 * @brief Constructs a paletted image with opaque palette entries.
	 * @param width Width in pixels.
	 * @param height Height in pixels.
	 * @param paletteData Shared palette data.
	 * @param memResource Polymorphic allocator resource.
	 */
	PalettedImage2D(unsigned width, unsigned height, std::shared_ptr<const Palette> paletteData, std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: PalettedImage2D(width, height, std::move(paletteData), -1, memResource) {}

	/**
	 * @brief Constructs a paletted image with explicit transparent palette index.
	 * @param width Width in pixels.
	 * @param height Height in pixels.
	 * @param paletteData Shared palette data.
	 * @param transparentIndex Transparent index in [0,255], or negative for none.
	 * @param memResource Polymorphic allocator resource.
	 */
	PalettedImage2D(unsigned width, unsigned height, std::shared_ptr<const Palette> paletteData, int transparentIndex, std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), palette(std::move(paletteData)), transparentColorIndex(transparentIndex), dimensions{width, height, 0, 0.0f, 0.0f, 0.0f, 0.0f} {
		dimensions.recalculateStride(Format::INDEXED);
		dimensions.recalculateFloats();
		pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
	}

	/** @brief Documents the PalettedImage2D type or declaration. */
	PalettedImage2D(const PalettedImage2D&) = default;
	/** @brief Documents the PalettedImage2D type or declaration. */
	PalettedImage2D(PalettedImage2D&&) noexcept = default;
	/** @brief Documents the operator= helper. */
	PalettedImage2D& operator=(const PalettedImage2D&) = default;
	/** @brief Documents the operator= helper. */
	PalettedImage2D& operator=(PalettedImage2D&&) noexcept = default;

	/**
	 * @brief Copy-constructs this image into a new allocator resource.
	 * @param other Source image.
	 * @param memResource Destination allocator resource.
	 */
	PalettedImage2D(const PalettedImage2D& other, std::pmr::memory_resource* memResource)
		: pixels(other.pixels, memResource), palette(other.palette), dimensions(other.dimensions) {}

	/**
	 * @brief Move-constructs this image into a new allocator resource.
	 * @param other Source image.
	 * @param memResource Destination allocator resource.
	 */
	PalettedImage2D(PalettedImage2D&& other, std::pmr::memory_resource* memResource)
		: pixels(std::move(other.pixels), memResource), palette(std::move(other.palette)), dimensions(other.dimensions) {}

	/**
	 * @brief Creates a non-owning view over the index buffer.
	 * @return ImageView describing indexed pixel data.
	 */
	inline ImageView toImageView() const {
		return {
			.data = pixels.data(),
			.width = dimensions.width,
			.height = dimensions.height,
			.stride = dimensions.stride,
			.format = Format::INDEXED // This view describes the index buffer only.
		};
	}
	/**
	 * @brief Returns mutable access to palette index data.
	 * @return Mutable span of indices.
	 */
	inline std::span<uint8_t> getPixels() { return pixels; }
	/**
	 * @brief Returns read-only access to palette index data.
	 * @return Const span of indices.
	 */
	inline std::span<const uint8_t> getPixels() const { return pixels; }

	/**
	 * @brief Returns image dimensions and cached sampling values.
	 * @return Const reference to dimensions.
	 */
	inline const ImageDimensions& getDimensions() const {
		return dimensions;
	}

	/**
	 * @brief Returns the allocator resource used by index storage.
	 * @return Polymorphic memory resource pointer.
	 */
	inline std::pmr::memory_resource* getMemoryResource() const {
		return pixels.get_allocator().resource();
	}

	/**
	 * @brief Gets the shared palette pointer.
	 * @return Shared pointer to const palette data.
	 */
	inline std::shared_ptr<const Palette> getPalette() const {
		return palette;
	}

	/**
	 * @brief Replaces the current palette.
	 * @param newPalette New shared palette pointer.
	 */
	inline void setPalette(std::shared_ptr<const Palette> newPalette) {
		palette = std::move(newPalette);
	}

	/**
	 * @brief Returns the configured transparent palette index.
	 * @return Transparent index, or a negative value when disabled.
	 */
	inline int getTransparentColorIndex() const {
		return transparentColorIndex;
	}

	/**
	 * @brief Updates the transparent palette index.
	 * @param newTransparentColorIndex Transparent index, or negative to disable.
	 */
	inline void setTransparentColorIndex(int newTransparentColorIndex) {
		transparentColorIndex = newTransparentColorIndex;
	}

	/**
	 * @brief Writes a palette index at the given pixel coordinate.
	 * @param pos Pixel position.
	 * @param paletteIndex Palette index value.
	 */
	void setPixelIndex(const glm::uvec2& pos, uint8_t paletteIndex) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)] = paletteIndex;
	}

	/**
	 * @brief Reads a palette index with wrap handling.
	 * @param pos Input pixel coordinate.
	 * @param wrap Wrapping mode for out-of-range coordinates.
	 * @return Palette index at the wrapped coordinate.
	 */
	uint8_t getPixelIndex(const glm::uvec2& pos, Wrap wrap) const {
		const unsigned x = detail::wrapCoordinate(pos.x, dimensions.width, wrap);
		const unsigned y = detail::wrapCoordinate(pos.y, dimensions.height, wrap);
		return pixels[toLinearIndex(dimensions.width, x, y)];
	}

	/**
	 * @brief Samples a decoded RGBA kernel colour from indexed storage.
	 * @param pos Input pixel coordinate.
	 * @param colourKernel Output colour in kernel representation.
	 * @param wrap Wrapping mode for out-of-range coordinates.
	 */
	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
		if(!palette) {
			colourKernel = glm::fvec4(0.0f);
			return;
		}
		const uint8_t index = getPixelIndex(pos, wrap);
		(*palette)[index].toKernel(colourKernel);
		if(transparentColorIndex >= 0 && transparentColorIndex <= 255 && index == static_cast<uint8_t>(transparentColorIndex)) {
			colourKernel.a = 0.0f;
		}
	}

	/**
	 * @brief Iterates over all decoded pixel colours.
	 * @tparam F Callback type constrained by ColourIteratorConcept.
	 * @param program Callback to invoke for each pixel.
	 */
	template <typename F> requires ColourIteratorConcept<F>
	void iterateOverPixels(F&& program) const {
		if(!palette) {
			return;
		}
		Elv::Util::over_2d_span<uint8_t>(pixels, [&](const uint8_t& index, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			(*palette)[index].toKernel(kernel);
			detail::invokeIterator(program, pos, kernel, dimensions.widthR, dimensions.heightR);
		}, glm::uvec2(dimensions.width, dimensions.height));
	}

	/**
	 * @brief Iterates over a decoded sub-rectangle.
	 * @tparam F Callback type constrained by ColourIteratorConcept.
	 * @param program Callback to invoke for each pixel.
	 * @param offset Top-left region offset.
	 * @param affectedDimensions Region size.
	 */
	template <typename F> requires ColourIteratorConcept<F>
	void iterateOverPixels(F&& program, const glm::uvec2& offset, const glm::uvec2& affectedDimensions) const {
		if(!palette) {
			return;
		}
		Elv::Util::over_2d_span<uint8_t>(pixels, [&](const uint8_t& index, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			(*palette)[index].toKernel(kernel);
			detail::invokeIterator(program, pos, kernel, dimensions.widthR, dimensions.heightR);
		}, glm::uvec2(dimensions.width, dimensions.height), offset, affectedDimensions);
	}

	/**
	 * @brief Resizes indexed storage preserving overlapping content.
	 * @param newWidth New width in pixels.
	 * @param newHeight New height in pixels.
	 */
	void resize(unsigned newWidth, unsigned newHeight) {
		std::pmr::vector<uint8_t> newPixels(pixels.get_allocator().resource());
		newPixels.resize(static_cast<size_t>(newWidth) * static_cast<size_t>(newHeight));

		const unsigned copyWidth = std::min(dimensions.width, newWidth);
		const unsigned copyHeight = std::min(dimensions.height, newHeight);

		for(unsigned y = 0; y < copyHeight; ++y) {
			std::memcpy(&newPixels[y * newWidth], &pixels[y * dimensions.width], copyWidth * sizeof(uint8_t));
		}

		pixels = std::move(newPixels);
		dimensions.width = newWidth;
		dimensions.height = newHeight;
		dimensions.recalculateStride(Format::INDEXED);
		dimensions.recalculateFloats();
	}

	/**
	 * @brief Samples this indexed image as a texture using generic filtering.
	 * @param pos Normalized texture coordinate in [0,1] space.
	 * @param screenpos Screen-space pixel coordinate used by dithered modes.
	 * @param colourKernel Output colour in kernel representation.
	 * @param filteringType Filtering algorithm to apply.
	 * @param wrap Wrapping mode for out-of-range coordinates.
	 */
	void sampleTexture(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap) const {
		Euph::Media::Image::sampleTexture(*this, pos, screenpos, colourKernel, filteringType, wrap);
	}

	/**
	 * @brief Expands indexed storage into a fully de-palettized image.
	 * @param memResource Optional destination allocator resource.
	 * @return A new unpacked image.
	 */
	Image2D<PixelType> depalettize(std::pmr::memory_resource* memResource = std::pmr::get_default_resource()) const {
		Image2D<PixelType> out(dimensions.width, dimensions.height, memResource);
		if(!palette) {
			return out;
		}

		Elv::Util::over_2d_span<uint8_t>(pixels, [&](const uint8_t& index, const glm::uvec2& pos) {
			out.setPixel(pos, (*palette)[index]);
		}, glm::uvec2(dimensions.width, dimensions.height));
		return out;
	}
};

/**
 * @brief Samples an image using one of the supported texture filtering modes.
 * @tparam ImageType Type satisfying TextureTypeConcept.
 * @param image Source image object.
 * @param pos Normalized texture coordinate in [0,1] space.
 * @param screenpos Screen-space pixel coordinate used by dithered modes.
 * @param colourKernel Output colour in kernel representation.
 * @param filteringType Filtering mode selector.
 * @param wrap Wrapping mode for out-of-range coordinates.
 */
template <TextureTypeConcept ImageType>
inline void sampleTexture(const ImageType& image, const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap) {
	const ImageDimensions& dims = image.getDimensions();
	if(dims.width == 0 || dims.height == 0) {
		colourKernel = glm::fvec4(0.0f);
		return;
	}

	switch(filteringType) {
		case TextureFiltering::NEAREST_NEIGHBOUR: {
			const glm::uvec2 texel(
				static_cast<unsigned>(std::round(pos.x * dims.widthF)),
				static_cast<unsigned>(std::round(pos.y * dims.heightF)));
			image.getPixel(texel, colourKernel, wrap);
			break;
		}
		case TextureFiltering::DITHERED: {
			glm::fvec2 texelCoords(pos.x * dims.widthF, pos.y * dims.heightF);
			texelCoords += Elv::Util::LOOKUP[screenpos[1] & 1][screenpos[0] & 1];
			image.getPixel(glm::uvec2(
							   static_cast<unsigned>(std::round(texelCoords.x)),
							   static_cast<unsigned>(std::round(texelCoords.y))), colourKernel, wrap);
			break;
		}
		case TextureFiltering::BILINEAR: {
			const glm::fvec2 tmp(pos.x * dims.widthF, pos.y * dims.heightF);
			const float xW = tmp.x - std::floor(tmp.x);
			const float yW = tmp.y - std::floor(tmp.y);
			const glm::uvec2 coords[4] = {
				glm::uvec2(static_cast<unsigned>(std::floor(tmp.x)), static_cast<unsigned>(std::floor(tmp.y))),
				glm::uvec2(static_cast<unsigned>(std::ceil(tmp.x)), static_cast<unsigned>(std::floor(tmp.y))),
				glm::uvec2(static_cast<unsigned>(std::floor(tmp.x)), static_cast<unsigned>(std::ceil(tmp.y))),
				glm::uvec2(static_cast<unsigned>(std::ceil(tmp.x)), static_cast<unsigned>(std::ceil(tmp.y)))
			};
			const float weights[4] = {
				(1.0f - xW) * (1.0f - yW),
				xW * (1.0f - yW),
				(1.0f - xW) * yW,
				xW * yW
			};

			glm::fvec4 colours[4];
			for(int i = 0; i < 4; ++i) {
				image.getPixel(coords[i], colours[i], wrap);
				colours[i] *= weights[i];
			}
			colourKernel = colours[0] + colours[1] + colours[2] + colours[3];
			break;
		}
		case TextureFiltering::THREE_POINT: {
			const glm::fvec2 tmp(pos.x * dims.widthF, pos.y * dims.heightF);
			const glm::fvec2 edgeTopLeft(std::floor(tmp.x), std::floor(tmp.y));
			const glm::fvec2 edgeTopRight(std::ceil(tmp.x), std::floor(tmp.y));
			const glm::fvec2 edgeBottomLeft(std::floor(tmp.x), std::ceil(tmp.y));
			const glm::fvec2 edgeBottomRight(std::ceil(tmp.x), std::ceil(tmp.y));
			const bool useBottomRight = ((tmp.y - edgeTopLeft.y) * (tmp.x - edgeTopLeft.x)) >= 0.25f;
			const glm::fvec2& thirdPosition = useBottomRight ? edgeBottomRight : edgeTopLeft;

			glm::fvec4 colourEdgeTopRight, colourEdgeBottomLeft, thirdColour;
			image.getPixel(glm::uvec2(static_cast<unsigned>(edgeTopRight.x), static_cast<unsigned>(edgeTopRight.y)), colourEdgeTopRight, wrap);
			image.getPixel(glm::uvec2(static_cast<unsigned>(edgeBottomLeft.x), static_cast<unsigned>(edgeBottomLeft.y)), colourEdgeBottomLeft, wrap);
			image.getPixel(glm::uvec2(static_cast<unsigned>(thirdPosition.x), static_cast<unsigned>(thirdPosition.y)), thirdColour, wrap);

			if(useBottomRight) {
				const float w0 = Elv::Util::edgeFunction<float>(edgeBottomLeft, thirdPosition, tmp);
				const float w1 = Elv::Util::edgeFunction<float>(thirdPosition, edgeTopRight, tmp);
				const float w2 = Elv::Util::edgeFunction<float>(edgeTopRight, edgeBottomLeft, tmp);
				colourKernel = (colourEdgeTopRight * w0) + (colourEdgeBottomLeft * w1) + (thirdColour * w2);
			} else {
				const float w0 = Elv::Util::edgeFunction<float>(edgeBottomLeft, edgeTopRight, tmp);
				const float w1 = Elv::Util::edgeFunction<float>(edgeTopRight, thirdPosition, tmp);
				const float w2 = Elv::Util::edgeFunction<float>(thirdPosition, edgeBottomLeft, tmp);
				colourKernel = (thirdColour * w0) + (colourEdgeBottomLeft * w1) + (colourEdgeTopRight * w2);
			}
			break;
		}
	}
}

/** @brief Type trait that reports whether a type is a PalettedImage2D specialization. */
template <typename T> struct is_paletted_image : std::false_type {};
/** @brief Specialized type trait for paletted image specializations. */
template <typename P> struct is_paletted_image<PalettedImage2D<P>> : std::true_type {};
/** @brief Convenience variable template for `is_paletted_image<T>::value`. */
template <typename T> inline constexpr bool is_paletted_image_v = is_paletted_image<T>::value;

/**
 * @brief Runtime variant over all supported concrete image container types.
 */
using AnyImage2D = std::variant<
	Image2D<PixelGreyscale_U8>,
	Image2D<PixelGreyscale_U16>,
	Image2D<PixelGreyscale_U32>,
	Image2D<PixelGreyscale_S8>,
	Image2D<PixelGreyscale_S16>,
	Image2D<PixelGreyscale_S32>,
	Image2D<PixelGreyscale_F16>,
	Image2D<PixelGreyscale_F32>,
	Image2D<PixelGreyscale_F64>,
	Image2D<PixelRG_U8>,
	Image2D<PixelRG_U16>,
	Image2D<PixelRG_U32>,
	Image2D<PixelRG_S8>,
	Image2D<PixelRG_S16>,
	Image2D<PixelRG_S32>,
	Image2D<PixelRG_F16>,
	Image2D<PixelRG_F32>,
	Image2D<PixelRG_F64>,
	Image2D<PixelRGB_U8>,
	Image2D<PixelRGB_U16>,
	Image2D<PixelRGB_U32>,
	Image2D<PixelRGB_S8>,
	Image2D<PixelRGB_S16>,
	Image2D<PixelRGB_S32>,
	Image2D<PixelRGB_F16>,
	Image2D<PixelRGB_F32>,
	Image2D<PixelRGB_F64>,
	Image2D<PixelBGR_U8>,
	Image2D<PixelBGR_U16>,
	Image2D<PixelBGR_U32>,
	Image2D<PixelBGR_S8>,
	Image2D<PixelBGR_S16>,
	Image2D<PixelBGR_S32>,
	Image2D<PixelBGR_F16>,
	Image2D<PixelBGR_F32>,
	Image2D<PixelBGR_F64>,
	Image2D<PixelRGBA_U8>,
	Image2D<PixelRGBA_U16>,
	Image2D<PixelRGBA_U32>,
	Image2D<PixelRGBA_S8>,
	Image2D<PixelRGBA_S16>,
	Image2D<PixelRGBA_S32>,
	Image2D<PixelRGBA_F16>,
	Image2D<PixelRGBA_F32>,
	Image2D<PixelRGBA_F64>,
	Image2D<PixelBGRA_U8>,
	Image2D<PixelBGRA_U16>,
	Image2D<PixelBGRA_U32>,
	Image2D<PixelBGRA_S8>,
	Image2D<PixelBGRA_S16>,
	Image2D<PixelBGRA_S32>,
	Image2D<PixelBGRA_F16>,
	Image2D<PixelBGRA_F32>,
	Image2D<PixelBGRA_F64>,
	Image2D<PixelARGB_U8>,
	Image2D<PixelARGB_U16>,
	Image2D<PixelARGB_U32>,
	Image2D<PixelARGB_S8>,
	Image2D<PixelARGB_S16>,
	Image2D<PixelARGB_S32>,
	Image2D<PixelARGB_F16>,
	Image2D<PixelARGB_F32>,
	Image2D<PixelARGB_F64>,
	Image2D<PixelRGB444>,
	Image2D<PixelRGB555>,
	Image2D<PixelRGB565>,
	PalettedImage2D<PixelRGB_U8>,
	PalettedImage2D<PixelRGB_U16>,
	PalettedImage2D<PixelRGB_U32>,
	PalettedImage2D<PixelRGB_S8>,
	PalettedImage2D<PixelRGB_S16>,
	PalettedImage2D<PixelRGB_S32>,
	PalettedImage2D<PixelRGB_F16>,
	PalettedImage2D<PixelRGB_F32>,
	PalettedImage2D<PixelRGB_F64>,
	PalettedImage2D<PixelBGR_U8>,
	PalettedImage2D<PixelBGR_U16>,
	PalettedImage2D<PixelBGR_U32>,
	PalettedImage2D<PixelBGR_S8>,
	PalettedImage2D<PixelBGR_S16>,
	PalettedImage2D<PixelBGR_S32>,
	PalettedImage2D<PixelBGR_F16>,
	PalettedImage2D<PixelBGR_F32>,
	PalettedImage2D<PixelBGR_F64>,
	PalettedImage2D<PixelRGBA_U8>,
	PalettedImage2D<PixelRGBA_U16>,
	PalettedImage2D<PixelRGBA_U32>,
	PalettedImage2D<PixelRGBA_S8>,
	PalettedImage2D<PixelRGBA_S16>,
	PalettedImage2D<PixelRGBA_S32>,
	PalettedImage2D<PixelRGBA_F16>,
	PalettedImage2D<PixelRGBA_F32>,
	PalettedImage2D<PixelRGBA_F64>,
	PalettedImage2D<PixelBGRA_U8>,
	PalettedImage2D<PixelBGRA_U16>,
	PalettedImage2D<PixelBGRA_U32>,
	PalettedImage2D<PixelBGRA_S8>,
	PalettedImage2D<PixelBGRA_S16>,
	PalettedImage2D<PixelBGRA_S32>,
	PalettedImage2D<PixelBGRA_F16>,
	PalettedImage2D<PixelBGRA_F32>,
	PalettedImage2D<PixelBGRA_F64>,
	PalettedImage2D<PixelARGB_U8>,
	PalettedImage2D<PixelARGB_U16>,
	PalettedImage2D<PixelARGB_U32>,
	PalettedImage2D<PixelARGB_S8>,
	PalettedImage2D<PixelARGB_S16>,
	PalettedImage2D<PixelARGB_S32>,
	PalettedImage2D<PixelARGB_F16>,
	PalettedImage2D<PixelARGB_F32>,
	PalettedImage2D<PixelARGB_F64>,
	PalettedImage2D<PixelRGB444>,
	PalettedImage2D<PixelRGB555>,
	PalettedImage2D<PixelRGB565>
>;

/**
 * @brief Converts an unpacked image into a DecodeTarget.
 * @tparam PixelType Pixel storage type.
 * @param source Source image to copy.
 * @param memRes Memory resource for the resulting DecodeTarget.
 * @return DecodeTarget with one frame and matching format.
 */
template <PixelConcept PixelType>
inline DecodeTarget fromImageToDecodeTarget(const Image2D<PixelType>& source, std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) {
	DecodeTarget out(memRes);
	out.setFormat(PixelType::FMT_ID);
	out.setIsAnimated(false);
	out.setDelayTime(std::nullopt);

	const ImageDimensions& dims = source.getDimensions();
	Frame& frame = out.addFrame(dims.width, dims.height);
	const auto pixels = source.getPixels();
	const size_t copyBytes = pixels.size() * sizeof(PixelType);
	if(copyBytes > 0) {
		std::memcpy(frame.data.data(), pixels.data(), copyBytes);
	}
	return out;
}

/**
 * @brief Converts an indexed image into a DecodeTarget, including palette when present.
 * @tparam PixelType Palette entry storage type.
 * @param source Source indexed image to copy.
 * @param memRes Memory resource for the resulting DecodeTarget.
 * @return DecodeTarget with one indexed frame and optional palette.
 */
template <PixelConcept PixelType>
inline DecodeTarget fromImageToDecodeTarget(const PalettedImage2D<PixelType>& source, std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) {
	DecodeTarget out(memRes);
	out.setFormat(Format::INDEXED);
	out.setIsAnimated(false);
	out.setDelayTime(std::nullopt);

	const ImageDimensions& dims = source.getDimensions();
	Frame& frame = out.addFrame(dims.width, dims.height);
	const auto pixels = source.getPixels();
	const size_t copyBytes = pixels.size() * sizeof(uint8_t);
	if(copyBytes > 0) {
		std::memcpy(frame.data.data(), pixels.data(), copyBytes);
	}

	const auto palette = source.getPalette();
	if(palette) {
		Palette& outPalette = out.createPalette(PixelType::FMT_ID, 256, source.getTransparentColorIndex());
		const size_t paletteBytes = palette->size() * sizeof(PixelType);
		if(paletteBytes > 0) {
			std::memcpy(outPalette.data.data(), palette->data(), paletteBytes);
		}
	}

	return out;
}

/**
 * @brief Converts a runtime image variant into a DecodeTarget.
 * @param source Source image variant.
 * @param memRes Memory resource for the resulting DecodeTarget.
 * @return DecodeTarget produced from the active variant alternative.
 */
inline DecodeTarget fromImageToDecodeTarget(const AnyImage2D& source, std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) {
	return std::visit([memRes](const auto& image) {
		return fromImageToDecodeTarget(image, memRes);
	}, source);
}

/**
 * @brief Converts a DecodeTarget into the corresponding runtime image variant.
 * @param source Source decode target.
 * @param memRes Memory resource used by the returned image container.
 * @return Converted image on success, or `std::nullopt` when format conversion fails.
 */
std::optional<AnyImage2D> fromDecodeTargetToImage(const DecodeTarget& source, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

} // namespace Image
} // namespace Media
} // namespace Euph

#endif // EUPHIMAGE_HPP
