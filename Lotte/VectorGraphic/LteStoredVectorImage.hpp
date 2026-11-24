#ifndef LTESTOREDVECTORIMAGE_HPP
#define LTESTOREDVECTORIMAGE_HPP
/**
 * @file LteStoredVectorImage.hpp
 * @brief Declares FontPacker-compatible preprocessed vector-image data for Lotte.
 *
 * @see https://github.com/Metalhead33-Foundation/FontPacker/blob/master/BINARY_FORMAT.md
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteFontFace.hpp>
#include <Elvavena/Io/ElvDataStream.hpp>
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Lotte {
/** @brief Standalone preprocessed vector-image APIs for Lotte. */
namespace VectorGraphic {

/** @brief Four-byte magic identifier for FontPacker stored vector-image files. */
inline constexpr std::array<char, 4> StoredVectorImageMagic = { 'W', 'O', 'D', 'I' };

/** @brief Current supported FontPacker stored vector-image format version. */
inline constexpr std::uint32_t StoredVectorImageVersion = 1;

/** @brief SDF channel layout shared with the FontPacker font format. */
using SDFType = Text::SDFType;

/** @brief Distance metric shared with the FontPacker font format. */
using DistanceType = Text::DistanceType;

/** @brief Null-terminated, zero-padded FontPacker image format tag. */
using ImageFormatTag = Text::ImageFormatTag;

/** @brief Bit values used by StoredVectorImage::encodingFlags (shared with `.wodf` version 2+). */
using EncodingFlag = Text::EncodingFlag;

/** @brief One encoded mipmap image payload. */
struct Mipmap {
	/** @brief Encoded image bytes for this mip level. */
	std::vector<std::byte> data;
};

/** @brief Complete FontPacker standalone vector-image SDF payload. */
struct StoredVectorImage {
	/** @brief FontPacker binary format version written after the WODI magic. */
	std::uint32_t version = StoredVectorImageVersion;
	/** @brief Generation square size `L`. The padding is inside it. */
	std::uint32_t processingSize = 0;
	/** @brief Level-0 stored square size `S`; the size of `mipmaps[0]`. Never 0 in a written file. */
	std::uint32_t actualSize = 0;
	/** @brief Padding `P` on all four sides, in pixels of the `L` grid. */
	std::uint32_t padding = 0;
	/** @brief Original vector canvas or viewBox X origin (currently always 0). */
	float logicalX = 0.0f;
	/** @brief Original vector canvas or viewBox Y origin (currently always 0). */
	float logicalY = 0.0f;
	/** @brief Original vector canvas or viewBox width. */
	float logicalWidth = 0.0f;
	/** @brief Original vector canvas or viewBox height. */
	float logicalHeight = 0.0f;
	/** @brief Ratio of logicalWidth to logicalHeight (`1.0` when the height is 0). */
	float aspectRatio = 0.0f;
	/** @brief Control-point bounding-box minimum X, in source coordinates (before normalization). */
	float minX = 0.0f;
	/** @brief Control-point bounding-box maximum X, in source coordinates (before normalization). */
	float maxX = 0.0f;
	/** @brief Control-point bounding-box minimum Y, in source coordinates (before normalization). */
	float minY = 0.0f;
	/** @brief Control-point bounding-box maximum Y, in source coordinates (before normalization). */
	float maxY = 0.0f;
	/** @brief Nominal encoded radius in X, in level-0 texels (`(samplesX / 2 or padding) * S / L`). */
	float distanceRangeX = 0.0f;
	/** @brief Nominal encoded radius in Y, in level-0 texels. */
	float distanceRangeY = 0.0f;
	/** @brief Bitmask of @ref EncodingFlag values baked into the pixels. */
	std::uint32_t encodingFlags = 0;
	/** @brief Baked midpoint divisor; meaningful only with `EncodingFlag::MidpointAdjusted`. */
	float midpointAdjustment = 1.0f;
	/** @brief SDF channel layout. */
	SDFType type = SDFType::SDF;
	/** @brief Distance metric used to generate the SDF. */
	DistanceType distType = DistanceType::Manhattan;
	/** @brief Null-terminated image format tag used by encoded mipmap payloads. */
	ImageFormatTag imageFormat = { 'P', 'N', 'G', '\0', '\0', '\0', '\0', '\0' };
	/** @brief Encoded mipmap image payloads; level 0 is `actualSize` square. */
	std::vector<Mipmap> mipmaps;

	/** @brief Returns whether a given @ref EncodingFlag bit is set. */
	bool hasEncodingFlag(EncodingFlag flag) const
	{
		return (encodingFlags & static_cast<std::uint32_t>(flag)) != 0;
	}

	/** @brief Normalizes and stores an image codec name (trimmed, upper-case, max 7 characters). */
	void setImageFormat(std::string_view format) { Text::setImageFormatTag(imageFormat, format); }

	/** @brief Returns the stored image codec name, without NUL padding. */
	std::string_view imageFormatName() const { return Text::imageFormatTagName(imageFormat); }

	/** @brief Padding expressed in level-0 texels: `P * S / L`, fractional unless P is a multiple of L/S. */
	float paddingInTexels() const
	{
		return processingSize ? static_cast<float>(padding) * static_cast<float>(actualSize)
									/ static_cast<float>(processingSize)
							  : 0.0f;
	}

	/** @brief Size of the whole logical canvas, in source units. */
	glm::fvec2 logicalSize() const { return glm::fvec2(logicalWidth, logicalHeight); }

	/** @brief Width and height of the source-space control-point bounding box. */
	glm::fvec2 sourceSize() const { return glm::fvec2(maxX - minX, maxY - minY); }

