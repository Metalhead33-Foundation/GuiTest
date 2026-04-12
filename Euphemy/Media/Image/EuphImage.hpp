#ifndef EUPHIMAGE_HPP
#define EUPHIMAGE_HPP

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

struct ImageDimensions {
	unsigned width;
	unsigned height;
	unsigned stride;
	float widthF;   // width - 1 (maximum texel X coordinate)
	float heightF;  // height - 1 (maximum texel Y coordinate)
	float widthR;   // reciprocal of widthF
	float heightR;  // reciprocal of heightF

	inline void recalculateStride(Format format) {
		stride = width * pixelByteSize(format);
	}

	inline void recalculateFloats() {
		widthF = (width > 1) ? static_cast<float>(width - 1) : 0.0f;
		heightF = (height > 1) ? static_cast<float>(height - 1) : 0.0f;
		widthR = (widthF > 0.0f) ? (1.0f / widthF) : 0.0f;
		heightR = (heightF > 0.0f) ? (1.0f / heightF) : 0.0f;
	}
};

template <typename T>
concept TextureTypeConcept =
	requires(const T& image, const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) {
		{ image.getDimensions() } -> std::same_as<const ImageDimensions&>;
		{ image.getPixel(pos, colourKernel, wrap) } -> std::same_as<void>;
	};

template <typename F>
concept ColourProgramConcept =
	std::is_invocable_v<std::remove_reference_t<F>, glm::uvec2> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::uvec2, glm::fvec4> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::fvec2> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::fvec2, glm::fvec4>;

template <typename F>
concept ColourIteratorConcept =
	std::is_invocable_v<std::remove_reference_t<F>, glm::uvec2, glm::fvec4> ||
	std::is_invocable_v<std::remove_reference_t<F>, glm::fvec2, glm::fvec4>;

template <TextureTypeConcept ImageType>
inline void sampleTexture(const ImageType& image, const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap);

namespace detail {

template <typename> inline constexpr bool always_false_v = false;

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
	explicit Image2D(std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), dimensions{0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f} {}

	Image2D(unsigned width, unsigned height, std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), dimensions{width, height, 0, 0.0f, 0.0f, 0.0f, 0.0f} {
		dimensions.recalculateStride(PixelType::FMT_ID);
		dimensions.recalculateFloats();
		pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
	}

	Image2D(const Image2D&) = default;
	Image2D(Image2D&&) noexcept = default;
	Image2D& operator=(const Image2D&) = default;
	Image2D& operator=(Image2D&&) noexcept = default;

	Image2D(const Image2D& other, std::pmr::memory_resource* memResource)
		: pixels(other.pixels, memResource), dimensions(other.dimensions) {}

	Image2D(Image2D&& other, std::pmr::memory_resource* memResource)
		: pixels(std::move(other.pixels), memResource), dimensions(other.dimensions) {}

	inline std::span<PixelType> getPixels() { return pixels; }
	inline std::span<const PixelType> getPixels() const { return pixels; }

	inline ImageView toImageView() const {
		return {
			.data = pixels.data(),
			.width = dimensions.width,
			.height = dimensions.height,
			.stride = dimensions.stride,
			.format = PixelType::FMT_ID
		};
	}

	inline const ImageDimensions& getDimensions() const {
		return dimensions;
	}

	inline std::pmr::memory_resource* getMemoryResource() const {
		return pixels.get_allocator().resource();
	}

	void setPixel(const glm::uvec2& pos, PixelType pix) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)] = pix;
	}

	void setPixel(const glm::uvec2& pos, const glm::fvec4& colourKernel) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)].fromKernel(colourKernel);
	}

	void setPixelDithered(const glm::uvec2& pos, const glm::fvec4& colourKernel) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)].fromKernelDithered(colourKernel, pos);
	}

	Format getFormat() const {
		return PixelType::FMT_ID;
	}

	void getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const {
		const unsigned x = detail::wrapCoordinate(pos.x, dimensions.width, wrap);
		const unsigned y = detail::wrapCoordinate(pos.y, dimensions.height, wrap);
		pixels[toLinearIndex(dimensions.width, x, y)].toKernel(colourKernel);
	}

	template <typename F> requires ColourIteratorConcept<F>
	void iterateOverPixels(F&& program) const {
		Elv::Util::over_2d_span<PixelType>(pixels, [&](const PixelType& px, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			px.toKernel(kernel);
			detail::invokeIterator(program, pos, kernel, dimensions.widthR, dimensions.heightR);
		}, glm::uvec2(dimensions.width, dimensions.height));
	}

	template <typename F> requires ColourIteratorConcept<F>
	void iterateOverPixels(F&& program, const glm::uvec2& offset, const glm::uvec2& affectedDimensions) const {
		Elv::Util::over_2d_span<PixelType>(pixels, [&](const PixelType& px, const glm::uvec2& pos) {
			glm::fvec4 kernel;
			px.toKernel(kernel);
			detail::invokeIterator(program, pos, kernel, dimensions.widthR, dimensions.heightR);
		}, glm::uvec2(dimensions.width, dimensions.height), offset, affectedDimensions);
	}

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

	void sampleTexture(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap) const {
		Euph::Media::Image::sampleTexture(*this, pos, screenpos, colourKernel, filteringType, wrap);
	}
};

