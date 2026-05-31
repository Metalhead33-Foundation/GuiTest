#ifndef LTESTOREDVECTORIMAGE_HPP
#define LTESTOREDVECTORIMAGE_HPP
/**
 * @file LteStoredVectorImage.hpp
 * @brief Declares FontPacker-compatible preprocessed vector-image data for Lotte.
 */

#include <Lotte/LotteLib.hpp>
#include <Lotte/Text/LteFontFace.hpp>
#include <Elvavena/Io/ElvDataStream.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
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

/** @brief Bit values used by StoredVectorImage::encodingFlags. */
enum class EncodingFlag : std::uint32_t {
	Inverted = 0x00000001,           /**< SDF values were inverted. */
	GammaCorrected = 0x00000002,     /**< SDF values were gamma corrected. */
	MaximumDownsampling = 0x00000004,/**< Downsampling used maximum values instead of averaging. */
	MidpointAdjusted = 0x00000008    /**< midpointAdjustment contains a baked adjustment value. */
};

/** @brief One encoded mipmap image payload. */
struct Mipmap {
	/** @brief Encoded image bytes for this mip level. */
	std::vector<std::byte> data;
};

/** @brief Complete FontPacker standalone vector-image SDF payload. */
struct StoredVectorImage {
	/** @brief FontPacker binary format version written after the WODI magic. */
	std::uint32_t version = StoredVectorImageVersion;
	/** @brief Square size used during SDF generation. */
	std::uint32_t processingSize = 0;
	/** @brief Level-0 final square texture size. */
	std::uint32_t actualSize = 0;
	/** @brief Padding in pixels. */
	std::uint32_t padding = 0;
	/** @brief Original vector canvas or viewBox X origin. */
	float logicalX = 0.0f;
	/** @brief Original vector canvas or viewBox Y origin. */
	float logicalY = 0.0f;
	/** @brief Original vector canvas or viewBox width. */
	float logicalWidth = 0.0f;
	/** @brief Original vector canvas or viewBox height. */
	float logicalHeight = 0.0f;
	/** @brief Ratio of logicalWidth to logicalHeight. */
	float aspectRatio = 0.0f;
	/** @brief Minimum decomposed source X before normalization. */
	float minX = 0.0f;
	/** @brief Maximum decomposed source X before normalization. */
	float maxX = 0.0f;
	/** @brief Minimum decomposed source Y before normalization. */
	float minY = 0.0f;
	/** @brief Maximum decomposed source Y before normalization. */
	float maxY = 0.0f;
	/** @brief Horizontal encoded distance range in level-0 pixels. */
	float distanceRangeX = 0.0f;
	/** @brief Vertical encoded distance range in level-0 pixels. */
	float distanceRangeY = 0.0f;
	/** @brief Bitmask of baked SDF value transforms. */
	std::uint32_t encodingFlags = 0;
	/** @brief Baked midpoint adjustment when EncodingFlag::MidpointAdjusted is set. */
	float midpointAdjustment = 0.0f;
	/** @brief SDF channel layout. */
	SDFType type = SDFType::SDF;
	/** @brief Distance metric used to generate the SDF. */
	DistanceType distType = DistanceType::Manhattan;
	/** @brief Null-terminated image format tag used by encoded mipmap payloads. */
	ImageFormatTag imageFormat = { 'P', 'N', 'G', '\0' };
	/** @brief Encoded mipmap image payloads. */
	std::vector<Mipmap> mipmaps;
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
		<< right.midpointAdjustment
		<< right.type
		<< right.distType
		<< right.imageFormat
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

	right = std::move(decoded);
	return left;
}

} // namespace VectorGraphic
} // namespace Lotte

#endif // LTESTOREDVECTORIMAGE_HPP
