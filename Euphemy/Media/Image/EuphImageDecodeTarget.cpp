#include <Euphemy/Media/Image/EuphImageDecodeTarget.hpp>
namespace Euph {
namespace Media {
namespace Image {
Palette::Palette(std::pmr::memory_resource* memResource)
	: data(memResource), format(Format::INVALID), transparentColorIndex(-1) {}

Frame::Frame(std::pmr::memory_resource* memResource)
	: data(memResource) {}

void Frame::allocate(Format format, unsigned width, unsigned height) {
	this->width = width;
	this->height = height;
	this->stride = pixelByteSize(format) * width;
	data.resize(stride * height);
}

bool DecodeTarget::getIsAnimated() const
{
	return isAnimated;
}

void DecodeTarget::setIsAnimated(bool newIsAnimated)
{
	isAnimated = newIsAnimated;
}

Format DecodeTarget::getFormat() const
{
	return format;
}

void DecodeTarget::setFormat(Format newFormat)
{
	format = newFormat;
}

std::optional<float> DecodeTarget::getDelayTime() const
{
	return delayTime;
}

void DecodeTarget::setDelayTime(std::optional<float> newDelayTime)
{
	delayTime = newDelayTime;
}

const std::pmr::vector<Frame>& DecodeTarget::getFrames() const
{
	return frames;
}

std::pmr::vector<Frame>& DecodeTarget::getFrames()
{
	return frames;
}

std::pmr::memory_resource* DecodeTarget::getMemResource() const
{
	return memResource;
}

DecodeTarget::DecodeTarget(std::pmr::memory_resource* memResource)
	: frames(memResource), memResource(memResource) {}

Frame& DecodeTarget::addFrame(unsigned width, unsigned height) {
	frames.emplace_back(memResource);
	frames.back().allocate(format, width, height);
	return frames.back();
}

Palette& DecodeTarget::createPalette(Format format, unsigned indices, unsigned transparentIndex) {
	palette.emplace(memResource);
	palette->format = format;
	palette->transparentColorIndex = transparentIndex;
	palette->data.resize(pixelByteSize(format) * indices);
	return *palette;
}

Frame& DecodeTarget::getFrame(size_t index) { return frames.at(index); }

const Frame& DecodeTarget::getFrame(size_t index) const { return frames.at(index); }

std::optional<Palette>& DecodeTarget::getPalette() { return palette; }

const std::optional<Palette>& DecodeTarget::getPalette() const { return palette; }

}
}
}
