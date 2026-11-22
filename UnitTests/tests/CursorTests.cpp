/**
 * @file CursorTests.cpp
 * @brief Origin resolution of `Lotte::Gui::Cursor` and its alpha crawl.
 *
 * Everything under test here is pure CPU math: a coverage mask, nine anchors and
 * five crawl directions. Keeping the search independent of image decoders and of a
 * backend is the point of `AlphaCoverage`, so the tests build masks by hand.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Lotte/Gui/LteCursor.hpp>
#include <cstdint>
#include <utility>
#include <vector>

namespace {

/** @brief Builds a mask from a list of opaque pixel coordinates. */
Lotte::Gui::AlphaCoverage maskFromPixels(std::uint16_t width, std::uint16_t height,
					 std::initializer_list<std::pair<int, int> > opaque)
{
	std::vector<std::uint8_t> alpha(std::size_t(width) * std::size_t(height), 0);
	for (const auto& pixel : opaque) {
		if (pixel.first >= 0 && pixel.second >= 0 && pixel.first < width &&
		    pixel.second < height) {
			alpha[std::size_t(pixel.second) * width + pixel.first] = 255;
		}
	}
	return Lotte::Gui::AlphaCoverage(width, height, std::move(alpha));
}

} // namespace

TEST_CASE("Every cursor anchor has a fraction and a matching default crawl",
	  "[gui][cursor]")
{
	using namespace Lotte::Gui;
	struct Expectation {
		CursorAnchor anchor;
		glm::fvec2 fraction;
		CursorCrawlMode crawl;
	};
	const Expectation expectations[] = {
		{ CursorAnchor::TopLeft, glm::fvec2(0.0f, 0.0f), CursorCrawlMode::Diagonal },
		{ CursorAnchor::Top, glm::fvec2(0.5f, 0.0f), CursorCrawlMode::Vertical },
		{ CursorAnchor::TopRight, glm::fvec2(1.0f, 0.0f), CursorCrawlMode::Diagonal },
		{ CursorAnchor::Left, glm::fvec2(0.0f, 0.5f), CursorCrawlMode::Horizontal },
		{ CursorAnchor::Center, glm::fvec2(0.5f, 0.5f), CursorCrawlMode::Radial },
		{ CursorAnchor::Right, glm::fvec2(1.0f, 0.5f), CursorCrawlMode::Horizontal },
		{ CursorAnchor::BottomLeft, glm::fvec2(0.0f, 1.0f), CursorCrawlMode::Diagonal },
		{ CursorAnchor::Bottom, glm::fvec2(0.5f, 1.0f), CursorCrawlMode::Vertical },
		{ CursorAnchor::BottomRight, glm::fvec2(1.0f, 1.0f), CursorCrawlMode::Diagonal }
	};
	for (const Expectation& expectation : expectations) {
		const glm::fvec2 fraction = cursorAnchorFraction(expectation.anchor);
		CHECK(fraction.x == Catch::Approx(expectation.fraction.x));
		CHECK(fraction.y == Catch::Approx(expectation.fraction.y));
		CHECK(defaultCrawlFor(expectation.anchor) == expectation.crawl);
		// Names exist for every enumerator and are stable strings.
		CHECK(cursorAnchorName(expectation.anchor) != nullptr);
	}
	CHECK(cursorCrawlModeName(CursorCrawlMode::Radial) != nullptr);
}

TEST_CASE("AlphaCoverage validates its storage and reads RGBA alpha", "[gui][cursor]")
{
	using namespace Lotte::Gui;

	std::vector<std::uint8_t> rgba(std::size_t(4) * 3 * 4, 0);
	// Opaque pixel at (2, 1): RGBA bytes start at ((1 * 4) + 2) * 4.
	const std::size_t opaqueIndex = (std::size_t(1) * 4 + 2) * 4;
	rgba[opaqueIndex + 0] = 10;
	rgba[opaqueIndex + 1] = 20;
	rgba[opaqueIndex + 2] = 30;
	rgba[opaqueIndex + 3] = 255;

	const AlphaCoverage coverage = AlphaCoverage::fromRgba8(rgba.data(), 4, 3);
	REQUIRE_FALSE(coverage.empty());
	CHECK(coverage.width() == 4);
	CHECK(coverage.height() == 3);
	CHECK(coverage.alpha().size() == 12);
	CHECK(coverage.isOpaque(2, 1, 0.5f));
	CHECK_FALSE(coverage.isOpaque(0, 0, 0.5f));
	// Out-of-range coordinates are always transparent instead of reading out of bounds.
	CHECK_FALSE(coverage.isOpaque(4, 1, 0.5f));

	// A short alpha vector must not produce a half-initialized mask.
	const AlphaCoverage truncated(4, 3, std::vector<std::uint8_t>(5, 255));
	CHECK(truncated.empty());
	CHECK(AlphaCoverage::fromRgba8(nullptr, 4, 3).empty());
}

