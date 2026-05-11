#ifndef LTEFONTFACE_HPP
#define LTEFONTFACE_HPP
/**
 * @file LteFontFace.hpp
 * @brief Declares FontPacker-compatible preprocessed font data for Lotte.
 */

#include <Lotte/LotteLib.hpp>
#include <Elvavena/Io/ElvDataStream.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Elv {
namespace Io {

/**
 * @brief Writes a GLM two-component float vector to an Elv data stream.
 * @details This overload lets DataStream serialize glm::fvec2 values found
 * inside generic STL containers such as std::unordered_map.
 * @tparam endianness Byte order used by the DataStream.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Data stream to write to.
 * @param right Vector to write.
 * @return Reference to the stream.
 */
template <Elv::Util::Endian endianness, DeviceLike IoType>
inline DataStream<endianness, IoType>& operator<<(DataStream<endianness, IoType>& left, const glm::fvec2& right)
{
	return left << right.x << right.y;
}

/**
 * @brief Reads a GLM two-component float vector from an Elv data stream.
 * @details This overload lets DataStream deserialize glm::fvec2 values found
 * inside generic STL containers such as std::unordered_map.
 * @tparam endianness Byte order used by the DataStream.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Data stream to read from.
 * @param right Destination vector.
 * @return Reference to the stream.
 */
template <Elv::Util::Endian endianness, DeviceLike IoType>
inline DataStream<endianness, IoType>& operator>>(DataStream<endianness, IoType>& left, glm::fvec2& right)
{
	return left >> right.x >> right.y;
}

} // namespace Io
} // namespace Elv

