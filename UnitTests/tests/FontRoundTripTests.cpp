/**
 * @file FontRoundTripTests.cpp
 * @brief Round-trip tests for Lotte's FontPacker containers.
 *
 * The font faces and vector images Lotte streams are produced by FontPacker and consumed by the
 * text layout code, so a byte that changes between writer and reader is a silent rendering bug.
 * These tests build a small but fully populated face (metadata, kerning, one glyph with an SDF
 * payload) and a vector image with a mipmap, push them through `Elv::Io::DataStream`, and compare
 * every field that was written.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Lotte/GFX/LteResourceFactory.hpp>
#include <Lotte/LteKld.hpp>
#include <Lotte/Text/LteFontAtlas.hpp>
#include <Lotte/Text/LteFontFace.hpp>
#include <Lotte/Text/LteFontRepository.hpp>
#include <Lotte/Text/LteTextLayout.hpp>
#include <Lotte/VectorGraphic/LteStoredVectorImage.hpp>
#include <cstddef>
#include <cstdint>
#include <glm/vec2.hpp>
#include <memory>
#include <memory_resource>
#include <string>
#include <utility>
#include <vector>

namespace {

/** @brief Reads a big-endian uint32 from a raw byte buffer. */
std::uint32_t readU32(const std::vector<std::byte>& bytes, std::size_t offset)
{
	REQUIRE(offset + 4 <= bytes.size());
	return (std::uint32_t(std::to_integer<std::uint8_t>(bytes[offset])) << 24)
		 | (std::uint32_t(std::to_integer<std::uint8_t>(bytes[offset + 1])) << 16)
		 | (std::uint32_t(std::to_integer<std::uint8_t>(bytes[offset + 2])) << 8)
		 | std::uint32_t(std::to_integer<std::uint8_t>(bytes[offset + 3]));
}

/** @brief Fills a face with a value for every field the format stores. */
Lotte::Text::PreprocessedFontFace makeFace()
{
	using namespace Lotte::Text;
	PreprocessedFontFace face;
	face.familyName = "RoundTrip";
	face.type = SDFType::MSDF;
	face.distType = DistanceType::Euclidean;
	face.bitmapSize = 32;
	face.bitmapLogicalSize = 24;
	face.bitmapPadding = 2;
	face.hasVert = true;
	face.imageFormat = { 'P', 'N', 'G', '\0', '\0', '\0', '\0', '\0' };
	face.ascender = 1.5f;
	face.descender = -0.5f;
	face.faceHeight = 2.0f;
	face.maxAdvance = 3.0f;
	face.unitsPerEm = 1000;
	face.distanceRangeX = 2.0f;
	face.distanceRangeY = 2.5f;
	face.encodingFlags = static_cast<std::uint32_t>(EncodingFlag::Inverted)
					   | static_cast<std::uint32_t>(EncodingFlag::MidpointAdjusted);
	face.midpointAdjustment = 2.0f;
	face.kerning[65][66] = glm::fvec2(1.0f, 2.0f);

	StoredCharacter glyph;
	glyph.valid = true;
	glyph.width = 8;
	glyph.height = 9;
	glyph.bearingX = -1;
	glyph.bearingY = 2;
	glyph.advanceX = 7.0f;
	glyph.advanceY = 10.0f;
	glyph.metricWidth = 8.0f;
	glyph.metricHeight = 9.0f;
	glyph.horiBearingX = -1.0f;
	glyph.horiBearingY = 2.0f;
	glyph.horiAdvance = 7.0f;
	glyph.vertBearingX = 0.5f;
	glyph.vertBearingY = 1.0f;
	glyph.vertAdvance = 10.0f;
	glyph.sdf = { std::byte { 0x01 }, std::byte { 0x02 }, std::byte { 0x03 } };
	face.glyphs[65] = glyph;
	return face;
}

/** @brief Serializes @p face into a complete FontPacker byte stream. */
std::vector<std::byte> serializeFace(const Lotte::Text::PreprocessedFontFace& face)
{
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	Lotte::Text::FontDataStream<Euph::Io::MemoryDevice> stream(device);
	stream << face;
	return device.getBuffer();
}

