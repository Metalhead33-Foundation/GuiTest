#ifndef LTEFONTFACE_HPP
#define LTEFONTFACE_HPP
/**
 * @file LteFontFace.hpp
 * @brief Declares FontPacker-compatible preprocessed font data for Lotte.
 */

#include <Lotte/LotteLib.hpp>
#include <Elvavena/Io/ElvDataStream.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Lotte {
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

/** @brief Two-component float vector used by kerning entries. */
struct Vec2f {
	float x = 0.0f;
	float y = 0.0f;
};

/** @brief One second-character kerning entry under a first character. */
struct PerCharKerningEntry {
	std::uint32_t secondChar = 0;
	Vec2f kerning {};
};

/** @brief Sparse kerning entries for one first character. */
struct KerningEntry {
	std::uint32_t firstChar = 0;
	std::vector<PerCharKerningEntry> perCharKerning;
};

/** @brief Offset table entry used internally by the FontPacker binary format. */
struct GlyphTOCEntry {
	std::uint32_t codePoint = 0;
	std::uint32_t offset = 0;
};

/** @brief Stored glyph metrics and SDF bitmap payload. */
struct StoredCharacter {
	bool valid = false;
	std::uint32_t width = 0;
	std::uint32_t height = 0;
	std::int32_t bearingX = 0;
	std::int32_t bearingY = 0;
	std::uint32_t advanceX = 0;
	std::uint32_t advanceY = 0;
	float metricWidth = 0.0f;
	float metricHeight = 0.0f;
	float horiBearingX = 0.0f;
	float horiBearingY = 0.0f;
	float horiAdvance = 0.0f;
	float vertBearingX = 0.0f;
	float vertBearingY = 0.0f;
	float vertAdvance = 0.0f;
	std::vector<std::byte> sdf;
};

/** @brief Unicode code point paired with its stored glyph data. */
struct Glyph {
	std::uint32_t codePoint = 0;
	StoredCharacter character {};
};

/** @brief Complete FontPacker preprocessed font face payload. */
struct PreprocessedFontFace {
	std::string familyName;
	SDFType type = SDFType::SDF;
	DistanceType distType = DistanceType::Manhattan;
	std::uint32_t bitmapSize = 0;
	std::uint32_t bitmapLogicalSize = 0;
	std::uint32_t bitmapPadding = 0;
	bool hasVert = false;
	bool jpeg = false;
	std::vector<KerningEntry> kerning;
	std::vector<Glyph> glyphs;
};

/** @brief Short public alias for the only top-level record in the format. */
using FontFace = PreprocessedFontFace;

template <Elv::Io::DeviceLike IoType = Elv::Io::Device>
using FontDataStream = Elv::Io::DataStream<Elv::Util::Endian::Big, IoType>;

namespace Detail {

inline std::uint32_t checkedUint32Size(std::size_t value, const char* field)
{
	if (value > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
		throw std::overflow_error(std::string(field) + " exceeds the FontPacker uint32_t range");
	return static_cast<std::uint32_t>(value);
}

inline std::uint32_t checkedFileOffset(long value, const char* field)
{
	if (value < 0 || static_cast<unsigned long long>(value) > std::numeric_limits<std::uint32_t>::max())
		throw std::overflow_error(std::string(field) + " exceeds the FontPacker uint32_t offset range");
	return static_cast<std::uint32_t>(value);
}

inline long checkedSeekOffset(std::uint32_t value, const char* field)
{
	if (static_cast<unsigned long long>(value) > static_cast<unsigned long long>(std::numeric_limits<long>::max()))
		throw std::overflow_error(std::string(field) + " exceeds this platform's seek range");
	return static_cast<long>(value);
}

template <Elv::Io::DeviceLike IoType>
inline void seekAbsolute(IoType& device, long offset, const char* field)
{
	if (offset < 0 || device.seek(offset, Elv::Io::SeekOrigin::SET) != 0)
		throw std::runtime_error(std::string("Failed to seek to ") + field);
}

} // namespace Detail

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, SDFType right)
{
	return left << static_cast<std::uint8_t>(right);
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, SDFType& right)
{
	std::uint8_t value = 0;
	left >> value;
	right = static_cast<SDFType>(value);
	return left;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, DistanceType right)
{
	return left << static_cast<std::uint8_t>(right);
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, DistanceType& right)
{
	std::uint8_t value = 0;
	left >> value;
	right = static_cast<DistanceType>(value);
	return left;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const Vec2f& right)
{
	return left << right.x << right.y;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, Vec2f& right)
{
	return left >> right.x >> right.y;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const PerCharKerningEntry& right)
{
	return left << right.secondChar << right.kerning;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, PerCharKerningEntry& right)
{
	return left >> right.secondChar >> right.kerning;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const KerningEntry& right)
{
	return left << right.firstChar << right.perCharKerning;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, KerningEntry& right)
{
	return left >> right.firstChar >> right.perCharKerning;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const GlyphTOCEntry& right)
{
	return left << right.codePoint << right.offset;
}

template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, GlyphTOCEntry& right)
{
	return left >> right.codePoint >> right.offset;
}

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
	for (const Glyph& glyph : right.glyphs) {
		toc.push_back({ glyph.codePoint, Detail::checkedFileOffset(left.device.tell(), "glyph offset") });
		left << glyph.character;
	}

	const long endPosition = left.device.tell();
	Detail::seekAbsolute(left.device, tocPosition, "glyph table of contents");
	for (const GlyphTOCEntry& entry : toc)
		left << entry;
	Detail::seekAbsolute(left.device, endPosition, "end of font face");
	return left;
}

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
		Glyph glyph;
		glyph.codePoint = entry.codePoint;
		left >> glyph.character;
		endPosition = std::max(endPosition, left.device.tell());
		decoded.glyphs.push_back(std::move(glyph));
	}

	Detail::seekAbsolute(left.device, endPosition, "end of font face");
	right = std::move(decoded);
	return left;
}

} // namespace Text
} // namespace Lotte

#endif // LTEFONTFACE_HPP