TEST_CASE("Corner anchors crawl along their own diagonal", "[gui][cursor]")
{
	using namespace Lotte::Gui;
	// 5x5 with an opaque diagonal from (2,2) to (4,4); the margin is transparent.
	const AlphaCoverage mask = maskFromPixels(5, 5, {
		{ 2, 2 }, { 3, 3 }, { 4, 4 }
	});
	// The whole diagonal (0,0)..(4,4) is opaque from (2,2) on, so all four corners
	// converge on the same pixel; that is what the legacy crawl did for TopLeft only.
	CHECK(mask.crawlOrigin(CursorCrawlMode::Diagonal, CursorAnchor::TopLeft, 0.5f).x
	      == Catch::Approx(2.5f));
	CHECK(mask.crawlOrigin(CursorCrawlMode::Diagonal, CursorAnchor::TopLeft, 0.5f).y
	      == Catch::Approx(2.5f));
	CHECK(mask.crawlOrigin(CursorCrawlMode::Diagonal, CursorAnchor::TopRight, 0.5f).x
	      == Catch::Approx(2.5f));
	CHECK(mask.crawlOrigin(CursorCrawlMode::Diagonal, CursorAnchor::BottomLeft, 0.5f).y
	      == Catch::Approx(2.5f));
	// BottomRight starts on (4,4), which is already opaque: its own center wins.
	CHECK(mask.crawlOrigin(CursorCrawlMode::Diagonal, CursorAnchor::BottomRight, 0.5f).x
	      == Catch::Approx(4.5f));
	CHECK(mask.crawlOrigin(CursorCrawlMode::Diagonal, CursorAnchor::BottomRight, 0.5f).y
	      == Catch::Approx(4.5f));
}

TEST_CASE("Edge anchors crawl along the middle row or column", "[gui][cursor]")
{
	using namespace Lotte::Gui;
	const AlphaCoverage mask = maskFromPixels(5, 5, { { 2, 2 } });

	const glm::fvec2 fromTop =
		mask.crawlOrigin(CursorCrawlMode::Vertical, CursorAnchor::Top, 0.5f);
	CHECK(fromTop.x == Catch::Approx(2.5f));
	CHECK(fromTop.y == Catch::Approx(2.5f));

	const glm::fvec2 fromBottom =
		mask.crawlOrigin(CursorCrawlMode::Vertical, CursorAnchor::Bottom, 0.5f);
	CHECK(fromBottom.x == Catch::Approx(2.5f));
	CHECK(fromBottom.y == Catch::Approx(2.5f));

	const glm::fvec2 fromLeft =
		mask.crawlOrigin(CursorCrawlMode::Horizontal, CursorAnchor::Left, 0.5f);
	CHECK(fromLeft.x == Catch::Approx(2.5f));
	CHECK(fromLeft.y == Catch::Approx(2.5f));

	const glm::fvec2 fromRight =
		mask.crawlOrigin(CursorCrawlMode::Horizontal, CursorAnchor::Right, 0.5f);
	CHECK(fromRight.x == Catch::Approx(2.5f));
	CHECK(fromRight.y == Catch::Approx(2.5f));
}

TEST_CASE("A center anchor crawls radially to the nearest opaque pixel", "[gui][cursor]")
{
	using namespace Lotte::Gui;
	// 7x7 with a transparent center and a cross at distance 2. The hourglass case:
	// the intended origin is the middle, but the middle itself is empty.
	const AlphaCoverage mask = maskFromPixels(7, 7, {
		{ 3, 1 }, { 5, 3 }, { 3, 5 }, { 1, 3 }
	});
	const glm::fvec2 origin =
		mask.crawlOrigin(CursorCrawlMode::Radial, CursorAnchor::Center, 0.5f);
	// Ring order is top row, bottom row, left column, right column, left to right,
	// so (3, 1) is the first hit and its center is the origin.
	CHECK(origin.x == Catch::Approx(3.5f));
	CHECK(origin.y == Catch::Approx(1.5f));

	// An opaque center keeps the exact anchor point (no half-pixel shift).
	std::vector<std::uint8_t> opaque(std::size_t(7) * 7, 255);
	const AlphaCoverage solid(7, 7, std::move(opaque));
	const glm::fvec2 center =
		solid.crawlOrigin(CursorCrawlMode::Radial, CursorAnchor::Center, 0.5f);
	CHECK(center.x == Catch::Approx(3.5f));
	CHECK(center.y == Catch::Approx(3.5f));
}

