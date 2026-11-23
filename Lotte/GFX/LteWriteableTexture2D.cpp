#include "LteWriteableTexture2D.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace Lotte {
namespace Gfx {

namespace {

/** @brief The one pixel format a WriteableTexture2D stores and uploads. */
constexpr Kld::TextureFormat surfaceFormat = Kld::TextureFormat::RGBA8U;

} // namespace

WriteableTexture2D::WriteableTexture2D() = default;

WriteableTexture2D::~WriteableTexture2D() = default;

WriteableTexture2D::WriteableTexture2D(WriteableTexture2D&& other) noexcept
	: texture_(std::move(other.texture_)), pixels_(std::move(other.pixels_)),
	  width_(other.width_), height_(other.height_),
	  dirtyX0_(other.dirtyX0_), dirtyY0_(other.dirtyY0_),
	  dirtyX1_(other.dirtyX1_), dirtyY1_(other.dirtyY1_)
{
	other.width_ = 0;
	other.height_ = 0;
	other.clearDirty();
}

WriteableTexture2D& WriteableTexture2D::operator=(WriteableTexture2D&& other) noexcept
{
	if (this != &other) {
		texture_ = std::move(other.texture_);
		pixels_ = std::move(other.pixels_);
		width_ = other.width_;
		height_ = other.height_;
		dirtyX0_ = other.dirtyX0_;
		dirtyY0_ = other.dirtyY0_;
		dirtyX1_ = other.dirtyX1_;
		dirtyY1_ = other.dirtyY1_;
		other.width_ = 0;
		other.height_ = 0;
		other.clearDirty();
	}
	return *this;
}

WriteableTexture2D::WriteableTexture2D(Texture texture,
				       std::shared_ptr<std::vector<std::uint8_t> > pixels,
				       std::uint16_t width, std::uint16_t height)
	: texture_(std::move(texture)), pixels_(std::move(pixels)),
	  width_(width), height_(height)
{
}

WriteableTexture2D WriteableTexture2D::create(ResourceFactory& factory, std::uint16_t width,
					      std::uint16_t height)
{
	if (width == 0 || height == 0) {
		throw std::invalid_argument("WriteableTexture2D: zero extents");
	}
	auto pixels = std::make_shared<std::vector<std::uint8_t> >(
		std::size_t(width) * std::size_t(height) * 4u, 0u);
	Texture texture = factory.createTexture2D(surfaceFormat, width, height, nullptr, {});
	// The create command carries no pixels, so the initial update below is what
	// actually defines the texture; keep the CPU bytes alive until submission.
	factory.retain(pixels);
	WriteableTexture2D result(std::move(texture), std::move(pixels), width, height);
	result.markAllDirty();
	result.recordUpdate();
	return result;
}

WriteableTexture2D WriteableTexture2D::fromDecodeTarget(
	ResourceFactory& factory, const Euph::Media::Image::DecodeTarget& source)
{
	return fromRgba8(factory, toRgba8(source));
}

WriteableTexture2D WriteableTexture2D::fromRgba8(ResourceFactory& factory, Rgba8Image image)
{
	if (!image.isValid()) {
		throw std::invalid_argument("WriteableTexture2D: the RGBA8 image is invalid");
	}
	const std::uint16_t width = image.width;
	const std::uint16_t height = image.height;
	Texture texture = factory.createTexture2D(surfaceFormat, width, height, nullptr, {});
	auto pixels = std::make_shared<std::vector<std::uint8_t> >(std::move(image.pixels));
	factory.retain(pixels);
	WriteableTexture2D result(std::move(texture), std::move(pixels), width, height);
	result.markAllDirty();
	result.recordUpdate();
	return result;
}

bool WriteableTexture2D::isValid() const noexcept
{
	return texture_.id() != 0 && pixels_ && !pixels_->empty() && width_ != 0 && height_ != 0;
}

std::uint8_t* WriteableTexture2D::pixels() noexcept
{
	return pixels_ && !pixels_->empty() ? pixels_->data() : nullptr;
}

const std::uint8_t* WriteableTexture2D::pixels() const noexcept
{
	return pixels_ && !pixels_->empty() ? pixels_->data() : nullptr;
}

void WriteableTexture2D::resize(std::uint16_t width, std::uint16_t height)
{
	if (width == 0 || height == 0) {
		throw std::invalid_argument("WriteableTexture2D::resize: zero extents");
	}
	pixels_->assign(std::size_t(width) * std::size_t(height) * 4u, 0u);
	width_ = width;
	height_ = height;
	texture_.resize2D(surfaceFormat, 1, false, width, height);
	markAllDirty();
}

void WriteableTexture2D::fill(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
			      std::uint8_t alpha)
{
	std::uint8_t* target = pixels();
	if (target == nullptr) {
		return;
	}
	for (std::size_t index = 0; index < byteSize(); index += 4) {
		target[index + 0] = red;
		target[index + 1] = green;
		target[index + 2] = blue;
		target[index + 3] = alpha;
	}
	markAllDirty();
}

void WriteableTexture2D::setPixel(std::uint16_t x, std::uint16_t y, std::uint8_t red,
				  std::uint8_t green, std::uint8_t blue, std::uint8_t alpha)
{
	std::uint8_t* target = pixels();
	if (target == nullptr || x >= width_ || y >= height_) {
		return;
	}
	std::uint8_t* pixel = target + std::size_t(y) * rowBytes() + std::size_t(x) * 4u;
	pixel[0] = red;
	pixel[1] = green;
	pixel[2] = blue;
	pixel[3] = alpha;
	markDirty(x, y, 1, 1);
}

bool WriteableTexture2D::blitRgba8(const std::uint8_t* source, std::uint16_t width,
				   std::uint16_t height, std::uint16_t x, std::uint16_t y)
{
	std::uint8_t* target = pixels();
	if (target == nullptr || source == nullptr || width == 0 || height == 0) {
		return false;
	}
	if (x >= width_ || y >= height_) {
		return false;
	}
	const std::uint16_t copyWidth =
		std::min<std::uint16_t>(width, static_cast<std::uint16_t>(width_ - x));
	const std::uint16_t copyHeight =
		std::min<std::uint16_t>(height, static_cast<std::uint16_t>(height_ - y));
	if (copyWidth == 0 || copyHeight == 0) {
		return false;
	}
	for (std::uint16_t row = 0; row < copyHeight; ++row) {
		std::memcpy(target + std::size_t(y + row) * rowBytes() + std::size_t(x) * 4u,
			    source + std::size_t(row) * std::size_t(width) * 4u,
			    std::size_t(copyWidth) * 4u);
	}
	markDirty(x, y, copyWidth, copyHeight);
	return true;
}

void WriteableTexture2D::markDirty(std::uint16_t x, std::uint16_t y, std::uint16_t width,
				   std::uint16_t height)
{
	if (pixels_ == nullptr || width == 0 || height == 0) {
		return;
	}
	const std::int32_t x0 = std::min<std::int32_t>(x, width_);
	const std::int32_t y0 = std::min<std::int32_t>(y, height_);
	const std::int32_t x1 = std::min<std::int32_t>(std::int32_t(x) + width, width_);
	const std::int32_t y1 = std::min<std::int32_t>(std::int32_t(y) + height, height_);
	if (x1 <= x0 || y1 <= y0) {
		return;
	}
	if (dirtyY1_ <= dirtyY0_ || dirtyX1_ <= dirtyX0_) {
		dirtyX0_ = x0;
		dirtyY0_ = y0;
		dirtyX1_ = x1;
		dirtyY1_ = y1;
		return;
	}
	dirtyX0_ = std::min(dirtyX0_, x0);
	dirtyY0_ = std::min(dirtyY0_, y0);
	dirtyX1_ = std::max(dirtyX1_, x1);
	dirtyY1_ = std::max(dirtyY1_, y1);
}

void WriteableTexture2D::markAllDirty()
{
	if (width_ == 0 || height_ == 0) {
		clearDirty();
		return;
	}
	dirtyX0_ = 0;
	dirtyY0_ = 0;
	dirtyX1_ = width_;
	dirtyY1_ = height_;
}

WriteableTexture2D::DirtyRegion WriteableTexture2D::dirtyRegion() const noexcept
{
	if (dirtyX1_ <= dirtyX0_ || dirtyY1_ <= dirtyY0_) {
		return {};
	}
	return DirtyRegion{ static_cast<std::uint16_t>(dirtyX0_),
			    static_cast<std::uint16_t>(dirtyY0_),
			    static_cast<std::uint16_t>(dirtyX1_ - dirtyX0_),
			    static_cast<std::uint16_t>(dirtyY1_ - dirtyY0_) };
}

void WriteableTexture2D::clearDirty() noexcept
{
	dirtyX0_ = 0;
	dirtyY0_ = 0;
	dirtyX1_ = 0;
	dirtyY1_ = 0;
}

void WriteableTexture2D::recordUpdate()
{
	const DirtyRegion region = dirtyRegion();
	const std::uint8_t* base = pixels();
	if (region.empty() || base == nullptr || texture_.id() == 0) {
		return;
	}
	const std::uint16_t x0 = region.x;
	const std::uint16_t y0 = region.y;
	if (x0 == 0 && region.width == width_) {
		// Full-width rectangles are contiguous in memory: one command.
		texture_.update2D(surfaceFormat, 0, 0, y0, width_, region.height,
				  base + std::size_t(y0) * rowBytes());
	} else {
		const std::uint16_t endY = static_cast<std::uint16_t>(y0 + region.height);
		for (std::uint16_t y = y0; y < endY; ++y) {
			texture_.update2D(surfaceFormat, 0, x0, y, region.width, 1,
					  base + std::size_t(y) * rowBytes() +
						  std::size_t(x0) * 4u);
		}
	}
	clearDirty();
}

} // namespace Gfx
} // namespace Lotte
