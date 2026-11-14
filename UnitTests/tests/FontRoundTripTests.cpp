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
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Lotte/Text/LteFontFace.hpp>
#include <Lotte/VectorGraphic/LteStoredVectorImage.hpp>
#include <cstddef>
#include <glm/vec2.hpp>

namespace {

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
	face.kerning[65][66] = glm::fvec2(1.0f, 2.0f);

	StoredCharacter glyph;
	glyph.valid = true;
	glyph.width = 8;
	glyph.height = 9;
	glyph.bearingX = -1;
	glyph.bearingY = 2;
	glyph.advanceX = 7 * 64;
	glyph.advanceY = 10 * 64;
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
	CHECK(decoded.glyphs.size() == 1);
	REQUIRE(decoded.glyphs.count(65) == 1);

	const StoredCharacter& glyph = decoded.glyphs.at(65);
	CHECK(glyph.valid == face.glyphs.at(65).valid);
	CHECK(glyph.width == face.glyphs.at(65).width);
	CHECK(glyph.height == face.glyphs.at(65).height);
	CHECK(glyph.bearingX == face.glyphs.at(65).bearingX);
	CHECK(glyph.bearingY == face.glyphs.at(65).bearingY);
	CHECK(glyph.advanceX == face.glyphs.at(65).advanceX);
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
	image.encodingFlags = static_cast<std::uint32_t>(Lotte::VectorGraphic::EncodingFlag::Inverted);
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
