#include "LteCursor.hpp"
#include <Lotte/GFX/LteImageConvert.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace Lotte {
namespace Gui {

const char* cursorAnchorName(CursorAnchor anchor) noexcept
{
	switch (anchor) {
		case CursorAnchor::TopLeft: return "TopLeft";
		case CursorAnchor::Top: return "Top";
		case CursorAnchor::TopRight: return "TopRight";
		case CursorAnchor::Left: return "Left";
		case CursorAnchor::Center: return "Center";
		case CursorAnchor::Right: return "Right";
		case CursorAnchor::BottomLeft: return "BottomLeft";
		case CursorAnchor::Bottom: return "Bottom";
		case CursorAnchor::BottomRight: return "BottomRight";
	}
	return "TopLeft";
}

const char* cursorCrawlModeName(CursorCrawlMode mode) noexcept
{
	switch (mode) {
		case CursorCrawlMode::NoCrawl: return "NoCrawl";
		case CursorCrawlMode::Diagonal: return "Diagonal";
		case CursorCrawlMode::Vertical: return "Vertical";
		case CursorCrawlMode::Horizontal: return "Horizontal";
		case CursorCrawlMode::Radial: return "Radial";
	}
	return "NoCrawl";
}

glm::fvec2 cursorAnchorFraction(CursorAnchor anchor) noexcept
{
	switch (anchor) {
		case CursorAnchor::TopLeft: return glm::fvec2(0.0f, 0.0f);
		case CursorAnchor::Top: return glm::fvec2(0.5f, 0.0f);
		case CursorAnchor::TopRight: return glm::fvec2(1.0f, 0.0f);
		case CursorAnchor::Left: return glm::fvec2(0.0f, 0.5f);
		case CursorAnchor::Center: return glm::fvec2(0.5f, 0.5f);
		case CursorAnchor::Right: return glm::fvec2(1.0f, 0.5f);
		case CursorAnchor::BottomLeft: return glm::fvec2(0.0f, 1.0f);
		case CursorAnchor::Bottom: return glm::fvec2(0.5f, 1.0f);
		case CursorAnchor::BottomRight: return glm::fvec2(1.0f, 1.0f);
	}
	return glm::fvec2(0.0f);
}

CursorCrawlMode defaultCrawlFor(CursorAnchor anchor) noexcept
{
	switch (anchor) {
		case CursorAnchor::TopLeft:
		case CursorAnchor::TopRight:
		case CursorAnchor::BottomLeft:
		case CursorAnchor::BottomRight:
			return CursorCrawlMode::Diagonal;
		case CursorAnchor::Top:
		case CursorAnchor::Bottom:
			return CursorCrawlMode::Vertical;
		case CursorAnchor::Left:
		case CursorAnchor::Right:
			return CursorCrawlMode::Horizontal;
		case CursorAnchor::Center:
			return CursorCrawlMode::Radial;
	}
	return CursorCrawlMode::NoCrawl;
}

AlphaCoverage::AlphaCoverage(std::uint16_t width, std::uint16_t height,
			     std::vector<std::uint8_t> alpha)
	: width_(width), height_(height), alpha_(std::move(alpha))
{
	const std::size_t expected = std::size_t(width_) * std::size_t(height_);
	if (width_ == 0 || height_ == 0 || alpha_.size() < expected) {
		width_ = 0;
		height_ = 0;
		alpha_.clear();
		return;
	}
	alpha_.resize(expected);
}

AlphaCoverage AlphaCoverage::fromRgba8(const std::uint8_t* pixels, std::uint16_t width,
				       std::uint16_t height)
{
	if (pixels == nullptr || width == 0 || height == 0) {
		return {};
	}
	std::vector<std::uint8_t> alpha(std::size_t(width) * std::size_t(height));
	for (std::size_t index = 0; index < alpha.size(); ++index) {
		alpha[index] = pixels[index * 4u + 3u];
	}
	return AlphaCoverage(width, height, std::move(alpha));
}

AlphaCoverage AlphaCoverage::fromDecodeTarget(
	const Euph::Media::Image::DecodeTarget& source)
{
	const Gfx::Rgba8Image image = Gfx::toRgba8(source);
	return fromRgba8(image.pixels.data(), image.width, image.height);
}

bool AlphaCoverage::isOpaque(std::uint16_t x, std::uint16_t y, float threshold) const noexcept
{
	if (empty() || x >= width_ || y >= height_) {
		return false;
	}
	const float value = float(alpha_[std::size_t(y) * width_ + x]) / 255.0f;
	return value >= threshold;
}

glm::fvec2 AlphaCoverage::anchorPixel(CursorAnchor anchor) const noexcept
{
	if (empty()) {
		return glm::fvec2(0.0f);
	}
	const glm::fvec2 fraction = cursorAnchorFraction(anchor);
	return glm::fvec2(fraction.x * float(width_), fraction.y * float(height_));
}

glm::fvec2 AlphaCoverage::nearestOpaque(glm::fvec2 anchor, float threshold) const noexcept
{
	if (empty()) {
		return glm::fvec2(0.0f);
	}
	const int width = int(width_);
	const int height = int(height_);
	const int cx = std::clamp(int(std::floor(anchor.x)), 0, width - 1);
	const int cy = std::clamp(int(std::floor(anchor.y)), 0, height - 1);
	const auto opaqueAt = [this, threshold, width, height](int x, int y) {
		return x >= 0 && y >= 0 && x < width && y < height &&
			isOpaque(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), threshold);
	};
	if (opaqueAt(cx, cy)) {
		// The anchor pixel is already opaque: keep the exact anchor point so a
		// Center cursor pivots around the authored center, not a pixel corner.
		return anchor;
	}
	const int maxDistance = std::max(width, height);
	for (int distance = 1; distance <= maxDistance; ++distance) {
		for (int x = cx - distance; x <= cx + distance; ++x) {
			if (opaqueAt(x, cy - distance)) {
				return glm::fvec2(float(x) + 0.5f, float(cy - distance) + 0.5f);
			}
			if (opaqueAt(x, cy + distance)) {
				return glm::fvec2(float(x) + 0.5f, float(cy + distance) + 0.5f);
			}
		}
		for (int y = cy - distance + 1; y <= cy + distance - 1; ++y) {
			if (opaqueAt(cx - distance, y)) {
				return glm::fvec2(float(cx - distance) + 0.5f, float(y) + 0.5f);
			}
			if (opaqueAt(cx + distance, y)) {
				return glm::fvec2(float(cx + distance) + 0.5f, float(y) + 0.5f);
			}
		}
	}
	return anchor;
}

