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

/**
 * @brief Mean absolute luminance difference between neighbours one pixel apart.
 * @param image Shaded atlas.
 * @param rect Part to measure.
 * @param stepX Horizontal step of the sampling direction.
 * @param stepY Vertical step of the sampling direction.
 * @return Roughness per pixel of step length.
 *
 * The outer 24 px of a part are skipped so the torn edge and the rope frame stay out of
 * the measurement.
 */
double directionalRoughness(const Lotte::Gfx::Rgba8Image& image, const GuiAtlasRect& rect,
							int stepX, int stepY)
{
	const auto luminance = [&image](std::uint32_t x, std::uint32_t y) {
		const std::size_t index = (std::size_t(y) * image.width + x) * 4u;
		return (double(image.pixels[index]) + double(image.pixels[index + 1]) +
				double(image.pixels[index + 2])) /
			   3.0;
	};
	double total = 0.0;
	std::size_t count = 0;
	for (std::uint32_t y = rect.y + 24; y + 24 < rect.y + rect.height; ++y) {
		for (std::uint32_t x = rect.x + 24; x + 24 < rect.x + rect.width; ++x) {
			total += std::fabs(luminance(x, y) -
							  luminance(std::uint32_t(int(x) + stepX), std::uint32_t(int(y) + stepY)));
			++count;
		}
	}
	const double length = std::sqrt(double(stepX) * stepX + double(stepY) * stepY);
	return count > 0 ? total / double(count) / length : 0.0;
}

/**
 * @brief Fraction of a part's rectangle that carries a visible alpha.
 * @param image Shaded atlas.
 * @param rect Part to measure.
 * @return Covered fraction in `[0, 1]`.
 *
 * A part whose recipe silently painted nothing is all margin, so this is the cheap way to
 * tell "the manifest lists it" from "the builder drew it".
 */
double partCoverage(const Lotte::Gfx::Rgba8Image& image, const GuiAtlasRect& rect)
{
	std::size_t covered = 0;
	for (std::uint32_t y = rect.y; y < rect.y + rect.height; ++y) {
		for (std::uint32_t x = rect.x; x < rect.x + rect.width; ++x) {
			if (image.pixels[(std::size_t(y) * image.width + x) * 4u + 3u] > 8) ++covered;
		}
	}
	return double(covered) / (double(rect.width) * double(rect.height));
}

/**
 * @brief Mean colour of the covered pixels inside one part.
 * @param image Shaded atlas.
 * @param rect Part to average.
 * @return RGB averages in `[0, 255]`; zero when the part is empty.
 */
