#include "LteFontAtlas.hpp"
#include <Lotte/GFX/LteImageConvert.hpp>
#include <Euphemy/Io/EuphMemoryDevice.hpp>
#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
#include <Euphemy/Media/Image/EuphJPEG.hpp>
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <Euphemy/Media/Image/EuphTGA.hpp>
#include <Euphemy/Media/Image/EuphWEBP.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace Lotte {
namespace Text {

namespace {

/** @brief Decodes one stored glyph payload into tightly packed RGBA8. */
Gfx::Rgba8Image decodePayload(const PreprocessedFontFace& face, const std::vector<std::byte>& bytes)
{
	if (bytes.empty()) {
		throw std::runtime_error("the glyph has an empty SDF payload");
	}
	using namespace Euph::Media::Image;
	DecodeTarget target;
	std::string format(face.imageFormatName());
	std::transform(format.begin(), format.end(), format.begin(),
				   [](unsigned char character) {
					   return static_cast<char>(std::toupper(character));
				   });
	const std::span<const std::byte> span(bytes.data(), bytes.size());

	if (format == "PNG") {
		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ, std::vector<std::byte>(bytes));
		PNG::decode(device, target);
	} else if (format == "TGA") {
		Euph::Io::MemoryDevice device(Elv::Io::Mode::READ, std::vector<std::byte>(bytes));
		TGA::decode(device, target);
	} else if (format == "JPEG" || format == "JPG") {
		if (!JPEG::decode(span, target)) {
			throw std::runtime_error("JPEG payload decoding failed");
		}
	} else if (format == "WEBP") {
		if (!WEBP::decode(span, target)) {
			throw std::runtime_error("WebP payload decoding failed");
		}
	} else {
		throw std::runtime_error("unsupported glyph image format '" + std::string(format) + "'");
	}
	return Gfx::toRgba8(target);
}

} // namespace

FontAtlas::FontAtlas(Gfx::ResourceFactory& factory, std::shared_ptr<const PreprocessedFontFace> face,
					 FontAtlasSettings settings)
	: factory_(&factory), face_(std::move(face)), settings_(settings)
{
	if (!face_) {
		throw std::invalid_argument("FontAtlas: the font face is null");
	}
	if (settings_.pageWidth == 0 || settings_.pageHeight == 0) {
		throw std::invalid_argument("FontAtlas: zero page extents");
	}
	tileSize_ = face_->storedBitmapSize();
	if (tileSize_ == 0) {
		throw std::invalid_argument("FontAtlas: the face has no stored tile size");
	}
	cellSize_ = tileSize_ + 2u * std::uint32_t(settings_.gutter);
	if (cellSize_ > settings_.pageWidth || cellSize_ > settings_.pageHeight) {
		throw std::invalid_argument("FontAtlas: a tile does not fit on a page");
	}
}

FontAtlas::~FontAtlas() = default;

void FontAtlas::createPage()
{
	pages_.push_back(Page{ Gfx::WriteableTexture2D::create(*factory_, settings_.pageWidth,
														   settings_.pageHeight) });
	// The create call already queued a full transparent upload.
	pendingUploads_ = true;
}

bool FontAtlas::allocateTile(FontAtlasGlyph& glyph)
{
	for (std::uint32_t page = 0; page < pages_.size(); ++page) {
		Page& candidate = pages_[page];
		if (candidate.nextX + cellSize_ > settings_.pageWidth) {
			candidate.nextX = 0;
			candidate.nextY += cellSize_;
		}
		if (candidate.nextY + cellSize_ > settings_.pageHeight) {
			continue;
		}
		glyph.page = page;
		glyph.x = static_cast<std::uint16_t>(candidate.nextX + settings_.gutter);
		glyph.y = static_cast<std::uint16_t>(candidate.nextY + settings_.gutter);
		candidate.nextX += cellSize_;
		return true;
	}

	if (settings_.maxPages != 0 && pages_.size() >= settings_.maxPages) {
		throw std::runtime_error("FontAtlas: the page budget is exhausted");
	}
	createPage();
	glyph.page = static_cast<std::uint32_t>(pages_.size() - 1);
	glyph.x = settings_.gutter;
	glyph.y = settings_.gutter;
	pages_.back().nextX = cellSize_;
	return true;
}

