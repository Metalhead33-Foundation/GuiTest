#ifndef LTEFONTFACE_HPP
#define LTEFONTFACE_HPP
/**
 * @file LteFontFace.hpp
 * @brief Declares FontPacker-compatible preprocessed font data for Lotte.
 *
 * @see https://github.com/Metalhead33-Foundation/FontPacker/blob/master/BINARY_FORMAT.md
 */

#include <Lotte/LotteLib.hpp>
#include <Elvavena/Io/ElvDataStream.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
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

/** @brief Four-byte magic identifier for FontPacker preprocessed font files. */
inline constexpr std::array<char, 4> FontFaceMagic = { 'W', 'O', 'D', 'F' };

/**
 * @brief Format version written by new font faces.
 *
 * FontPacker's current version is 3. Version 2 added the nominal distance
 * ranges, the encoding flags and the midpoint adjustment to the header;
 * version 3 changed each glyph's `advance_x`/`advance_y` from a raw 26.6
 * fixed-point integer to a float in pixels. The record sizes and offsets are
 * identical in versions 2 and 3.
 */
inline constexpr std::uint32_t FontFaceVersion = 3;

/** @brief Oldest reader-supported font face version. */
inline constexpr std::uint32_t FontFaceMinVersion = 1;

/** @brief Scale of the raw 26.6 fixed-point advances stored by versions 1 and 2. */
inline constexpr float FixedPoint26_6 = 64.0f;

/** @brief Fixed byte count for FontPacker image format tags. */
inline constexpr std::size_t ImageFormatLength = 8;

/** @brief Null-terminated, zero-padded FontPacker image format tag. */
using ImageFormatTag = std::array<char, ImageFormatLength>;

/**
 * @brief Normalizes an image codec name into a FontPacker image format tag.
 *
 * Mirrors FontPacker's `setImageFormat()`: the value is trimmed, upper-cased,
 * truncated to seven characters and zero padded. An empty value becomes `PNG`.
 * @param tag Tag to overwrite.
 * @param format Codec name accepted by the image decoder.
 */
inline void setImageFormatTag(ImageFormatTag& tag, std::string_view format)
{
	tag.fill('\0');
	std::size_t begin = 0;
	std::size_t end = format.size();
	while (begin < end && std::isspace(static_cast<unsigned char>(format[begin]))) ++begin;
	while (end > begin && std::isspace(static_cast<unsigned char>(format[end - 1]))) --end;

	const std::size_t length = std::min(end - begin, ImageFormatLength - 1);
	if (length == 0) {
		tag[0] = 'P';
		tag[1] = 'N';
		tag[2] = 'G';
		return;
	}
	for (std::size_t i = 0; i < length; ++i)
		tag[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(format[begin + i])));
}

/**
 * @brief Returns the codec name stored in an image format tag.
 * @param tag Tag to inspect; reading stops at the first NUL.
 */
inline std::string_view imageFormatTagName(const ImageFormatTag& tag)
{
	const auto terminator = std::find(tag.begin(), tag.end(), '\0');
	return std::string_view(tag.data(), static_cast<std::size_t>(terminator - tag.begin()));
}

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

/**
 * @brief Transforms that were baked into the encoded SDF payloads.
 *
 * Bit values are shared by `.wodf` (version 2+) and `.wodi`, and match
 * `StoredVectorImage::encodingFlags` in FontPacker's BINARY_FORMAT.md.
 */
enum class EncodingFlag : std::uint32_t {
	Inverted = 0x00000001,            /**< Values were inverted (`1 - x`). */
	GammaCorrected = 0x00000002,      /**< The cubic gamma adjustment was applied. */
	MaximumDownsampling = 0x00000004, /**< Downsampling used maximum instead of average. */
	MidpointAdjusted = 0x00000008     /**< @ref PreprocessedFontFace::midpointAdjustment holds a baked divisor. */
};

/**
 * @brief Kerning offsets keyed by the second Unicode code point.
 *
 * Values are FreeType kerning offsets in **pixels at the face's reference size**
 * (FontPacker converts the 26.6 vector returned by `FT_Get_Kerning()` before storing
 * it), so they are used exactly like `bearingX`/`width` once scaled to the requested
 * run size. Most fonts carry no kerning at all, in which case the whole map is empty.
 */
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