glm::dvec3 meanColour(const Lotte::Gfx::Rgba8Image& image, const GuiAtlasRect& rect)
{
	double sum[3] = { 0.0, 0.0, 0.0 };
	std::size_t count = 0;
	for (std::uint32_t y = rect.y; y < rect.y + rect.height; ++y) {
		for (std::uint32_t x = rect.x; x < rect.x + rect.width; ++x) {
			const std::size_t index = (std::size_t(y) * image.width + x) * 4u;
			if (image.pixels[index + 3u] <= 8) continue;
			for (std::size_t channel = 0; channel < 3; ++channel) {
				sum[channel] += double(image.pixels[index + channel]);
			}
			++count;
		}
	}
	if (count == 0) return glm::dvec3(0.0);
	return glm::dvec3(sum[0], sum[1], sum[2]) / double(count);
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

TEST_CASE("Procedural SDF: a rounded box reports its interior distance", "[procedural]")
{
	const SdfFunction box = roundedBox(glm::fvec2(0.0f, 0.0f), glm::fvec2(100.0f, 40.0f), 5.0f);
	// Centre: 20 px from the nearest edge, negative because it is inside.
	REQUIRE(std::fabs(box(glm::fvec2(50.0f, 20.0f)) + 20.0f) < 1.0e-3f);
	// Five pixels inside the flat edges.
	REQUIRE(std::fabs(box(glm::fvec2(50.0f, 35.0f)) + 5.0f) < 1.0e-3f);
	REQUIRE(std::fabs(box(glm::fvec2(5.0f, 20.0f)) + 5.0f) < 1.0e-3f);
	// Outside the flat edges and outside the rounded corner.
	REQUIRE(std::fabs(box(glm::fvec2(50.0f, -3.0f)) - 3.0f) < 1.0e-3f);
	REQUIRE(std::fabs(box(glm::fvec2(-5.0f, 20.0f)) - 5.0f) < 1.0e-3f);
	REQUIRE(std::fabs(box(glm::fvec2(0.0f, 0.0f)) - 2.071068f) < 1.0e-4f);
}

TEST_CASE("Procedural SDF: outline() paints a frame, not the whole canvas", "[procedural]")
{
	// The parchment hatch came from exactly this: the rounded box used to report `-radius`
	// for every interior point, `outline()` therefore covered the whole sheet, and the rope
	// band's diagonal weave was blended over the face at full coverage.
	const SdfFunction box = roundedBox(glm::fvec2(20.0f, 20.0f), glm::fvec2(140.0f, 100.0f), 2.0f);
	const SdfFunction band = outline(box, 7.0f, 0.0f);
	REQUIRE(band(glm::fvec2(80.0f, 60.0f)) > 0.5f);
	// On the outline is inside the band: the band is where `abs(box) < 3.5`.
	REQUIRE(band(glm::fvec2(80.0f, 20.0f)) < 0.5f);
	REQUIRE(band(glm::fvec2(20.0f, 60.0f)) < 0.5f);

	SurfaceCanvas canvas(160, 120);
	canvas.paint(band, flatMaterial(glm::fvec3(0.8f, 0.6f, 0.4f)));
	const Lotte::Gfx::Rgba8Image image = canvas.shade(ShadeOptions{});
	REQUIRE(image.isValid());
	const std::size_t covered = coveredPixels(image);
	REQUIRE(covered > 0);
	// Before the SDF fix the whole canvas matched and every pixel was covered;
	// a frame around a 120x80 box with a 7 px stroke is a third of it.
	REQUIRE(covered < std::size_t(image.width) * image.height / 2);
	// The middle of the sheet stays empty; the frame itself is opaque.
	REQUIRE(image.pixels[(std::size_t(60) * image.width + 80) * 4 + 3] == 0);
	REQUIRE(image.pixels[(std::size_t(60) * image.width + 20) * 4 + 3] > 128);
}

TEST_CASE("Procedural atlas: the parchment face is not a diagonal grating", "[procedural]")
{
	GuiAtlasSettings settings;
	settings.seed = 4242;
	settings.atlasWidth = 512;
	const GuiAtlas atlas = buildGuiAtlas(settings);
	const GuiAtlasRect* parchment = atlas.find(GuiAtlasPart::Parchment);
	REQUIRE(parchment != nullptr);
	const double horizontal = directionalRoughness(atlas.image, *parchment, 1, 0);
	const double vertical = directionalRoughness(atlas.image, *parchment, 0, 1);
	const double forward = directionalRoughness(atlas.image, *parchment, 1, 1);
	const double backward = directionalRoughness(atlas.image, *parchment, 1, -1);
	CAPTURE(horizontal, vertical, forward, backward);
	const double largest = std::max({ horizontal, vertical, forward, backward });
	const double smallest = std::min({ horizontal, vertical, forward, backward });
	REQUIRE(smallest > 0.0);
	// A full-face `sin((x + y) * 0.82)` grating makes one diagonal an order of magnitude
	// rougher than the other; a paper face is within a factor of about 1.6.
	REQUIRE(largest / smallest < 3.0);
}

TEST_CASE("Procedural atlas: every element type the reference forges drew is generated", "[procedural]")
{
	GuiAtlasSettings settings;
	settings.seed = 4242;
	const GuiAtlas atlas = buildGuiAtlas(settings);
	// `claude/ui_forge.cpp` plus the three notes draw these types on top of the panels,
	// buttons, slots, sliders, checkboxes and parchment this builder already had. Each one
	// needs a rectangle of its own *and* pixels in it: a recipe that silently paints
	// nothing would otherwise pass the manifest check.
	const GuiAtlasPart types[] = {
		GuiAtlasPart::RadioEmpty,   GuiAtlasPart::RadioChecked, GuiAtlasPart::ArrowUp,
		GuiAtlasPart::ArrowDown,    GuiAtlasPart::TriDown,      GuiAtlasPart::ScrollTrack,
		GuiAtlasPart::ScrollThumb,  GuiAtlasPart::TabActive,    GuiAtlasPart::TabInactive,
		GuiAtlasPart::FrameText,     GuiAtlasPart::SlotGroup,    GuiAtlasPart::BorderStrip,
		GuiAtlasPart::GemTile,
	};
	for (const GuiAtlasPart part : types) {
		CAPTURE(guiAtlasPartName(part));
		const GuiAtlasRect* rect = atlas.find(part);
		REQUIRE(rect != nullptr);
		REQUIRE(rect->width > 0);
		REQUIRE(rect->height > 0);
		REQUIRE(partCoverage(atlas.image, *rect) > 0.25);
	}

	// The slot group is a container, so it has to be larger than the wells it holds, and the
	// scroll track is a *vertical* groove.
	const GuiAtlasRect* group = atlas.find(GuiAtlasPart::SlotGroup);
	const GuiAtlasRect* slot = atlas.find(GuiAtlasPart::Slot);
	REQUIRE(group->width > slot->width * 2);
	REQUIRE(group->height > slot->height);
	const GuiAtlasRect* track = atlas.find(GuiAtlasPart::ScrollTrack);
	REQUIRE(track->height > track->width);
	// The two radio states share a recipe but must not shade to the same bytes.
	const GuiAtlasRect* empty = atlas.find(GuiAtlasPart::RadioEmpty);
	const GuiAtlasRect* checked = atlas.find(GuiAtlasPart::RadioChecked);
	REQUIRE(empty->width == checked->width);
	REQUIRE(empty->height == checked->height);
	// The two radio states share a recipe but must not shade to the same bytes: sample the
	// middle of each part, where the empty one is a dark well and the selected one a gem.
	const auto centreGreen = [&atlas](const GuiAtlasRect& rect) {
		const std::uint32_t x = rect.x + rect.width / 2;
		const std::uint32_t y = rect.y + rect.height / 2;
		return atlas.image.pixels[(std::size_t(y) * atlas.image.width + x) * 4u + 1u];
	};
	REQUIRE(centreGreen(*empty) != centreGreen(*checked));
}

TEST_CASE("Procedural atlas: the CSS bevel and tint reach the generated skin", "[procedural][css]")
{
	GuiAtlasSettings plain;
	plain.seed = 4242;
	const GuiAtlas baseline = buildGuiAtlas(plain);

	// What `resolveThemeCss` hands over for `border-width: 6px; background-color: #...`.
	GuiAtlasSettings themed = plain;
	themed.bevelWidth = 6.0f;
	themed.faceTint = glm::fvec3(0.870f, 0.894f, 1.0f);
	const GuiAtlas tinted = buildGuiAtlas(themed);
	REQUIRE(tinted.image.isValid());
	REQUIRE(tinted.image.pixels != baseline.image.pixels);

	const GuiAtlasRect* panel = baseline.find(GuiAtlasPart::Panel);
	const GuiAtlasRect* tintedPanel = tinted.find(GuiAtlasPart::Panel);
	REQUIRE(panel != nullptr);
	REQUIRE(tintedPanel != nullptr);
	REQUIRE(tintedPanel->width == panel->width);
	const glm::dvec3 before = meanColour(baseline.image, *panel);
	const glm::dvec3 after = meanColour(tinted.image, *tintedPanel);
	CAPTURE(before.x, before.y, before.z, after.x, after.y, after.z);
	// A cool tint multiplies red down more than blue, so the blue/red ratio has to rise.
	REQUIRE(after.z / after.x > before.z / before.x);
}