namespace Lotte {
/** @brief Text and font data APIs for Lotte. */
namespace Text {

/** @brief FontPacker SDF channel layout. */
enum class SDFType : std::uint8_t {
	SDF = 0,   /**< Single-channel signed distance field. */
	MSDF = 1,  /**< Multi-channel signed distance field. */
	MSDFA = 2  /**< Multi-channel signed distance field with alpha. */
};

/** @brief FontPacker distance metric used while generating the SDF. */
enum class DistanceType : std::uint8_t {
	Manhattan = 0, /**< L1 distance. */
	Euclidean = 1  /**< L2 distance. */
};

/** @brief Kerning offsets keyed by the second Unicode code point. */
using PerCharacterKerning = std::unordered_map<std::uint32_t, glm::fvec2>;

/** @brief Sparse kerning table keyed first by the first Unicode code point. */
using KerningMap = std::unordered_map<std::uint32_t, PerCharacterKerning>;

/** @brief Offset table entry used internally by the FontPacker binary format. */
struct GlyphTOCEntry {
	/** @brief Unicode code point of the glyph. */
	std::uint32_t codePoint = 0;
	/** @brief Absolute byte offset from the start of the file to glyph data. */
	std::uint32_t offset = 0;
};

/** @brief Stored glyph metrics and SDF bitmap payload. */
struct StoredCharacter {
	/** @brief Whether this glyph entry contains usable metric and bitmap data. */
	bool valid = false;
	/** @brief Intended bitmap width in pixels. */
	std::uint32_t width = 0;
	/** @brief Intended bitmap height in pixels. */
	std::uint32_t height = 0;
	/** @brief Signed horizontal bearing in pixel units. */
	std::int32_t bearingX = 0;
	/** @brief Signed vertical bearing in pixel units. */
	std::int32_t bearingY = 0;
	/** @brief Horizontal advance in pixel units. */
	std::uint32_t advanceX = 0;
	/** @brief Vertical advance in pixel units. */
	std::uint32_t advanceY = 0;
	/** @brief Floating-point glyph metric width. */
	float metricWidth = 0.0f;
	/** @brief Floating-point glyph metric height. */
	float metricHeight = 0.0f;
	/** @brief FreeType-style horizontal bearing X. */
	float horiBearingX = 0.0f;
	/** @brief FreeType-style horizontal bearing Y. */
	float horiBearingY = 0.0f;
	/** @brief FreeType-style horizontal advance. */
	float horiAdvance = 0.0f;
	/** @brief FreeType-style vertical bearing X. */
	float vertBearingX = 0.0f;
	/** @brief FreeType-style vertical bearing Y. */
	float vertBearingY = 0.0f;
	/** @brief FreeType-style vertical advance. */
	float vertAdvance = 0.0f;
	/** @brief Raw SDF bitmap bytes, compressed when the parent font has jpeg set. */
	std::vector<std::byte> sdf;
};

/** @brief Complete FontPacker preprocessed font face payload. */
struct PreprocessedFontFace {
	/** @brief UTF-8 font family name. */
	std::string familyName;
	/** @brief SDF channel layout. */
	SDFType type = SDFType::SDF;
	/** @brief Distance metric used to generate the SDF. */
	DistanceType distType = DistanceType::Manhattan;
	/** @brief Bitmap size in pixels. */
	std::uint32_t bitmapSize = 0;
	/** @brief Logical bitmap size before padding. */
	std::uint32_t bitmapLogicalSize = 0;
	/** @brief Bitmap padding in pixels. */
	std::uint32_t bitmapPadding = 0;
	/** @brief Whether vertical layout metrics are available. */
	bool hasVert = false;
	/** @brief Whether SDF payloads are JPEG-compressed. */
	bool jpeg = false;
	/** @brief Sparse font kerning table keyed as first code point, second code point, offset. */
	KerningMap kerning;
	/** @brief Stored glyph payloads keyed by Unicode code point. */
	std::unordered_map<std::uint32_t, StoredCharacter> glyphs;
};

/** @brief Short public alias for the only top-level record in the format. */
using FontFace = PreprocessedFontFace;

/**
 * @brief Big-endian Elv data stream type for FontPacker binary font files.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 */
template <Elv::Io::DeviceLike IoType = Elv::Io::Device>
using FontDataStream = Elv::Io::DataStream<Elv::Util::Endian::Big, IoType>;

/** @brief Internal helpers for FontPacker binary stream operators. */
namespace Detail {

/**
 * @brief Converts an STL container size to the format's uint32_t size field.
 * @param value STL container size.
 * @param field Human-readable field name used in overflow diagnostics.
 * @return Value narrowed to uint32_t.
 * @throws std::overflow_error if the value cannot fit in uint32_t.
 */
inline std::uint32_t checkedUint32Size(std::size_t value, const char* field)
{
	if (value > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
		throw std::overflow_error(std::string(field) + " exceeds the FontPacker uint32_t range");
	return static_cast<std::uint32_t>(value);
}

/**
 * @brief Converts a device tell position to the format's uint32_t offset field.
 * @param value Device position returned by tell().
 * @param field Human-readable field name used in overflow diagnostics.
 * @return Value narrowed to uint32_t.
 * @throws std::overflow_error if the value is negative or too large.
 */
inline std::uint32_t checkedFileOffset(long value, const char* field)
{
	if (value < 0 || static_cast<unsigned long long>(value) > std::numeric_limits<std::uint32_t>::max())
		throw std::overflow_error(std::string(field) + " exceeds the FontPacker uint32_t offset range");
	return static_cast<std::uint32_t>(value);
}

/**
 * @brief Converts a format offset to the platform seek type.
 * @param value FontPacker uint32_t offset.
 * @param field Human-readable field name used in overflow diagnostics.
 * @return Value widened to long for Elv::Io::Device::seek().
 * @throws std::overflow_error if the platform cannot represent the offset.
 */
inline long checkedSeekOffset(std::uint32_t value, const char* field)
{
	if (static_cast<unsigned long long>(value) > static_cast<unsigned long long>(std::numeric_limits<long>::max()))
		throw std::overflow_error(std::string(field) + " exceeds this platform's seek range");
	return static_cast<long>(value);
}

/**
 * @brief Seeks a device to an absolute offset or throws on failure.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param device Device to seek.
 * @param offset Absolute byte offset from the start of the file.
 * @param field Human-readable target name used in diagnostics.
 * @throws std::runtime_error if the seek fails.
 */
template <Elv::Io::DeviceLike IoType>
inline void seekAbsolute(IoType& device, long offset, const char* field)
{
	if (offset < 0 || device.seek(offset, Elv::Io::SeekOrigin::SET) != 0)
		throw std::runtime_error(std::string("Failed to seek to ") + field);
}

} // namespace Detail

/**
 * @brief Writes an SDFType enumeration as its uint8_t binary value.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Enumeration value to write.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, SDFType right)
{
	return left << static_cast<std::uint8_t>(right);
}

/**
 * @brief Reads an SDFType enumeration from its uint8_t binary value.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination enumeration value.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, SDFType& right)
{
	std::uint8_t value = 0;
	left >> value;
	right = static_cast<SDFType>(value);
	return left;
}

/**
 * @brief Writes a DistanceType enumeration as its uint8_t binary value.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Enumeration value to write.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, DistanceType right)
{
	return left << static_cast<std::uint8_t>(right);
}

/**
 * @brief Reads a DistanceType enumeration from its uint8_t binary value.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination enumeration value.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, DistanceType& right)
{
	std::uint8_t value = 0;
	left >> value;
	right = static_cast<DistanceType>(value);
	return left;
}

/**
 * @brief Writes one glyph table-of-contents entry.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right TOC entry to write.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const GlyphTOCEntry& right)
{
	return left << right.codePoint << right.offset;
}

/**
 * @brief Reads one glyph table-of-contents entry.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination TOC entry.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, GlyphTOCEntry& right)
{
	return left >> right.codePoint >> right.offset;
}

/**
 * @brief Writes a stored glyph payload.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Glyph payload to write.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const StoredCharacter& right)
{
	left << right.valid;
	if (!right.valid)
		return left;

	return left
		<< right.width
		<< right.height
		<< right.bearingX
		<< right.bearingY
		<< right.advanceX
		<< right.advanceY
		<< right.metricWidth
		<< right.metricHeight
		<< right.horiBearingX
		<< right.horiBearingY
		<< right.horiAdvance
		<< right.vertBearingX
		<< right.vertBearingY
		<< right.vertAdvance
		<< right.sdf;
}

/**
 * @brief Reads a stored glyph payload.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination glyph payload.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, StoredCharacter& right)
{
	StoredCharacter decoded;
	left >> decoded.valid;
	if (decoded.valid) {
		left
			>> decoded.width
			>> decoded.height
			>> decoded.bearingX
			>> decoded.bearingY
			>> decoded.advanceX
			>> decoded.advanceY
			>> decoded.metricWidth
			>> decoded.metricHeight
			>> decoded.horiBearingX
			>> decoded.horiBearingY
			>> decoded.horiAdvance
			>> decoded.vertBearingX
			>> decoded.vertBearingY
			>> decoded.vertAdvance
			>> decoded.sdf;
	}
	right = std::move(decoded);
	return left;
}

/**
 * @brief Writes a complete FontPacker preprocessed font face.
 * @details The exporter writes placeholder TOC entries, writes kerning and
 * glyph payloads, then seeks back to patch absolute glyph offsets into the TOC.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Font face to write.
 * @return Reference to the stream.
 * @throws std::overflow_error if a count or offset cannot fit the binary format.
 * @throws std::runtime_error if required seeking fails.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const PreprocessedFontFace& right)
{
	left
		<< right.familyName
		<< right.type
		<< right.distType
		<< right.bitmapSize
		<< right.bitmapLogicalSize
		<< right.bitmapPadding
		<< right.hasVert
		<< right.jpeg;

	const std::uint32_t charCount = Detail::checkedUint32Size(right.glyphs.size(), "glyph count");
	left << charCount;

	const long tocPosition = left.device.tell();
	for (std::uint32_t i = 0; i < charCount; ++i)
		left << GlyphTOCEntry {};

	left << right.kerning;

	std::vector<GlyphTOCEntry> toc;
	toc.reserve(right.glyphs.size());
	for (const auto& glyph : right.glyphs) {
		toc.push_back({ glyph.first, Detail::checkedFileOffset(left.device.tell(), "glyph offset") });
		left << glyph.second;
	}

	const long endPosition = left.device.tell();
	Detail::seekAbsolute(left.device, tocPosition, "glyph table of contents");
	for (const GlyphTOCEntry& entry : toc)
		left << entry;
	Detail::seekAbsolute(left.device, endPosition, "end of font face");
	return left;
}

/**
 * @brief Reads a complete FontPacker preprocessed font face.
 * @details The importer reads the TOC and kerning block first, then seeks to
 * each absolute glyph offset to import stored glyph payloads.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination font face.
 * @return Reference to the stream.
 * @throws std::overflow_error if an offset cannot be represented for seeking.
 * @throws std::runtime_error if required seeking fails.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, PreprocessedFontFace& right)
{
	PreprocessedFontFace decoded;
	left
		>> decoded.familyName
		>> decoded.type
		>> decoded.distType
		>> decoded.bitmapSize
		>> decoded.bitmapLogicalSize
		>> decoded.bitmapPadding
		>> decoded.hasVert
		>> decoded.jpeg;

	std::uint32_t charCount = 0;
	left >> charCount;

	std::vector<GlyphTOCEntry> toc(charCount);
	for (GlyphTOCEntry& entry : toc)
		left >> entry;

	left >> decoded.kerning;

	decoded.glyphs.reserve(toc.size());
	long endPosition = left.device.tell();
	for (const GlyphTOCEntry& entry : toc) {
		Detail::seekAbsolute(left.device, Detail::checkedSeekOffset(entry.offset, "glyph offset"), "glyph data");
		StoredCharacter character;
		left >> character;
		endPosition = std::max(endPosition, left.device.tell());
		decoded.glyphs.emplace(entry.codePoint, std::move(character));
	}

	Detail::seekAbsolute(left.device, endPosition, "end of font face");
	right = std::move(decoded);
	return left;
}

} // namespace Text
} // namespace Lotte

#endif // LTEFONTFACE_HPP