	/**
	 * @brief Maps a normalized texture coordinate back to source coordinates.
	 *
	 * The inner content box is `[P/L, (L-P)/L]` on both axes; the rest of the
	 * texture is the padding runway. Values outside `[0, 1]` are extrapolated.
	 * @param uv Texture coordinate in `[0, 1]`.
	 * @return Source-space position.
	 */
	glm::fvec2 uvToSource(const glm::fvec2& uv) const
	{
		if (!processingSize) return glm::fvec2(logicalX, logicalY);
		const float pad = static_cast<float>(padding) / static_cast<float>(processingSize);
		const float span = 1.0f - 2.0f * pad;
		if (!(span > 0.0f)) return glm::fvec2(minX, minY);
		return glm::fvec2(minX + (uv.x - pad) / span * (maxX - minX),
						  minY + (uv.y - pad) / span * (maxY - minY));
	}

	/**
	 * @brief Maps a source-space position into the logical texture.
	 * @param source Position in source coordinates.
	 * @return Texture coordinate in `[P/L, (L-P)/L]` for points inside the bbox.
	 */
	glm::fvec2 sourceToUv(const glm::fvec2& source) const
	{
		if (!processingSize) return glm::fvec2(0.0f);
		const float pad = static_cast<float>(padding) / static_cast<float>(processingSize);
		const float span = 1.0f - 2.0f * pad;
		const float dx = maxX - minX;
		const float dy = maxY - minY;
		return glm::fvec2(pad + (dx != 0.0f ? (source.x - minX) / dx : 0.0f) * span,
						  pad + (dy != 0.0f ? (source.y - minY) / dy : 0.0f) * span);
	}
};

/**
 * @brief Big-endian Elv data stream type for FontPacker vector-image files.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 */
template <Elv::Io::DeviceLike IoType = Elv::Io::Device>
using VectorGraphicDataStream = Elv::Io::DataStream<Elv::Util::Endian::Big, IoType>;

/** @brief Internal helpers for FontPacker vector-image stream operators. */
namespace Detail {

/**
 * @brief Verifies a four-byte FontPacker vector-image magic.
 * @param actual Magic read from the stream.
 * @throws std::runtime_error if the magic does not match WODI.
 */
inline void validateMagic(const std::array<char, 4>& actual)
{
	if (actual != StoredVectorImageMagic)
		throw std::runtime_error("Unsupported StoredVectorImage magic");
}

/**
 * @brief Verifies a FontPacker vector-image format version.
 * @param version Version read from the stream.
 * @throws std::runtime_error if the version is unsupported.
 */
inline void validateVersion(std::uint32_t version)
{
	if (version != StoredVectorImageVersion)
		throw std::runtime_error("Unsupported StoredVectorImage version");
}

} // namespace Detail

/**
 * @brief Writes an encoded vector-image mipmap payload.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Mipmap payload to write.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline VectorGraphicDataStream<IoType>& operator<<(VectorGraphicDataStream<IoType>& left, const Mipmap& right)
{
	return left << right.data;
}

/**
 * @brief Reads an encoded vector-image mipmap payload.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination mipmap payload.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline VectorGraphicDataStream<IoType>& operator>>(VectorGraphicDataStream<IoType>& left, Mipmap& right)
{
	return left >> right.data;
}

/**
 * @brief Writes a complete FontPacker stored vector image.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Stored vector image to write.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline VectorGraphicDataStream<IoType>& operator<<(VectorGraphicDataStream<IoType>& left, const StoredVectorImage& right)
{
	ImageFormatTag imageFormat = right.imageFormat;
	imageFormat.back() = '\0';
	const float midpoint = right.hasEncodingFlag(EncodingFlag::MidpointAdjusted)
						 ? right.midpointAdjustment : 1.0f;
	return left
		<< StoredVectorImageMagic
		<< right.version
		<< right.processingSize
		<< right.actualSize
		<< right.padding
		<< right.logicalX
		<< right.logicalY
		<< right.logicalWidth
		<< right.logicalHeight
		<< right.aspectRatio
		<< right.minX
		<< right.maxX
		<< right.minY
		<< right.maxY
		<< right.distanceRangeX
		<< right.distanceRangeY
		<< right.encodingFlags
		<< midpoint
		<< right.type
		<< right.distType
		<< imageFormat
		<< right.mipmaps;
}

/**
 * @brief Reads a complete FontPacker stored vector image.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination stored vector image.
 * @return Reference to the stream.
 * @throws std::runtime_error if the magic or version is unsupported.
 */
template <Elv::Io::DeviceLike IoType>
inline VectorGraphicDataStream<IoType>& operator>>(VectorGraphicDataStream<IoType>& left, StoredVectorImage& right)
{
	std::array<char, 4> magic {};
	StoredVectorImage decoded;
	left
		>> magic
		>> decoded.version;
	Detail::validateMagic(magic);
	Detail::validateVersion(decoded.version);

	left
		>> decoded.processingSize
		>> decoded.actualSize
		>> decoded.padding
		>> decoded.logicalX
		>> decoded.logicalY
		>> decoded.logicalWidth
		>> decoded.logicalHeight
		>> decoded.aspectRatio
		>> decoded.minX
		>> decoded.maxX
		>> decoded.minY
		>> decoded.maxY
		>> decoded.distanceRangeX
		>> decoded.distanceRangeY
		>> decoded.encodingFlags
		>> decoded.midpointAdjustment
		>> decoded.type
		>> decoded.distType
		>> decoded.imageFormat
		>> decoded.mipmaps;

	// FontPacker's reader forces the last byte of the fixed-width tag to NUL.
	decoded.imageFormat.back() = '\0';

	right = std::move(decoded);
	return left;
}

} // namespace VectorGraphic
} // namespace Lotte

#endif // LTESTOREDVECTORIMAGE_HPP