/**
 * @brief Stored glyph metrics and SDF bitmap payload.
 *
 * FontPacker fills this from FreeType. Two layout boxes are stored:
 *
 * - `width`/`height`/`bearingX`/`bearingY` come from `FT_GlyphSlot.bitmap.width`,
 *   `bitmap.rows`, `bitmap_left` and `bitmap_top`. They are the **intended layout
 *   box** in whole pixels, with the padding excluded. The runtime draws the
 *   content box at this size and offset and expands it by the padding runway so
 *   the SDF glow has room (see `LteTextLayout.hpp` and `LteFontAtlas.hpp`).
 * - `metricWidth`/`metricHeight`/`horiBearingX`/`horiBearingY` are the
 *   **FreeType metric box** (float pixels). FontPacker stretches exactly this
 *   box over the square SDF content box, so it describes the box the payload was
 *   generated from. It is *not* modified by the fit, so the aspect ratio survives
 *   in `width`/`height`.
 *
 * For SVG-backed faces (`familyName == "SVG"`) both boxes are the SVG viewport
 * and the bearings are 0; the texture holds the whole viewport.
 *
 * Units: every field is in **pixels at `em = bitmapLogicalSize - bitmapPadding`**
 * for font input, and in SVG user units for SVG input. Version 3 stores
 * `advanceX`/`advanceY` as float pixels. Versions 1 and 2 stored raw 26.6
 * integers (version 1 SVG faces stored plain pixels); the importer converts them
 * to float pixels, and the exporter re-encodes them when writing an older
 * container version.
 */
struct StoredCharacter {
	/** @brief Whether this glyph entry contains usable metric and bitmap data. */
	bool valid = false;
	/** @brief Intended glyph box width in whole pixels; excludes padding. */
	std::uint32_t width = 0;
	/** @brief Intended glyph box height in whole pixels; excludes padding. */
	std::uint32_t height = 0;
	/** @brief Offset from the pen origin to the left edge of the box, in whole pixels. */
	std::int32_t bearingX = 0;
	/** @brief Offset from the baseline up to the top edge of the box, in whole pixels. */
	std::int32_t bearingY = 0;
	/** @brief Horizontal advance in float pixels (versions 1/2 are decoded from 26.6 on import). */
	float advanceX = 0.0f;
	/** @brief Vertical advance in float pixels (usually 0 for horizontal fonts). */
	float advanceY = 0.0f;
	/** @brief Metric-box width in float pixels; the box stretched onto the SDF square. */
	float metricWidth = 0.0f;
	/** @brief Metric-box height in float pixels; the box stretched onto the SDF square. */
	float metricHeight = 0.0f;
	/** @brief Horizontal offset of the metric box from the pen position, in float pixels. */
	float horiBearingX = 0.0f;
	/** @brief Vertical offset of the metric box from the baseline, in float pixels (positive = above). */
	float horiBearingY = 0.0f;
	/** @brief Horizontal advance in float pixels; normally equal to @ref advanceX. */
	float horiAdvance = 0.0f;
	/** @brief Vertical bearing X in float pixels; valid only when `hasVert` is true. */
	float vertBearingX = 0.0f;
	/** @brief Vertical bearing Y in float pixels; valid only when `hasVert` is true. */
	float vertBearingY = 0.0f;
	/** @brief Vertical advance in float pixels; valid only when `hasVert` is true. */
	float vertAdvance = 0.0f;
	/** @brief Raw encoded SDF bitmap bytes using the parent font's imageFormat. */
	std::vector<std::byte> sdf;
};