/**
 * @brief Minimal Elv::Io::System that serves one in-memory font to every open.
 *
 * The tests must exercise the System-based overload without touching the host
 * filesystem or requiring a PhysFS mount. PhysFS itself lives in Euphemy; this test
 * double shows that Lotte only depends on the abstract seam.
 */
class MemorySystem final : public Elv::Io::System {
public:
	explicit MemorySystem(std::vector<std::byte> bytes)
		: bytes_(std::move(bytes))
	{
	}

	Elv::Io::Device* open(const char*, Elv::Io::Mode mode) override
	{
		return new Euph::Io::MemoryDevice(mode, std::vector<std::byte>(bytes_));
	}

	Elv::Io::uDevice openUnique(const char*, Elv::Io::Mode mode,
								std::pmr::memory_resource* memRes) override
	{
		return Elv::Util::pmr_make_unique<Euph::Io::MemoryDevice>(
			memRes, mode, std::vector<std::byte>(bytes_));
	}

	Elv::Io::sDevice openShared(const char*, Elv::Io::Mode mode,
								std::pmr::memory_resource* memRes) override
	{
		return Elv::Util::pmr_make_shared<Euph::Io::MemoryDevice>(
			memRes, mode, std::vector<std::byte>(bytes_));
	}

	bool exists(const char*) override { return true; }
	char separator() const override { return '/'; }
	void enumerate(const char*, bool, Elv::Io::System::FilenameCallback callback) override
	{
		(void)callback;
	}
	void enumerate(const char*, Elv::Io::System::FilesystemCallback callback) override
	{
		(void)callback;
	}
	bool isDirectory(const char*) override { return false; }
	bool isSymlink(const char*) override { return false; }
	bool isFile(const char*) override { return true; }
	bool mkdir(const char*) override { return false; }
	bool remove(const char*) override { return false; }

private:
	std::vector<std::byte> bytes_;
};

}

TEST_CASE("A preprocessed font face survives a DataStream round-trip", "[font][roundtrip]")
{
	using namespace Lotte::Text;

	const PreprocessedFontFace face = makeFace();

	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	FontDataStream<Euph::Io::MemoryDevice> stream(device);
	stream << face;
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

	PreprocessedFontFace decoded;
	stream >> decoded;

	CHECK(decoded.version == FontFaceVersion);
	CHECK(decoded.familyName == face.familyName);
	CHECK(decoded.type == face.type);
	CHECK(decoded.distType == face.distType);
	CHECK(decoded.bitmapSize == face.bitmapSize);
	CHECK(decoded.bitmapLogicalSize == face.bitmapLogicalSize);
	CHECK(decoded.bitmapPadding == face.bitmapPadding);
	CHECK(decoded.hasVert == face.hasVert);
	CHECK(decoded.ascender == face.ascender);
	CHECK(decoded.descender == face.descender);
	CHECK(decoded.faceHeight == face.faceHeight);
	CHECK(decoded.maxAdvance == face.maxAdvance);
	CHECK(decoded.unitsPerEm == face.unitsPerEm);
	CHECK(decoded.distanceRangeX == face.distanceRangeX);
	CHECK(decoded.distanceRangeY == face.distanceRangeY);
	CHECK(decoded.encodingFlags == face.encodingFlags);
	CHECK(decoded.midpointAdjustment == face.midpointAdjustment);
	CHECK(decoded.glyphs.size() == 1);
	REQUIRE(decoded.glyphs.count(65) == 1);

	const StoredCharacter& glyph = decoded.glyphs.at(65);
	CHECK(glyph.valid == face.glyphs.at(65).valid);
	CHECK(glyph.width == face.glyphs.at(65).width);
	CHECK(glyph.height == face.glyphs.at(65).height);
	CHECK(glyph.bearingX == face.glyphs.at(65).bearingX);
	CHECK(glyph.bearingY == face.glyphs.at(65).bearingY);
	CHECK(glyph.advanceX == face.glyphs.at(65).advanceX);
	CHECK(glyph.advanceY == face.glyphs.at(65).advanceY);
	CHECK(glyph.metricWidth == face.glyphs.at(65).metricWidth);
	CHECK(glyph.metricHeight == face.glyphs.at(65).metricHeight);
	CHECK(glyph.horiBearingX == face.glyphs.at(65).horiBearingX);
	CHECK(glyph.horiBearingY == face.glyphs.at(65).horiBearingY);
	CHECK(glyph.horiAdvance == face.glyphs.at(65).horiAdvance);
	CHECK(glyph.sdf == face.glyphs.at(65).sdf);
	REQUIRE(decoded.kerning.count(65) == 1);
	CHECK(decoded.kerning.at(65).at(66) == glm::fvec2(1.0f, 2.0f));
}

