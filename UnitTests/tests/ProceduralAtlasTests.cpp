/**
 * @file ProceduralAtlasTests.cpp
 * @brief Checks the procedural GUI atlas: determinism, manifest coverage, shading.
 *
 * The atlas builder is the piece that turns the ported SDF/material/lighting code
 * into something a theme can use, so the checks are about its contract rather than
 * about how a particular pixel looks: every part has a rectangle, the same settings
 * reproduce the same bytes, a different seed does not, and the shaded image carries
 * the alpha and colour the compositor needs.
 */
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Procedural/LteGuiAtlas.hpp>
#include <Lotte/Procedural/LteGuiMaterials.hpp>
#include <Lotte/Procedural/LteSdf.hpp>
#include <Lotte/Procedural/LteSurfaceCanvas.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

namespace {

using namespace Lotte::Procedural;

/// @brief Counts pixels with a non-zero alpha.
std::size_t coveredPixels(const Lotte::Gfx::Rgba8Image& image)
{
	std::size_t covered = 0;
	for (std::size_t index = 3; index < image.pixels.size(); index += 4) {
		if (image.pixels[index] > 8) ++covered;
	}
	return covered;
}

} // namespace

TEST_CASE("Procedural atlas: every part has a rectangle and the image is shaded", "[procedural]")
{
	GuiAtlasSettings settings;
	settings.seed = 4242;
	settings.atlasWidth = 512;
	const GuiAtlas atlas = buildGuiAtlas(settings);

	REQUIRE(atlas.image.isValid());
	REQUIRE(atlas.parts.size() == GuiAtlasPartCount);
	for (std::size_t index = 0; index < GuiAtlasPartCount; ++index) {
		REQUIRE(atlas.find(static_cast<GuiAtlasPart>(index)) != nullptr);
		const GuiAtlasRect* rect = atlas.find(static_cast<GuiAtlasPart>(index));
		REQUIRE(rect->width > 0);
		REQUIRE(rect->height > 0);
		REQUIRE(rect->x + rect->width <= atlas.image.width);
		REQUIRE(rect->y + rect->height <= atlas.image.height);
	}
	// A shaded atlas is mostly transparent (the margins) but not empty.
	const std::size_t covered = coveredPixels(atlas.image);
	REQUIRE(covered > 0);
	REQUIRE(covered < atlas.image.pixels.size() / 4);
}

TEST_CASE("Procedural atlas: the same settings reproduce the same bytes", "[procedural]")
{
	GuiAtlasSettings settings;
	settings.seed = 77;
	settings.atlasWidth = 256;
	settings.panelWidth = 64;
	settings.panelHeight = 64;
	settings.buttonWidth = 48;
	settings.buttonHeight = 24;
	settings.parchmentWidth = 64;
	settings.parchmentHeight = 64;
	const GuiAtlas first = buildGuiAtlas(settings);
	const GuiAtlas second = buildGuiAtlas(settings);
	REQUIRE(first.image.width == second.image.width);
	REQUIRE(first.image.height == second.image.height);
	REQUIRE(first.image.pixels == second.image.pixels);

	settings.seed = 78;
	const GuiAtlas other = buildGuiAtlas(settings);
	REQUIRE(other.image.pixels != first.image.pixels);
}

TEST_CASE("Procedural atlas: the manifest names round-trip", "[procedural]")
{
	for (std::size_t index = 0; index < GuiAtlasPartCount; ++index) {
		const auto part = static_cast<GuiAtlasPart>(index);
		GuiAtlasPart parsed = GuiAtlasPart::Count;
		REQUIRE(parseGuiAtlasPart(guiAtlasPartName(part), parsed));
		REQUIRE(parsed == part);
	}
	GuiAtlasPart unused = GuiAtlasPart::Count;
	REQUIRE_FALSE(parseGuiAtlasPart("not_a_part", unused));
	REQUIRE_FALSE(parseGuiAtlasPart("", unused));
}

TEST_CASE("Procedural canvas: a painted layer shades to a covered image", "[procedural]")
{
	SurfaceCanvas canvas(32, 32);
	REQUIRE(canvas.isValid());
	const SurfaceMaterial material = flatMaterial(glm::fvec3(0.5f, 0.25f, 0.125f));
	canvas.paint(roundedBox(glm::fvec2(4.0f, 4.0f), glm::fvec2(28.0f, 28.0f), 3.0f), material);

	const Lotte::Gfx::Rgba8Image image = canvas.shade();
	REQUIRE(image.isValid());
	// The centre is covered and opaque; the corner is not.
	const std::size_t centre = (std::size_t(16) * 32 + 16) * 4;
	const std::size_t corner = 0;
	REQUIRE(image.pixels[centre + 3] > 200);
	REQUIRE(image.pixels[corner + 3] < 16);

	const Euph::Media::Image::DecodeTarget target = toDecodeTarget(image);
	REQUIRE(target.getFormat() == Euph::Media::Image::Format::RGBA8U);
	REQUIRE(target.getFrames().size() == 1);
	REQUIRE(target.getFrame(0).width == 32);
}

TEST_CASE("Procedural materials: the crack field is a real Voronoi border", "[procedural]")
{
	const GuiField cracks = makeVoronoiCrackField(64, 64, 16, 5u);
	REQUIRE(cracks.isValid());
	float minimum = 1.0f;
	float maximum = 0.0f;
	std::set<int> buckets;
	for (const float value : cracks.samples) {
		REQUIRE(value >= 0.0f);
		REQUIRE(value <= 1.0f);
		minimum = std::min(minimum, value);
		maximum = std::max(maximum, value);
		buckets.insert(static_cast<int>(value * 4.0f));
	}
	// A border field spans most of its range and is not a constant.
	REQUIRE(minimum < 0.05f);
	REQUIRE(maximum > 0.5f);
	REQUIRE(buckets.size() >= 3);
	// Sampling outside the field clamps instead of reading out of bounds.
	REQUIRE(cracks.sample(glm::fvec2(-5.0f, -5.0f)) == cracks.samples.front());
}