/** @brief Complete FontPacker preprocessed font face payload. */
struct PreprocessedFontFace {
	/**
	 * @brief FontPacker binary format version written after the WODF magic.
	 *
	 * New faces are written as version 3. A loaded version 1 or 2 face keeps its
	 * version. Version 1 is legacy: its four float metric fields hold outline
	 * bounds for outline/SVG glyphs, its SVG advances are plain pixels and its
	 * `bitmapSize` may be 0; regenerate the asset to obtain version 3.
	 */
	std::uint32_t version = FontFaceVersion;
	/** @brief UTF-8 font family name. */
	std::string familyName;
	/** @brief SDF channel layout. */
	SDFType type = SDFType::SDF;
	/** @brief Distance metric used to generate the SDF. */
	DistanceType distType = DistanceType::Manhattan;
	/**
	 * @brief Stored square size `S` of every glyph payload.
	 *
	 * Writers always write the real size. Only a historical version 1 file may
	 * contain `0`, which means "the stored images are `bitmapLogicalSize` x
	 * `bitmapLogicalSize`". Use @ref storedBitmapSize() instead of reading this
	 * field directly.
	 */
	std::uint32_t bitmapSize = 0;
	/**
	 * @brief Generation square size `L`; the padding is inside it.
	 *
	 * FontPacker generates each glyph at `internalProcessSize` and then
	 * downsamples to @ref bitmapSize (power-of-two halving plus a final resize),
	 * so this is the resolution the outlined glyph - and its padding - was
	 * scaled into.
	 */
	std::uint32_t bitmapLogicalSize = 0;
	/**
	 * @brief Padding `P` on all four sides, in pixels of the `L` grid.
	 *
	 * The padding is inside `L`, so the content box is `(L - 2P)^2`. It is baked
	 * into the SDF and shrinks with the downsample: the padding of the stored
	 * payload is `P_final = bitmapPadding * bitmapSize / bitmapLogicalSize`. The
	 * atlas must not add padding of its own, but the drawing quad must expand by
	 * `width * P / (L - 2P)` (see `LteTextLayout.hpp`).
	 */
	std::uint32_t bitmapPadding = 0;
	/** @brief Whether vertical layout metrics are available. */
	bool hasVert = false;
	/** @brief Null-terminated image format tag used by encoded glyph payloads. */
	ImageFormatTag imageFormat = { 'P', 'N', 'G', '\0', '\0', '\0', '\0', '\0' };
	/** @brief Scaled face ascender in pixels (SVG: viewport height in user units). */
	float ascender = 0.0f;
	/** @brief Scaled face descender in pixels (usually negative; 0 for SVG). */
	float descender = 0.0f;
	/** @brief Scaled baseline-to-baseline distance in pixels (SVG: viewport height). */
	float faceHeight = 0.0f;
	/** @brief Scaled maximum advance in pixels (SVG: viewport width). */
	float maxAdvance = 0.0f;
	/** @brief Original font design units per EM (SVG: viewport height). */
	std::uint32_t unitsPerEm = 0;
	/**
	 * @brief Version 2+: nominal encoded radius in X, in level-0 texels.
	 *
	 * This is `(samplesToCheckX / 2 or bitmapPadding) * S / L`, i.e. the search
	 * radius expressed in stored texels. Both generators normalise each image by
	 * its own observed maximum, so this is an upper bound for `screenPxRange`,
	 * not an exact per-glyph scale. Version 1 files have no such field; the
	 * importer reconstructs `bitmapPadding * S / L` in memory.
	 */
	float distanceRangeX = 0.0f;
	/** @brief Version 2+: nominal encoded radius in Y, in level-0 texels. */
	float distanceRangeY = 0.0f;
	/** @brief Version 2+: bitmask of @ref EncodingFlag values baked into the pixels. */
	std::uint32_t encodingFlags = 0;
	/** @brief Version 2+: baked midpoint divisor; only meaningful with @ref EncodingFlag::MidpointAdjusted. */
	float midpointAdjustment = 1.0f;
	/** @brief Sparse font kerning table keyed as first code point, second code point, offset. */
	KerningMap kerning;
	/** @brief Stored glyph payloads keyed by Unicode code point. */
	std::unordered_map<std::uint32_t, StoredCharacter> glyphs;

	/** @brief Resolves the stored square size, falling back to `L` for legacy version 1 files. */
	std::uint32_t storedBitmapSize() const { return bitmapSize ? bitmapSize : bitmapLogicalSize; }

	/** @brief Returns the effective em size the metrics are expressed in, or 0 when unknown. */
	std::uint32_t referenceEmSize() const
	{
		return bitmapLogicalSize > bitmapPadding ? bitmapLogicalSize - bitmapPadding : 0;
	}

	/** @brief Whether this face was generated from an SVG (FontPacker names those faces "SVG"). */
	bool isSvgBacked() const { return familyName == "SVG"; }

	/** @brief Whether the non-versioned 26.6 advance interpretation is the version 1 SVG exception. */
	bool hasLegacySvgAdvances() const { return version == 1 && isSvgBacked(); }