TEST_CASE("A stored vector image survives a DataStream round-trip", "[font][vectorgraphic][roundtrip]")
{
	using VectorImage = Lotte::VectorGraphic::StoredVectorImage;
	using VectorMipmap = Lotte::VectorGraphic::Mipmap;

	VectorImage image;
	image.processingSize = 64;
	image.actualSize = 48;
	image.padding = 4;
	image.logicalX = 1.0f;
	image.logicalY = 2.0f;
	image.logicalWidth = 3.0f;
	image.logicalHeight = 4.0f;
	image.aspectRatio = 0.75f;
	image.minX = -1.0f;
	image.maxX = 1.0f;
	image.minY = -2.0f;
	image.maxY = 2.0f;
	image.distanceRangeX = 4.0f;
	image.distanceRangeY = 8.0f;
	image.encodingFlags = static_cast<std::uint32_t>(Lotte::VectorGraphic::EncodingFlag::Inverted)
						| static_cast<std::uint32_t>(Lotte::VectorGraphic::EncodingFlag::MidpointAdjusted);
	image.midpointAdjustment = 0.5f;
	image.type = Lotte::Text::SDFType::SDF;
	image.distType = Lotte::Text::DistanceType::Manhattan;

	VectorMipmap mipmap;
	mipmap.data = { std::byte { 0xAA }, std::byte { 0xBB } };
	image.mipmaps.push_back(std::move(mipmap));

	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	Lotte::VectorGraphic::VectorGraphicDataStream<Euph::Io::MemoryDevice> stream(device);
	stream << image;
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);

	VectorImage decoded;
	stream >> decoded;

	CHECK(decoded.processingSize == image.processingSize);
	CHECK(decoded.actualSize == image.actualSize);
	CHECK(decoded.padding == image.padding);
	CHECK(decoded.logicalX == image.logicalX);
	CHECK(decoded.logicalY == image.logicalY);
	CHECK(decoded.logicalWidth == image.logicalWidth);
	CHECK(decoded.logicalHeight == image.logicalHeight);
	CHECK(decoded.aspectRatio == image.aspectRatio);
	CHECK(decoded.minX == image.minX);
	CHECK(decoded.maxX == image.maxX);
	CHECK(decoded.minY == image.minY);
	CHECK(decoded.maxY == image.maxY);
	CHECK(decoded.distanceRangeX == image.distanceRangeX);
	CHECK(decoded.distanceRangeY == image.distanceRangeY);
	CHECK(decoded.encodingFlags == image.encodingFlags);
	CHECK(decoded.midpointAdjustment == image.midpointAdjustment);
	CHECK(decoded.type == image.type);
	CHECK(decoded.distType == image.distType);
	REQUIRE(decoded.mipmaps.size() == 1);
	CHECK(decoded.mipmaps.front().data == image.mipmaps.front().data);
}