template <PixelConcept PixelType> class PalettedImage2D {
public:
	using Palette = std::array<PixelType, 256>;

private:
	std::pmr::vector<uint8_t> pixels;
	std::shared_ptr<const Palette> palette;
	int transparentColorIndex;
	ImageDimensions dimensions;

public:
	explicit PalettedImage2D(std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), palette(), transparentColorIndex(-1), dimensions{0, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f} {}

	PalettedImage2D(unsigned width, unsigned height, std::shared_ptr<const Palette> paletteData, std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: PalettedImage2D(width, height, std::move(paletteData), -1, memResource) {}

	PalettedImage2D(unsigned width, unsigned height, std::shared_ptr<const Palette> paletteData, int transparentIndex, std::pmr::memory_resource* memResource = std::pmr::get_default_resource())
		: pixels(memResource), palette(std::move(paletteData)), transparentColorIndex(transparentIndex), dimensions{width, height, 0, 0.0f, 0.0f, 0.0f, 0.0f} {
		dimensions.recalculateStride(Format::INDEXED);
		dimensions.recalculateFloats();
		pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
	}

	PalettedImage2D(const PalettedImage2D&) = default;
	PalettedImage2D(PalettedImage2D&&) noexcept = default;
	PalettedImage2D& operator=(const PalettedImage2D&) = default;
	PalettedImage2D& operator=(PalettedImage2D&&) noexcept = default;

	PalettedImage2D(const PalettedImage2D& other, std::pmr::memory_resource* memResource)
		: pixels(other.pixels, memResource), palette(other.palette), dimensions(other.dimensions) {}

	PalettedImage2D(PalettedImage2D&& other, std::pmr::memory_resource* memResource)
		: pixels(std::move(other.pixels), memResource), palette(std::move(other.palette)), dimensions(other.dimensions) {}

	inline ImageView toImageView() const {
		return {
			.data = pixels.data(),
			.width = dimensions.width,
			.height = dimensions.height,
			.stride = dimensions.stride,
			.format = Format::INDEXED // This view describes the index buffer only.
		};
	}
	inline std::span<uint8_t> getPixels() { return pixels; }
	inline std::span<const uint8_t> getPixels() const { return pixels; }

	inline const ImageDimensions& getDimensions() const {
		return dimensions;
	}

	inline std::pmr::memory_resource* getMemoryResource() const {
		return pixels.get_allocator().resource();
	}

	inline std::shared_ptr<const Palette> getPalette() const {
		return palette;
	}

	inline void setPalette(std::shared_ptr<const Palette> newPalette) {
		palette = std::move(newPalette);
	}

	inline int getTransparentColorIndex() const {
		return transparentColorIndex;
	}

	inline void setTransparentColorIndex(int newTransparentColorIndex) {
		transparentColorIndex = newTransparentColorIndex;
	}

	void setPixelIndex(const glm::uvec2& pos, uint8_t paletteIndex) {
		pixels[toLinearIndex(dimensions.width, pos.x, pos.y)] = paletteIndex;
	}

	uint8_t getPixelIndex(const glm::uvec2& pos, Wrap wrap) const {
		const unsigned x = detail::wrapCoordinate(pos.x, dimensions.width, wrap);
		const unsigned y = detail::wrapCoordinate(pos.y, dimensions.height, wrap);
		return pixels[toLinearIndex(dimensions.width, x, y)];
	}

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

	void sampleTexture(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap) const {
		Euph::Media::Image::sampleTexture(*this, pos, screenpos, colourKernel, filteringType, wrap);
	}

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

template <typename T> struct is_paletted_image : std::false_type {};
template <typename P> struct is_paletted_image<PalettedImage2D<P>> : std::true_type {};
template <typename T> inline constexpr bool is_paletted_image_v = is_paletted_image<T>::value;

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

inline DecodeTarget fromImageToDecodeTarget(const AnyImage2D& source, std::pmr::memory_resource* memRes = std::pmr::get_default_resource()) {
	return std::visit([memRes](const auto& image) {
		return fromImageToDecodeTarget(image, memRes);
	}, source);
}

std::optional<AnyImage2D> fromDecodeTargetToImage(const DecodeTarget& source, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

} // namespace Image
} // namespace Media
} // namespace Euph

#endif // EUPHIMAGE_HPP