glm::fvec2 AlphaCoverage::crawlOrigin(CursorCrawlMode mode, CursorAnchor anchor,
				      float threshold) const noexcept
{
	const glm::fvec2 anchorPoint = anchorPixel(anchor);
	if (empty() || mode == CursorCrawlMode::NoCrawl) {
		return anchorPoint;
	}

	const glm::fvec2 fraction = cursorAnchorFraction(anchor);
	const bool atRight = fraction.x >= 1.0f;
	const bool atBottom = fraction.y >= 1.0f;
	const bool atHorizontalEdge = fraction.x <= 0.0f || fraction.x >= 1.0f;
	const bool atVerticalEdge = fraction.y <= 0.0f || fraction.y >= 1.0f;
	const bool atCorner = atHorizontalEdge && atVerticalEdge;

	int startX = 0;
	int startY = 0;
	int stepX = 0;
	int stepY = 0;
	int limit = 0;
	switch (mode) {
		case CursorCrawlMode::Diagonal:
			if (!atCorner) {
				return nearestOpaque(anchorPoint, threshold);
			}
			startX = atRight ? int(width_) - 1 : 0;
			startY = atBottom ? int(height_) - 1 : 0;
			stepX = atRight ? -1 : 1;
			stepY = atBottom ? -1 : 1;
			limit = std::min(int(width_), int(height_));
			break;
		case CursorCrawlMode::Vertical:
			if (!atVerticalEdge) {
				return nearestOpaque(anchorPoint, threshold);
			}
			startX = int(width_) / 2;
			startY = atBottom ? int(height_) - 1 : 0;
			stepY = atBottom ? -1 : 1;
			limit = int(height_);
			break;
		case CursorCrawlMode::Horizontal:
			if (!atHorizontalEdge) {
				return nearestOpaque(anchorPoint, threshold);
			}
			startX = atRight ? int(width_) - 1 : 0;
			startY = int(height_) / 2;
			stepX = atRight ? -1 : 1;
			limit = int(width_);
			break;
		case CursorCrawlMode::Radial:
			return nearestOpaque(anchorPoint, threshold);
		case CursorCrawlMode::NoCrawl:
			return anchorPoint;
	}

	for (int step = 0; step < limit; ++step) {
		const int x = startX + step * stepX;
		const int y = startY + step * stepY;
		if (x < 0 || y < 0 || x >= int(width_) || y >= int(height_)) {
			continue;
		}
		if (isOpaque(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), threshold)) {
			return glm::fvec2(float(x) + 0.5f, float(y) + 0.5f);
		}
	}
	// The crawl path never hit an opaque pixel (a fully transparent mask, or an
	// anchor whose direction points out of the image): fall back to the nearest
	// opaque pixel, which degrades to the anchor itself when nothing is opaque.
	return nearestOpaque(anchorPoint, threshold);
}