TEST_CASE("A version 3 font face has the documented byte layout", "[font][binary]")
{
	using namespace Lotte::Text;

	const PreprocessedFontFace face = makeFace();
	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	FontDataStream<Euph::Io::MemoryDevice> stream(device);
	stream << face;

	const std::vector<std::byte>& bytes = device.getBuffer();
	REQUIRE(bytes.size() > 120);

	// Magic + version + NUL-inclusive family name ("RoundTrip" is 9 bytes + NUL).
	CHECK(std::to_integer<int>(bytes[0]) == 'W');
	CHECK(std::to_integer<int>(bytes[1]) == 'O');
	CHECK(std::to_integer<int>(bytes[2]) == 'D');
	CHECK(std::to_integer<int>(bytes[3]) == 'F');
	CHECK(readU32(bytes, 4) == 3);
	CHECK(readU32(bytes, 8) == 10);
	CHECK(std::to_integer<int>(bytes[21]) == 0); // family-name NUL

	// type, distType, bitmap_size, bitmap_logical_size, bitmap_padding, hasVert.
	CHECK(std::to_integer<std::uint8_t>(bytes[22]) == 1); // MSDF
	CHECK(std::to_integer<std::uint8_t>(bytes[23]) == 1); // Euclidean
	CHECK(readU32(bytes, 24) == 32);
	CHECK(readU32(bytes, 28) == 24);
	CHECK(readU32(bytes, 32) == 2);
	CHECK(std::to_integer<std::uint8_t>(bytes[36]) == 1);

	// imageFormat is eight raw bytes, not a length-prefixed container.
	CHECK(std::to_integer<int>(bytes[37]) == 'P');
	CHECK(std::to_integer<int>(bytes[38]) == 'N');
	CHECK(std::to_integer<int>(bytes[39]) == 'G');
	for (std::size_t i = 40; i < 45; ++i)
		CHECK(std::to_integer<int>(bytes[i]) == 0);

	// Version 2+ header fields.
	CHECK(readU32(bytes, 65) == 0x40000000u); // distanceRangeX = 2.0f
	CHECK(readU32(bytes, 69) == 0x40200000u); // distanceRangeY = 2.5f
	CHECK(readU32(bytes, 73) == 0x9u);        // Inverted | MidpointAdjusted
	CHECK(readU32(bytes, 77) == 0x40000000u); // midpointAdjustment = 2.0f

	CHECK(readU32(bytes, 81) == 1); // charCount
	CHECK(readU32(bytes, 85) == 65); // TOC[0].codePoint
	const std::uint32_t glyphOffset = readU32(bytes, 89);

	REQUIRE(glyphOffset + 61 + 3 <= bytes.size());
	CHECK(std::to_integer<std::uint8_t>(bytes[glyphOffset]) == 1); // valid
	CHECK(readU32(bytes, glyphOffset + 1) == 8);   // width
	CHECK(readU32(bytes, glyphOffset + 5) == 9);   // height
	CHECK(readU32(bytes, glyphOffset + 9) == 0xFFFFFFFFu); // bearingX = -1
	CHECK(readU32(bytes, glyphOffset + 13) == 2);  // bearingY
	CHECK(readU32(bytes, glyphOffset + 17) == 0x40E00000u); // advanceX = 7.0f
	CHECK(readU32(bytes, glyphOffset + 21) == 0x41200000u); // advanceY = 10.0f
	CHECK(readU32(bytes, glyphOffset + 57) == 3); // sdfLength
	CHECK(std::to_integer<int>(bytes[glyphOffset + 61]) == 1);
	CHECK(std::to_integer<int>(bytes[glyphOffset + 62]) == 2);
	CHECK(std::to_integer<int>(bytes[glyphOffset + 63]) == 3);
}