void FontAtlas::stampGlyph(char32_t codePoint, const StoredCharacter& glyph,
						   const FontAtlasGlyph& target)
{
	Gfx::Rgba8Image image;
	try {
		image = decodePayload(*face_, glyph.sdf);
	} catch (const std::exception& error) {
		throw std::runtime_error("FontAtlas: failed to decode U+" +
								 std::to_string(static_cast<std::uint32_t>(codePoint)) + ": " +
								 error.what());
	}
	if (image.width != tileSize_ || image.height != tileSize_) {
		throw std::runtime_error("FontAtlas: decoded tile for U+" +
								 std::to_string(static_cast<std::uint32_t>(codePoint)) +
								 " is not " + std::to_string(tileSize_) + "x" +
								 std::to_string(tileSize_));
	}
	if (target.page >= pages_.size()) {
		throw std::runtime_error("FontAtlas: invalid target page");
	}
	if (!pages_[target.page].texture.blitRgba8(image.pixels.data(), image.width, image.height,
											   target.x, target.y)) {
		throw std::runtime_error("FontAtlas: failed to stamp a glyph tile");
	}
	pendingUploads_ = true;
}

bool FontAtlas::ensureGlyph(char32_t codePoint)
{
	if (glyphs_.find(codePoint) != glyphs_.end()) {
		return true;
	}
	if (unavailable_.find(codePoint) != unavailable_.end()) {
		return false;
	}
	const auto found = face_->glyphs.find(static_cast<std::uint32_t>(codePoint));
	if (found == face_->glyphs.end() || !found->second.valid || found->second.sdf.empty()) {
		unavailable_.insert(codePoint);
		return false;
	}

	FontAtlasGlyph target;
	allocateTile(target);
	stampGlyph(codePoint, found->second, target);
	glyphs_.emplace(codePoint, target);
	return true;
}

std::size_t FontAtlas::ensureGlyphs(const TextLayout& layout)
{
	std::size_t added = 0;
	for (const PlacedGlyph& glyph : layout.glyphs) {
		if (glyph.faceIndex >= layout.faces.size() || layout.faces[glyph.faceIndex] != face_.get()) {
			continue;
		}
		if (glyphs_.find(glyph.codePoint) != glyphs_.end() ||
			unavailable_.find(glyph.codePoint) != unavailable_.end()) {
			continue;
		}
		if (ensureGlyph(glyph.codePoint)) {
			++added;
		}
	}
	return added;
}

bool FontAtlas::recordUpdates()
{
	bool recorded = pendingUploads_;
	const std::uint64_t pageArea = std::uint64_t(settings_.pageWidth) * settings_.pageHeight;
	for (Page& page : pages_) {
		if (!page.texture.hasDirtyRegion()) {
			continue;
		}
		const Gfx::WriteableTexture2D::DirtyRegion region = page.texture.dirtyRegion();
		const std::uint64_t dirtyArea = std::uint64_t(region.width) * region.height;
		// Bulk loads touch most of a page; a full-page upload is one command, while a
		// narrow rectangle would need one command per row.
		if (dirtyArea * 4u >= pageArea || std::uint32_t(region.width) * 2u >= settings_.pageWidth) {
			page.texture.markAllDirty();
		}
		page.texture.recordUpdate();
		recorded = true;
	}
	pendingUploads_ = false;
	return recorded;
}

const FontAtlasGlyph* FontAtlas::findGlyph(char32_t codePoint) const
{
	const auto found = glyphs_.find(codePoint);
	return found == glyphs_.end() ? nullptr : &found->second;
}

bool FontAtlas::mapGlyph(char32_t codePoint, glm::fvec2& uv0, glm::fvec2& uv1) const
{
	const FontAtlasGlyph* glyph = findGlyph(codePoint);
	if (glyph == nullptr || glyph->page >= pages_.size()) {
		return false;
	}
	const float width = static_cast<float>(settings_.pageWidth);
	const float height = static_cast<float>(settings_.pageHeight);
	const float left = static_cast<float>(glyph->x) / width;
	const float top = static_cast<float>(glyph->y) / height;
	const float right = static_cast<float>(glyph->x + tileSize_) / width;
	const float bottom = static_cast<float>(glyph->y + tileSize_) / height;
	uv0 = glm::fvec2(left, top);
	uv1 = glm::fvec2(right, bottom);
	return true;
}

std::uint32_t FontAtlas::pageForGlyph(char32_t codePoint) const
{
	const FontAtlasGlyph* glyph = findGlyph(codePoint);
	return glyph == nullptr ? InvalidAtlasPage : glyph->page;
}

Kld::HandleId FontAtlas::pageTexture(std::uint32_t page) const
{
	if (page >= pages_.size()) {
		return Kld::HandleId(0);
	}
	return pages_[page].texture.handle();
}

const Gfx::WriteableTexture2D* FontAtlas::pageSurface(std::uint32_t page) const
{
	return page < pages_.size() ? &pages_[page].texture : nullptr;
}

} // namespace Text
} // namespace Lotte