glm::fvec2 AlphaCoverage::origin(CursorAnchor anchor, bool alphaCrawl,
				 CursorCrawlMode crawlMode, float threshold) const noexcept
{
	if (!alphaCrawl) {
		return anchorPixel(anchor);
	}
	return crawlOrigin(crawlMode, anchor, threshold);
}

Cursor::Cursor(CursorImageVisual visual, const AlphaCoverage* coverage,
	       CursorMetadata metadata)
	: visual_(visual), metadata_(metadata)
{
	if (coverage != nullptr) {
		coverage_ = *coverage;
	}
	refreshHotspot();
}

Cursor::Cursor(CursorModelVisual visual, CursorMetadata metadata)
	: visual_(visual), metadata_(metadata)
{
	refreshHotspot();
}

void Cursor::setMetadata(CursorMetadata metadata)
{
	metadata_ = std::move(metadata);
	refreshHotspot();
}

void Cursor::setCoverage(const AlphaCoverage& coverage)
{
	coverage_ = coverage;
	refreshHotspot();
}

void Cursor::setImageVisual(CursorImageVisual visual)
{
	visual_ = visual;
	refreshHotspot();
}

void Cursor::setModelVisual(CursorModelVisual visual)
{
	visual_ = visual;
	refreshHotspot();
}

void Cursor::refreshHotspot()
{
	if (metadata_.hotspotPixels) {
		hotspot_ = *metadata_.hotspotPixels;
		return;
	}
	const CursorImageVisual* visual = image();
	if (visual == nullptr) {
		// A model cursor's origin is authored in model space; the screen placement
		// comes from CursorModelVisual::originInModel, not from an image hotspot.
		hotspot_ = glm::fvec2(0.0f);
		return;
	}
	const CursorCrawlMode mode = metadata_.crawl.value_or(
		defaultCrawlFor(metadata_.anchor));
	const bool coverageMatches = !coverage_.empty() &&
		coverage_.width() == visual->pixelSize.x &&
		coverage_.height() == visual->pixelSize.y;
	if (coverageMatches) {
		hotspot_ = coverage_.origin(metadata_.anchor, metadata_.alphaCrawl, mode,
					    metadata_.alphaThreshold);
		return;
	}
	hotspot_ = cursorAnchorFraction(metadata_.anchor) * glm::fvec2(visual->pixelSize);
}

void Cursor::render(GuiRenderer& renderer, glm::fvec2 position) const
{
	render(renderer, position, glm::fvec4(1.0f));
}

void Cursor::render(GuiRenderer& renderer, glm::fvec2 position, glm::fvec4 tint) const
{
	if (!metadata_.visible) {
		return;
	}
	const CursorImageVisual* visual = image();
	if (visual == nullptr || visual->texture == 0 ||
	    visual->pixelSize.x == 0 || visual->pixelSize.y == 0) {
		return;
	}
	const glm::fvec2 size = glm::fvec2(visual->pixelSize) * metadata_.scale;
	if (size.x <= 0.0f || size.y <= 0.0f) {
		return;
	}
	const glm::fvec2 topLeft = position - hotspot_ * metadata_.scale;
	ColouredTexturedQuad quad;
	quad.topLeft = topLeft;
	quad.bottomRight = topLeft + size;
	quad.uv[0] = visual->uv[0];
	quad.uv[1] = visual->uv[1];
	quad.binding.texture = visual->texture;
	quad.colour = tint;
	renderer.render(quad);
}

CursorModelPlacement Cursor::modelPlacement(glm::fvec2 position, float depth) const
{
	CursorModelPlacement placement;
	placement.screenOrigin = position;
	const CursorModelVisual* visual = model();
	if (visual == nullptr) {
		return placement;
	}
	glm::mat4 transform(1.0f);
	transform = glm::translate(transform, glm::fvec3(position, depth));
	if (visual->rotationRadians.x != 0.0f) {
		transform = glm::rotate(transform, visual->rotationRadians.x, glm::fvec3(1.0f, 0.0f, 0.0f));
	}
	if (visual->rotationRadians.y != 0.0f) {
		transform = glm::rotate(transform, visual->rotationRadians.y, glm::fvec3(0.0f, 1.0f, 0.0f));
	}
	if (visual->rotationRadians.z != 0.0f) {
		transform = glm::rotate(transform, visual->rotationRadians.z, glm::fvec3(0.0f, 0.0f, 1.0f));
	}
	transform = glm::scale(transform, visual->scale);
	transform = glm::translate(transform, -visual->originInModel);
	placement.transform = transform;
	return placement;
}

} // namespace Gui
} // namespace Lotte