TEST_CASE("Older font face versions keep their advance encoding", "[font][binary][version]")
{
	using namespace Lotte::Text;

	{
		// Version 2 still stores raw 26.6 advances; 7.25 px is exactly 464.
		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		FontDataStream<Euph::Io::MemoryDevice> stream(device);
		StoredCharacter glyph;
		glyph.valid = true;
		glyph.width = 1;
		glyph.height = 1;
		glyph.advanceX = 7.25f;
		glyph.advanceY = 10.0f;
		writeStoredCharacter(stream, glyph, 2, false);

		const std::vector<std::byte>& bytes = device.getBuffer();
		REQUIRE(bytes.size() >= 21);
		CHECK(readU32(bytes, 17) == 464);

		REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);
		StoredCharacter decoded;
		readStoredCharacter(stream, decoded, 2, false);
		CHECK(decoded.advanceX == 7.25f);
		CHECK(decoded.advanceY == 10.0f);
	}

	{
		// A version 2 face keeps its version and the 26.6 values across a rewrite.
		PreprocessedFontFace face = makeFace();
		face.version = 2;
		face.glyphs.at(65).advanceX = 7.25f;

		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		FontDataStream<Euph::Io::MemoryDevice> stream(device);
		stream << face;
		REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);
		PreprocessedFontFace decoded;
		stream >> decoded;

		CHECK(decoded.version == 2);
		CHECK(decoded.glyphs.at(65).advanceX == 7.25f);
		CHECK(decoded.distanceRangeX == face.distanceRangeX);
		CHECK(decoded.midpointAdjustment == face.midpointAdjustment);
	}

	{
		// Version 1 SVG faces store plain pixel advances, and the importer
		// reconstructs the nominal distance range from P, S and L.
		PreprocessedFontFace face = makeFace();
		face.version = 1;
		face.familyName = "SVG";
		face.bitmapSize = 12;
		face.bitmapLogicalSize = 24;
		face.bitmapPadding = 2;
		face.glyphs.at(65).advanceX = 7.0f;

		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		FontDataStream<Euph::Io::MemoryDevice> stream(device);
		stream << face;
		REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);
		PreprocessedFontFace decoded;
		stream >> decoded;

		CHECK(decoded.version == 1);
		CHECK(decoded.hasLegacySvgAdvances());
		CHECK(decoded.glyphs.at(65).advanceX == 7.0f);
		CHECK(decoded.encodingFlags == 0);
		CHECK(decoded.midpointAdjustment == 1.0f);
		CHECK(decoded.distanceRangeX == Catch::Approx(2.0f * 12.0f / 24.0f));
	}

	{
		// Version 1 may store bitmap_size = 0; writers must resolve it to L.
		PreprocessedFontFace face = makeFace();
		face.version = 1;
		face.bitmapSize = 0;
		face.bitmapLogicalSize = 24;
		CHECK(face.storedBitmapSize() == 24);

		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
		FontDataStream<Euph::Io::MemoryDevice> stream(device);
		stream << face;
		REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);
		PreprocessedFontFace decoded;
		stream >> decoded;
		CHECK(decoded.version == 1);
		CHECK(decoded.bitmapSize == 24);
	}
}