TEST_CASE("A fully transparent mask falls back to the anchor", "[gui][cursor]")
{
	using namespace Lotte::Gui;
	const AlphaCoverage empty(5, 5, std::vector<std::uint8_t>(25, 0));
	const glm::fvec2 origin =
		empty.crawlOrigin(CursorCrawlMode::Diagonal, CursorAnchor::TopLeft, 0.5f);
	CHECK(origin.x == Catch::Approx(0.0f));
	CHECK(origin.y == Catch::Approx(0.0f));
	// NoCrawl ignores the mask entirely.
	const glm::fvec2 anchorOnly =
		empty.origin(CursorAnchor::BottomRight, true, CursorCrawlMode::NoCrawl, 0.5f);
	CHECK(anchorOnly.x == Catch::Approx(5.0f));
	CHECK(anchorOnly.y == Catch::Approx(5.0f));
}

TEST_CASE("Cursor resolves its hotspot from metadata, coverage and overrides",
	  "[gui][cursor]")
{
	using namespace Lotte::Gui;
	CursorImageVisual visual;
	visual.texture = 42;
	visual.pixelSize = glm::uvec2(10, 20);

	// No coverage: the anchor fraction is scaled to the image size.
	Cursor cursor(visual, nullptr, CursorMetadata{});
	CHECK(cursor.isImage());
	CHECK_FALSE(cursor.isModel());
	CHECK(cursor.hotspotPixels().x == Catch::Approx(0.0f));
	CHECK(cursor.hotspotPixels().y == Catch::Approx(0.0f));

	CursorMetadata centered;
	centered.anchor = CursorAnchor::Center;
	cursor.setMetadata(centered);
	CHECK(cursor.hotspotPixels().x == Catch::Approx(5.0f));
	CHECK(cursor.hotspotPixels().y == Catch::Approx(10.0f));

	// An explicit hotspot beats anchor and crawl.
	CursorMetadata explicitHotspot = centered;
	explicitHotspot.hotspotPixels = glm::fvec2(3.0f, 4.0f);
	cursor.setMetadata(explicitHotspot);
	CHECK(cursor.hotspotPixels().x == Catch::Approx(3.0f));
	CHECK(cursor.hotspotPixels().y == Catch::Approx(4.0f));

	// A mask of the right size re-enables the crawl.
	const AlphaCoverage mask = maskFromPixels(10, 20, { { 2, 7 } });
	cursor.setCoverage(mask);
	CursorMetadata crawling;
	crawling.anchor = CursorAnchor::TopLeft;
	crawling.alphaCrawl = true;
	cursor.setMetadata(crawling);
	CHECK(cursor.hotspotPixels().x == Catch::Approx(2.5f));
	CHECK(cursor.hotspotPixels().y == Catch::Approx(7.5f));

	// A mask of the wrong size is ignored instead of being indexed out of bounds.
	CursorImageVisual larger = visual;
	larger.pixelSize = glm::uvec2(20, 20);
	cursor.setImageVisual(larger);
	CHECK(cursor.hotspotPixels().x == Catch::Approx(0.0f));
	CHECK(cursor.hotspotPixels().y == Catch::Approx(0.0f));
}

TEST_CASE("Model cursors place their model-space origin on the screen position",
	  "[gui][cursor]")
{
	using namespace Lotte::Gui;
	CursorModelVisual model;
	model.mesh = 7;
	model.pipeline = 9;
	model.originInModel = glm::fvec3(1.0f, 2.0f, 0.0f);

	Cursor cursor(model);
	CHECK(cursor.isModel());
	CHECK_FALSE(cursor.isImage());
	REQUIRE(cursor.model() != nullptr);
	CHECK(cursor.model()->mesh == 7);

	const CursorModelPlacement placement =
		cursor.modelPlacement(glm::fvec2(100.0f, 50.0f));
	CHECK(placement.screenOrigin.x == Catch::Approx(100.0f));
	// The authored origin has to land exactly on the requested screen position.
	const glm::fvec4 mapped =
		placement.transform * glm::fvec4(model.originInModel, 1.0f);
	CHECK(mapped.x == Catch::Approx(100.0f));
	CHECK(mapped.y == Catch::Approx(50.0f));
	CHECK(mapped.z == Catch::Approx(0.0f));
}