	/** @brief Returns whether a given @ref EncodingFlag bit is set (version 2+ files). */
	bool hasEncodingFlag(EncodingFlag flag) const
	{
		return (encodingFlags & static_cast<std::uint32_t>(flag)) != 0;
	}

	/** @brief Normalizes and stores an image codec name (trimmed, upper-case, max 7 characters). */
	void setImageFormat(std::string_view format) { setImageFormatTag(imageFormat, format); }

	/** @brief Returns the stored image codec name, without NUL padding. */
	std::string_view imageFormatName() const { return imageFormatTagName(imageFormat); }
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
 * @brief Verifies a four-byte FontPacker container magic.
 * @param actual Magic read from the stream.
 * @param expected Magic required by the container.
 * @param container Human-readable container name for diagnostics.
 * @throws std::runtime_error if the magic does not match.
 */
inline void validateMagic(const std::array<char, 4>& actual, const std::array<char, 4>& expected, const char* container)
{
	if (actual != expected)
		throw std::runtime_error(std::string("Unsupported ") + container + " magic");
}

/**
 * @brief Verifies a FontPacker container version against the supported range.
 * @param version Version read from the stream.
 * @param minVersion Oldest supported version (inclusive).
 * @param maxVersion Newest supported version (inclusive).
 * @param container Human-readable container name for diagnostics.
 * @throws std::runtime_error if the version is unsupported.
 */
inline void validateVersion(std::uint32_t version, std::uint32_t minVersion, std::uint32_t maxVersion, const char* container)
{
	if (version < minVersion || version > maxVersion)
		throw std::runtime_error(std::string("Unsupported ") + container + " version");
}

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

/**
 * @brief Decodes a version 1/2 raw 26.6 fixed-point advance into float pixels.
 * @param raw Raw integer stored by the container.
 * @return Advance in pixels.
 */
inline float decodeFixed26_6(std::uint32_t raw)
{
	return static_cast<float>(raw) / FixedPoint26_6;
}

/**
 * @brief Encodes a float pixel advance as a raw 26.6 fixed-point integer.
 * @param value Advance in pixels.
 * @return Rounded integer, matching FontPacker's `std::round(value * 64)`.
 */
inline std::uint32_t encodeFixed26_6(float value)
{
	if (!(value > 0.0f)) return 0;
	const double scaled = std::round(static_cast<double>(value) * static_cast<double>(FixedPoint26_6));
	if (scaled >= static_cast<double>(std::numeric_limits<std::uint32_t>::max()))
		return std::numeric_limits<std::uint32_t>::max();
	return static_cast<std::uint32_t>(scaled);
}

/**
 * @brief Encodes a float pixel advance as the plain integer used by version 1 SVG faces.
 * @param value Advance in pixels.
 * @return Rounded integer.
 */
inline std::uint32_t encodePlainAdvance(float value)
{
	if (!(value > 0.0f)) return 0;
	const double rounded = std::round(static_cast<double>(value));
	if (rounded >= static_cast<double>(std::numeric_limits<std::uint32_t>::max()))
		return std::numeric_limits<std::uint32_t>::max();
	return static_cast<std::uint32_t>(rounded);
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
 * @brief Writes a stored glyph payload in a specific container version.
 *
 * The record layout is identical in every supported version; only the
 * `advanceX`/`advanceY` encoding changes: version 3+ writes float pixels,
 * versions 1 and 2 write a raw 26.6 integer (a version 1 SVG face writes a
 * plain pixel integer instead).
 *
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Glyph payload to write.
 * @param version Container version that governs the advance encoding.
 * @param legacySvgAdvances Whether this is a version 1 SVG-backed face.
 */
template <Elv::Io::DeviceLike IoType>
inline void writeStoredCharacter(FontDataStream<IoType>& left, const StoredCharacter& right,
								 std::uint32_t version, bool legacySvgAdvances = false)
{
	left << right.valid;
	if (!right.valid)
		return;

	left << right.width << right.height << right.bearingX << right.bearingY;
	if (version >= FontFaceVersion) {
		left << right.advanceX << right.advanceY;
	} else if (legacySvgAdvances) {
		left << Detail::encodePlainAdvance(right.advanceX) << Detail::encodePlainAdvance(right.advanceY);
	} else {
		left << Detail::encodeFixed26_6(right.advanceX) << Detail::encodeFixed26_6(right.advanceY);
	}
	left << right.metricWidth << right.metricHeight
		 << right.horiBearingX << right.horiBearingY << right.horiAdvance
		 << right.vertBearingX << right.vertBearingY << right.vertAdvance;
	Detail::checkedUint32Size(right.sdf.size(), "sdf length");
	left << right.sdf;
}

/**
 * @brief Reads a stored glyph payload written in a specific container version.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination glyph payload.
 * @param version Container version that governs the advance encoding.
 * @param legacySvgAdvances Whether this is a version 1 SVG-backed face.
 */
template <Elv::Io::DeviceLike IoType>
inline void readStoredCharacter(FontDataStream<IoType>& left, StoredCharacter& right,
								std::uint32_t version, bool legacySvgAdvances = false)
{
	StoredCharacter decoded;
	left >> decoded.valid;
	if (decoded.valid) {
		left >> decoded.width >> decoded.height >> decoded.bearingX >> decoded.bearingY;
		if (version >= FontFaceVersion) {
			left >> decoded.advanceX >> decoded.advanceY;
		} else {
			std::uint32_t rawX = 0;
			std::uint32_t rawY = 0;
			left >> rawX >> rawY;
			if (legacySvgAdvances) {
				decoded.advanceX = static_cast<float>(rawX);
				decoded.advanceY = static_cast<float>(rawY);
			} else {
				decoded.advanceX = Detail::decodeFixed26_6(rawX);
				decoded.advanceY = Detail::decodeFixed26_6(rawY);
			}
		}
		left >> decoded.metricWidth >> decoded.metricHeight
			 >> decoded.horiBearingX >> decoded.horiBearingY >> decoded.horiAdvance
			 >> decoded.vertBearingX >> decoded.vertBearingY >> decoded.vertAdvance
			 >> decoded.sdf;
	}
	right = std::move(decoded);
}

/**
 * @brief Writes a stored glyph payload in the current container version.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Glyph payload to write.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const StoredCharacter& right)
{
	writeStoredCharacter(left, right, FontFaceVersion, false);
	return left;
}

/**
 * @brief Reads a stored glyph payload written in the current container version.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination glyph payload.
 * @return Reference to the stream.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, StoredCharacter& right)
{
	readStoredCharacter(left, right, FontFaceVersion, false);
	return left;
}

/**
 * @brief Writes a complete FontPacker preprocessed font face.
 * @details The exporter writes placeholder TOC entries, writes kerning and
 * glyph payloads, then seeks back to patch absolute glyph offsets into the TOC.
 * Glyphs and kerning pairs are written in ascending code-point order, so the
 * output is byte-deterministic and matches FontPacker's `QMap` ordering.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Font face to write.
 * @return Reference to the stream.
 * @throws std::overflow_error if a count or offset cannot fit the binary format.
 * @throws std::runtime_error if the version is unsupported or required seeking fails.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator<<(FontDataStream<IoType>& left, const PreprocessedFontFace& right)
{
	Detail::validateVersion(right.version, FontFaceMinVersion, FontFaceVersion, "PreprocessedFontFace");

	left
		<< FontFaceMagic
		<< right.version
		<< right.familyName
		<< right.type
		<< right.distType
		<< right.storedBitmapSize()
		<< right.bitmapLogicalSize
		<< right.bitmapPadding
		<< right.hasVert;
	// The tag is fixed-width and must be NUL-terminated; the last byte is reserved.
	ImageFormatTag imageFormat = right.imageFormat;
	imageFormat.back() = '\0';
	left << imageFormat
		<< right.ascender
		<< right.descender
		<< right.faceHeight
		<< right.maxAdvance
		<< right.unitsPerEm;
	if (right.version >= 2) {
		const float midpoint = right.hasEncodingFlag(EncodingFlag::MidpointAdjusted)
							 ? right.midpointAdjustment : 1.0f;
		left << right.distanceRangeX << right.distanceRangeY
			 << right.encodingFlags << midpoint;
	}

	const std::uint32_t charCount = Detail::checkedUint32Size(right.glyphs.size(), "glyph count");
	std::vector<GlyphTOCEntry> placeholders(charCount);

	const long tocPosition = left.device.tell();
	left << placeholders;

	// Order both maps so the file does not depend on hash iteration order.
	std::map<std::uint32_t, std::map<std::uint32_t, glm::fvec2>> orderedKerning;
	for (const auto& first : right.kerning) {
		for (const auto& second : first.second)
			orderedKerning[first.first][second.first] = second.second;
	}
	left << orderedKerning;

	std::vector<std::uint32_t> codePoints;
	codePoints.reserve(right.glyphs.size());
	for (const auto& glyph : right.glyphs)
		codePoints.push_back(glyph.first);
	std::sort(codePoints.begin(), codePoints.end());

	const bool legacySvgAdvances = right.hasLegacySvgAdvances();
	std::vector<GlyphTOCEntry> toc;
	toc.reserve(codePoints.size());
	for (const std::uint32_t codePoint : codePoints) {
		toc.push_back({ codePoint, Detail::checkedFileOffset(left.device.tell(), "glyph offset") });
		writeStoredCharacter(left, right.glyphs.at(codePoint), right.version, legacySvgAdvances);
	}

	const long endPosition = left.device.tell();
	Detail::seekAbsolute(left.device, tocPosition, "glyph table of contents");
	left << toc;
	Detail::seekAbsolute(left.device, endPosition, "end of font face");
	return left;
}

/**
 * @brief Reads a complete FontPacker preprocessed font face.
 * @details The importer reads the TOC and kerning block first, then seeks to
 * each absolute glyph offset to import stored glyph payloads. Versions 1, 2 and
 * 3 are accepted; version 1 files get the nominal distance ranges reconstructed
 * in memory, and versions 1/2 advances are converted from 26.6 to float pixels.
 * @tparam IoType Elv device-like type wrapped by the DataStream.
 * @param left Big-endian FontPacker data stream.
 * @param right Destination font face.
 * @return Reference to the stream.
 * @throws std::overflow_error if an offset cannot be represented for seeking.
 * @throws std::runtime_error if the magic/version is unsupported or required seeking fails.
 */
template <Elv::Io::DeviceLike IoType>
inline FontDataStream<IoType>& operator>>(FontDataStream<IoType>& left, PreprocessedFontFace& right)
{
	std::array<char, 4> magic {};
	PreprocessedFontFace decoded;
	left
		>> magic
		>> decoded.version;
	Detail::validateMagic(magic, FontFaceMagic, "PreprocessedFontFace");
	Detail::validateVersion(decoded.version, FontFaceMinVersion, FontFaceVersion, "PreprocessedFontFace");

	left
		>> decoded.familyName
		>> decoded.type
		>> decoded.distType
		>> decoded.bitmapSize
		>> decoded.bitmapLogicalSize
		>> decoded.bitmapPadding
		>> decoded.hasVert
		>> decoded.imageFormat
		>> decoded.ascender
		>> decoded.descender
		>> decoded.faceHeight
		>> decoded.maxAdvance
		>> decoded.unitsPerEm;
	// FontPacker's reader forces the last byte of the fixed-width tag to NUL.
	decoded.imageFormat.back() = '\0';
	if (decoded.version >= 2) {
		left >> decoded.distanceRangeX >> decoded.distanceRangeY
			 >> decoded.encodingFlags >> decoded.midpointAdjustment;
	} else {
		// Version 1 did not store the ranges or the baked transforms.
		decoded.encodingFlags = 0;
		decoded.midpointAdjustment = 1.0f;
		if (decoded.bitmapLogicalSize > 0) {
			const float storedSize = static_cast<float>(decoded.storedBitmapSize());
			const float nominal = static_cast<float>(decoded.bitmapPadding) * storedSize
								  / static_cast<float>(decoded.bitmapLogicalSize);
			decoded.distanceRangeX = nominal;
			decoded.distanceRangeY = nominal;
		}
	}

	std::vector<GlyphTOCEntry> toc;
	left >> toc;

	left >> decoded.kerning;

	const bool legacySvgAdvances = decoded.hasLegacySvgAdvances();
	decoded.glyphs.reserve(toc.size());
	long endPosition = left.device.tell();
	for (const GlyphTOCEntry& entry : toc) {
		Detail::seekAbsolute(left.device, Detail::checkedSeekOffset(entry.offset, "glyph offset"), "glyph data");
		StoredCharacter character;
		readStoredCharacter(left, character, decoded.version, legacySvgAdvances);
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