TEST_CASE("Glyph layout applies the SDF padding runway", "[font][layout]")
{
	using namespace Lotte::Text;

	PreprocessedFontFace face;
	face.familyName = "Layout";
	face.bitmapLogicalSize = 64;
	face.bitmapPadding = 8;
	face.bitmapSize = 16;
	face.faceHeight = 60.0f;
	face.ascender = 52.0f;
	face.descender = -8.0f;
	face.unitsPerEm = 2048;

	StoredCharacter glyph;
	glyph.valid = true;
	glyph.width = 8;
	glyph.height = 10;
	glyph.bearingX = 1;
	glyph.bearingY = 12;
	glyph.metricWidth = 8.0f;
	glyph.metricHeight = 10.0f;
	glyph.horiBearingX = 1.0f;
	glyph.horiBearingY = 12.0f;
	glyph.horiAdvance = 9.0f;
	glyph.advanceX = 9.0f;
	face.glyphs.emplace(std::uint32_t(U'A'), glyph);

	// The metrics were measured at em = L - P, not at faceHeight and not at L.
	CHECK(getReferencePixelSize(face) == 56.0f);
	CHECK(getGlyphPaddingRatio(face) == Catch::Approx(1.0f / 6.0f));

	TextAttributes attributes;
	attributes.size = 56.0f; // 1:1 with the stored metrics
	TextLayoutOptions options;
	options.origin = glm::fvec2(10.0f, 20.0f);
	auto lookup = [&face](const std::string&, bool) { return &face; };
	const TextLayout layout = layoutRuns({ TextRun{ U"AA\nA", attributes } }, lookup, options);
	REQUIRE(layout.glyphs.size() == 3);

	const PlacedGlyph& first = layout.glyphs.front();
	CHECK(first.contentSize.x == Catch::Approx(8.0f));
	CHECK(first.contentSize.y == Catch::Approx(10.0f));
	CHECK(first.position.x == Catch::Approx(10.0f + 1.0f - 8.0f / 6.0f));
	CHECK(first.position.y == Catch::Approx(20.0f - 12.0f - 10.0f / 6.0f));
	CHECK(first.size.x == Catch::Approx(8.0f + 2.0f * 8.0f / 6.0f));
	CHECK(first.size.y == Catch::Approx(10.0f + 2.0f * 10.0f / 6.0f));

	// The quad must be larger than the silhouette by P_final on all sides.
	CHECK(first.size.x - first.contentSize.x == Catch::Approx(2.0f * 8.0f / 6.0f));
	CHECK(first.size.y - first.contentSize.y == Catch::Approx(2.0f * 10.0f / 6.0f));

	CHECK(layout.glyphs[1].position.x == Catch::Approx(10.0f + 9.0f + 1.0f - 8.0f / 6.0f));
	// "AA\nA": the newline resets the pen, so the final line ends at one advance.
	CHECK(layout.endOffset.x == Catch::Approx(19.0f));
	CHECK(layout.endOffset.y == Catch::Approx(80.0f));
	CHECK(layout.height == Catch::Approx(88.0f)); // last baseline + descender

	// Italic shear follows the unpadded glyph height.
	TextAttributes italic = attributes;
	italic.italic = true;
	const TextLayout italicLayout = layoutRuns({ TextRun{ U"A", italic } }, lookup, options);
	REQUIRE(italicLayout.glyphs.size() == 1);
	CHECK(italicLayout.glyphs.front().shear == Catch::Approx(5.0f));
}

TEST_CASE("SVG-backed faces scale against the viewport, and hand-built faces fall back", "[font][layout]")
{
	using namespace Lotte::Text;

	{
		PreprocessedFontFace face;
		face.familyName = "SVG";
		face.bitmapLogicalSize = 64;
		face.bitmapPadding = 8;
		face.bitmapSize = 16;
		face.unitsPerEm = 200; // viewport height
		face.faceHeight = 200.0f;
		face.maxAdvance = 100.0f;

		StoredCharacter glyph;
		glyph.valid = true;
		glyph.width = 100;
		glyph.height = 50;
		glyph.metricWidth = 100.0f;
		glyph.metricHeight = 50.0f;
		glyph.horiBearingY = 50.0f;
		glyph.horiAdvance = 100.0f;
		glyph.advanceX = 100.0f;
		face.glyphs.emplace(std::uint32_t(U'A'), glyph);

		CHECK(getReferencePixelSize(face) == 200.0f);

		TextAttributes attributes;
		attributes.size = 200.0f;
		auto lookup = [&face](const std::string&, bool) { return &face; };
		const TextLayout layout = layoutRuns({ TextRun{ U"A", attributes } }, lookup);
		REQUIRE(layout.glyphs.size() == 1);
		// The viewport is scaled 1:1 into the padded quad.
		CHECK(layout.glyphs.front().contentSize.x == Catch::Approx(100.0f));
		CHECK(layout.glyphs.front().contentSize.y == Catch::Approx(50.0f));
		CHECK(layout.endOffset.x == Catch::Approx(100.0f));
	}

	{
		// No bitmap fields: the float metric box is the fallback, and zero padding
		// means no quad expansion.
		PreprocessedFontFace face;
		face.familyName = "Fallback";
		face.bitmapLogicalSize = 64;
		face.bitmapPadding = 0;

		StoredCharacter glyph;
		glyph.valid = true;
		glyph.metricWidth = 8.0f;
		glyph.metricHeight = 10.0f;
		glyph.horiBearingX = 1.0f;
		glyph.horiBearingY = 12.0f;
		glyph.advanceX = 9.0f;
		face.glyphs.emplace(std::uint32_t(U'A'), glyph);

		CHECK(getReferencePixelSize(face) == 64.0f);
		CHECK(getGlyphPaddingRatio(face) == 0.0f);

		TextAttributes attributes;
		attributes.size = 64.0f;
		TextLayoutOptions options;
		options.origin = glm::fvec2(10.0f, 20.0f);
		auto lookup = [&face](const std::string&, bool) { return &face; };
		const TextLayout layout = layoutRuns({ TextRun{ U"A", attributes } }, lookup, options);
		REQUIRE(layout.glyphs.size() == 1);
		CHECK(layout.glyphs.front().position.x == Catch::Approx(11.0f));
		CHECK(layout.glyphs.front().position.y == Catch::Approx(8.0f));
		CHECK(layout.glyphs.front().size.x == Catch::Approx(8.0f));
		CHECK(layout.glyphs.front().size.y == Catch::Approx(10.0f));
		CHECK(layout.endOffset.x == Catch::Approx(19.0f));
	}
}

TEST_CASE("Stored vector image geometry follows the padding contract", "[vectorgraphic]")
{
	using namespace Lotte::VectorGraphic;
	using Lotte::Text::EncodingFlag;

	StoredVectorImage image;
	CHECK(image.midpointAdjustment == 1.0f);

	image.processingSize = 1024;
	image.actualSize = 64;
	image.padding = 100;
	image.logicalWidth = 100.0f;
	image.logicalHeight = 50.0f;
	image.minX = -2.0f;
	image.maxX = 2.0f;
	image.minY = -1.0f;
	image.maxY = 1.0f;
	image.encodingFlags = static_cast<std::uint32_t>(EncodingFlag::Inverted)
						| static_cast<std::uint32_t>(EncodingFlag::MidpointAdjusted);

	CHECK(image.hasEncodingFlag(EncodingFlag::Inverted));
	CHECK(image.hasEncodingFlag(EncodingFlag::MidpointAdjusted));
	CHECK(!image.hasEncodingFlag(EncodingFlag::GammaCorrected));
	CHECK(image.paddingInTexels() == Catch::Approx(6.25f)); // P * S / L
	CHECK(image.logicalSize().x == Catch::Approx(100.0f));
	CHECK(image.logicalSize().y == Catch::Approx(50.0f));
	CHECK(image.sourceSize().x == Catch::Approx(4.0f));
	CHECK(image.sourceSize().y == Catch::Approx(2.0f));

	const float pad = 100.0f / 1024.0f;
	const float span = 1.0f - 2.0f * pad;
	const glm::fvec2 uv = image.sourceToUv(glm::fvec2(0.0f, 0.0f));
	CHECK(uv.x == Catch::Approx(pad + 0.5f * span));
	CHECK(uv.y == Catch::Approx(pad + 0.5f * span));

	const glm::fvec2 source = image.uvToSource(uv);
	CHECK(source.x == Catch::Approx(0.0f));
	CHECK(source.y == Catch::Approx(0.0f));

	// The padding runway maps to source coordinates outside [min, max].
	CHECK(image.uvToSource(glm::fvec2(0.0f, 0.0f)).x
		  == Catch::Approx(-2.0f + (0.0f - pad) / span * 4.0f));
}

TEST_CASE("Image format tags and the midpoint flag follow the container rules", "[font][vectorgraphic][binary]")
{
	using namespace Lotte::Text;

	PreprocessedFontFace face = makeFace();
	CHECK(face.imageFormatName() == "PNG");
	face.setImageFormat(" webp ");
	CHECK(face.imageFormatName() == "WEBP");
	CHECK(face.imageFormat.back() == '\0');
	face.setImageFormat("toolongname");
	CHECK(face.imageFormatName() == "TOOLONG");
	face.setImageFormat("");
	CHECK(face.imageFormatName() == "PNG");

	// A non-terminated tag is repaired by the writer and by the reader.
	face.imageFormat = { 'P', 'N', 'G', '\0', '\0', '\0', '\0', 'X' };
	// Without the midpoint flag, the divisor is written as 1.0.
	face.encodingFlags = static_cast<std::uint32_t>(EncodingFlag::Inverted);
	face.midpointAdjustment = 9.0f;

	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ_WRITE);
	FontDataStream<Euph::Io::MemoryDevice> stream(device);
	stream << face;
	REQUIRE(device.seek(0, Elv::Io::SeekOrigin::SET) == 0);
	PreprocessedFontFace decoded;
	stream >> decoded;

	CHECK(decoded.imageFormatName() == "PNG");
	CHECK(decoded.imageFormat.back() == '\0');
	CHECK(decoded.encodingFlags == static_cast<std::uint32_t>(EncodingFlag::Inverted));
	CHECK(decoded.midpointAdjustment == 1.0f);

	// The same normalization applies to a standalone vector image.
	Lotte::VectorGraphic::StoredVectorImage image;
	image.encodingFlags = static_cast<std::uint32_t>(Lotte::VectorGraphic::EncodingFlag::Inverted);
	image.midpointAdjustment = 9.0f;
	image.imageFormat = { 'p', 'n', 'g', '\0', '\0', '\0', '\0', 'X' };
	image.mipmaps.emplace_back();

	Euph::Io::MemoryDevice vectorDevice(Elv::Io::Mode::READ_WRITE);
	Lotte::VectorGraphic::VectorGraphicDataStream<Euph::Io::MemoryDevice> vectorStream(vectorDevice);
	vectorStream << image;
	REQUIRE(vectorDevice.seek(0, Elv::Io::SeekOrigin::SET) == 0);
	Lotte::VectorGraphic::StoredVectorImage decodedImage;
	vectorStream >> decodedImage;
	CHECK(decodedImage.midpointAdjustment == 1.0f);
	CHECK(decodedImage.imageFormat.back() == '\0');
}

TEST_CASE("Font faces load through Elv::Io::System or an open device", "[font][io]")
{
	using namespace Lotte::Text;

	const PreprocessedFontFace face = makeFace();
	const std::vector<std::byte> bytes = serializeFace(face);

	Euph::Io::MemoryDevice device(Elv::Io::Mode::READ, std::vector<std::byte>(bytes));
	const std::shared_ptr<PreprocessedFontFace> fromDevice = loadFontFace(device);
	REQUIRE(fromDevice != nullptr);
	CHECK(fromDevice->familyName == face.familyName);

	MemorySystem system(bytes);
	const std::shared_ptr<PreprocessedFontFace> fromSystem =
		loadFontFace(system, "fonts/roundtrip.wodf");
	REQUIRE(fromSystem != nullptr);
	CHECK(fromSystem->familyName == face.familyName);

	Lotte::CommandBuffer dispatcher;
	Lotte::Gfx::ResourceFactory factory(dispatcher);
	FontRepository repository(factory);

	Euph::Io::MemoryDevice deviceForRepository(Elv::Io::Mode::READ,
											  std::vector<std::byte>(bytes));
	const std::shared_ptr<const PreprocessedFontFace> byDevice =
		repository.addFaceFromDevice("DeviceFace", deviceForRepository);
	REQUIRE(byDevice != nullptr);
	CHECK(repository.face("DeviceFace") == byDevice.get());

	const std::shared_ptr<const PreprocessedFontFace> byFile =
		repository.addFaceFromFile("FileFace", system, "fonts/roundtrip.wodf");
	REQUIRE(byFile != nullptr);
	CHECK(repository.face("FileFace") == byFile.get());
	CHECK(repository.faceCount() == 2);
}
