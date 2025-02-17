#include "EuphImage.hpp"
#include "Elvavena/Util/ElvSpanHelpers.hpp"
#include <Elvavena/Util/ElvDither.hpp>
#include <Elvavena/Util/ElvEdgeFunction.hpp>
#include <cstring>
namespace Euph {
namespace Media {
namespace Image {

void ReadOnlyPalettedImage2D::setIndices(const std::span<const uint8_t>& newIndices)
{
	indices = newIndices;
}

unsigned IReadOnlyImage2D::getWidth() const
{
	return width;
}

unsigned IReadOnlyImage2D::getHeight() const
{
	return height;
}

unsigned IReadOnlyImage2D::getStride() const
{
	return stride;
}

float IReadOnlyImage2D::getWidthF() const
{
	return widthF;
}

float IReadOnlyImage2D::getHeightF() const
{
	return heightF;
}

float IReadOnlyImage2D::getWidthR() const
{
	return widthR;
}

float IReadOnlyImage2D::getHeightR() const
{
	return heightR;
}

void IReadOnlyImage2D::recalculateDimensions()
{
	/*
	unsigned width, height, stride;
	float widthF, heightF;
	float widthR, heightR;
*/
	widthF = static_cast<float>(width-1);
	heightF = static_cast<float>(height-1);
	widthR = 1.0f / static_cast<float>(width-1);
	heightR = 1.0f / static_cast<float>(height-1);
}

/*
unsigned width;
unsigned height;
unsigned stride;
float widthF;
float heightF;
float widthR;
float heightR;
*/
IReadOnlyImage2D::IReadOnlyImage2D(const IReadOnlyImage2D& cpy)
	: width(cpy.width), height(cpy.height), stride(cpy.stride),
	widthF(cpy.widthF), heightF(cpy.heightF), widthR(cpy.widthR), heightR(cpy.heightR)
{

}

IReadOnlyImage2D& IReadOnlyImage2D::operator=(const IReadOnlyImage2D& cpy)
{
	this->width = cpy.width;
	this->height = cpy.height;
	this->stride = cpy.stride;
	this->widthF = cpy.widthF;
	this->heightF = cpy.heightF;
	this->widthR = cpy.widthR;
	this->heightR = cpy.heightR;
	return *this;
}

void IReadOnlyImage2D::sample(const glm::fvec2& pos, const glm::uvec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap) const
{
	switch (filteringType) {
	case TextureFiltering::NEAREST_NEIGHBOUR:
		getPixel(glm::uvec2( static_cast<unsigned>( std::round(pos.x*getWidthF())) % getWidth() , static_cast<unsigned>(std::round(pos.y * getHeightF())) % getHeight() ),
				 colourKernel, wrap);
		break;
	case TextureFiltering::DITHERED:
		{
		glm::fvec2 texelCoords = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF());
		texelCoords += Elv::Util::LOOKUP[screenpos[1]&1][screenpos[0]&1];
		getPixel(glm::uvec2( static_cast<unsigned>(std::round(texelCoords.x))%getWidth(),static_cast<unsigned>(std::round(texelCoords.y))%getHeight() ),colourKernel, wrap);
		break;
		}
	case TextureFiltering::BILINEAR: {
		const glm::fvec2 tmp = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF() );
		const float xW = tmp[0] - std::floor(tmp[0]);
		const float yW = tmp[1] - std::floor(tmp[1]);
		const glm::uvec2 coords[4] = {
			// Top left
			glm::uvec2( static_cast<int>(std::floor(tmp[0])), static_cast<unsigned>(std::floor(tmp[1])) ),
			// Top Right
			glm::uvec2( static_cast<int>(std::ceil(tmp[0])), static_cast<unsigned>(std::floor(tmp[1])) ),
			// Bottom left
			glm::uvec2( static_cast<int>(std::floor(tmp[0])), static_cast<unsigned>(std::ceil(tmp[1])) ),
			// Bottom Right
			glm::uvec2( static_cast<int>(std::ceil(tmp[0])), static_cast<unsigned>(std::ceil(tmp[1])) )
		};
		const float weights[4] = {
			// Top left
			((1.0f - xW) * (1.0f - yW)),
			// Top Right
			(xW * (1.0f - yW)),
			// Bottom left
			((1.0f - xW) * yW),
			// Bottom Right
			(xW * yW)
		};
		glm::fvec4 colours[4];
		for(int i = 0; i < 4; ++i)
		{
			getPixel(coords[i], colours[i], wrap );
			colours[i] *= weights[i];
		}
		colourKernel = colours[0];
		for(int i = 1; i < 4; ++i)
		{
			colourKernel += colours[i];
		}
	}
	case TextureFiltering::THREE_POINT: {
		const glm::fvec2 tmp = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF() );
		const glm::fvec2 coordEdgeTopLeft( std::floor(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeTopRight( std::ceil(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeBottomLeft( std::floor(tmp[0]), std::ceil(tmp[1]) );
		const glm::fvec2 coordEdgeBottomRight( std::ceil(tmp[0]), std::ceil(tmp[1]) );
		const bool bottomRightIsUsed = ((tmp.y - coordEdgeTopLeft.y) * (tmp.x - coordEdgeTopLeft.x)) >= 0.25f;
		const glm::fvec2& thirdPosition = bottomRightIsUsed ? coordEdgeBottomRight : coordEdgeTopLeft;
		glm::fvec4 colourEdgeTopRight, colourEdgeBottomLeft, thirdColour;
		getPixel(glm::uvec2( static_cast<unsigned>(coordEdgeTopRight[0]) ,static_cast<unsigned>(coordEdgeTopRight[1]) ),colourEdgeTopRight,wrap );
		getPixel(glm::uvec2( static_cast<unsigned>(coordEdgeBottomLeft[0]) ,static_cast<unsigned>(coordEdgeBottomLeft[1]) ),colourEdgeBottomLeft,wrap );
		getPixel(glm::uvec2( static_cast<unsigned>(thirdPosition[0]) ,static_cast<unsigned>(thirdPosition[1]) ),thirdColour,wrap );
		/*
			const float w0 = edgeFunction(v1, v2, tmp);
			const float w1 = edgeFunction(v2, v0, tmp);
			const float w2 = edgeFunction(v0, v1, tmp);
			If we have flat-bottom:
			V0 - TopRight
			V1 - BottomLeft
			V2 - BottomRight = thirdPosition
			If we have flat-top:
			V0 = Top-Left = ThirdPosition
			V1 - Top-Right
			V2 - Bottom-Left
*/
		if(bottomRightIsUsed) {
			const float w0 = Elv::Util::edgeFunction<float>(coordEdgeBottomLeft, thirdPosition, tmp);
			const float w1 = Elv::Util::edgeFunction<float>(thirdPosition, coordEdgeTopRight, tmp);
			const float w2 = Elv::Util::edgeFunction<float>(coordEdgeTopRight, coordEdgeBottomLeft, tmp);
			colourKernel = (colourEdgeTopRight * w0) + (colourEdgeBottomLeft * w1) + (thirdColour * w2);
		} else {
			const float w0 = Elv::Util::edgeFunction<float>(coordEdgeBottomLeft, coordEdgeTopRight, tmp);
			const float w1 = Elv::Util::edgeFunction<float>(coordEdgeTopRight, thirdPosition, tmp);
			const float w2 = Elv::Util::edgeFunction<float>(thirdPosition, coordEdgeBottomLeft, tmp);
			colourKernel = (thirdColour * w0) + (colourEdgeBottomLeft * w1) + (colourEdgeTopRight * w2);
		}
		break;
		}
	}
}

IMutableImage2D::IMutableImage2D()
{

}

IMutableImage2D::IMutableImage2D(unsigned int width, unsigned int height)
	: IReadOnlyImage2D(width, height)
{

}

bool IMutableImage2D::setPixelWithBlending(const glm::uvec2& pos, const glm::fvec4& colourKernel, AlphaBlending blendingType)
{
	float a = colourKernel.w;
	switch (blendingType) {
	case AlphaBlending::ALPHA_TESTING:
	{
		if(a >= 0.99607843137255f) {
			setPixelDithered(pos,colourKernel);
			return true;
		} else return false;
		break;
	}
	case AlphaBlending::ALPHA_DITHERING:
	{
		a = (a >= Elv::Util::thresholdMatrix[pos.x % 4][pos.y % 4]) ? 1.0f : 0.0f;
		if(a >= 0.99607843137255f) {
			setPixelDithered(pos,colourKernel);
			return true;
		} else return false;
		break;
	}
	case AlphaBlending::ALPHA_BLENDING:
	{
		if(a >= 0.99607843137255f) {
			setPixelDithered(pos,colourKernel);
			return true;
		} else if(a <= 0.003921568627451f) {
			return false;
		} else {
			glm::fvec4 kernel = getPixel(pos);
			const float rem = 1.0f - a;
			kernel.x = (kernel.x * rem) + (colourKernel.x * a);
			kernel.y = (kernel.y * rem) + (colourKernel.y * a);
			kernel.z = (kernel.z * rem) + (colourKernel.z * a);
			kernel.w = std::min(1.0f,kernel.w + a);
			setPixelDithered(pos,kernel);
			return true;
		}
		break;
	}
	default: return false;
	}
}

void IMutableImage2D::blit(const IReadOnlyImage2D& cpy, const glm::uvec2& destinationOffset, const glm::uvec2& sourceOffset, const glm::uvec2& sourceDimensionsFromOffset, bool dither)
{
	const glm::uvec2 sourceDimensions(cpy.getWidth(),cpy.getHeight());
	const Format format = getFormat();
	if((destinationOffset.x > width) || (destinationOffset.y > height) ||
		(sourceOffset.x > sourceDimensions.x) || (sourceOffset.y > sourceDimensions.y)) {
		return;
	}
	const glm::uvec2 maxDim = calculateMaximumOffsetForBlit(glm::uvec2(width,height),sourceDimensions,destinationOffset,sourceOffset,sourceDimensionsFromOffset);
	const size_t stride = pixelByteSize(format)*maxDim.x;
	if (getFormat() == cpy.getFormat()) {
		std::byte * const startPtrForDst = static_cast<std::byte*>(getRawPixels());
		const std::byte * const startPtrForSrc = static_cast<const std::byte*>(cpy.getRawPixels());
		for(unsigned y = 0; y < maxDim.y; ++y) {
			unsigned dstOffsetIndex = (width*(y+destinationOffset.y))+destinationOffset.x;
			unsigned srcOffsetIndex = (sourceDimensions.x*(y+sourceOffset.y))+sourceOffset.x;
			std::memcpy(&startPtrForDst[pixelByteSize(format)*dstOffsetIndex],&startPtrForSrc[pixelByteSize(format)*srcOffsetIndex],stride);
		}
	} else {
		clearToColour((ColourProgrammer)[&cpy,&destinationOffset,&sourceOffset](const glm::uvec2& curPos) {
			const glm::uvec2 offsetIndexInSource = (curPos - destinationOffset)+sourceOffset;
			glm::fvec4 kernel;
			cpy.getPixel(offsetIndexInSource,kernel);
			return kernel;
		}, destinationOffset, maxDim-destinationOffset, dither);
	}
}

void IReadOnlyImage2D::saveInto(DecodeTarget& destination) const
{
	const Format format = getFormat();
	size_t sizeInBytes = static_cast<size_t>(getWidth() * getHeight()) * pixelByteSize(format);
	destination.setFormat(format);
	destination.setIsAnimated(false);
	auto& frame = destination.addFrame(getWidth(),getHeight());
	memcpy(frame.data.data(),getRawPixels(),sizeInBytes);
}

IReadOnlyImage2D::IReadOnlyImage2D()
{

}

IReadOnlyImage2D::IReadOnlyImage2D(unsigned int width, unsigned int height)
	: width(width), height(height)
{
	recalculateDimensions();
}

const sIReadOnlyPalette& IReadOnlyPalettedImage2D::getPalette() const
{
	return palette;
}

void IReadOnlyPalettedImage2D::setPalette(const sIReadOnlyPalette& newPalette)
{
	palette = newPalette;
}

void IReadOnlyPalettedImage2D::setPalette(sIReadOnlyPalette&& newPalette)
{
	palette = std::move(newPalette);
}

sIResizeableImage2D IReadOnlyPalettedImage2D::depalettizeS(std::pmr::memory_resource* memRes) const
{
	return palette->depalettizeS(getIndices(),width, height, memRes);
}

uIResizeableImage2D IReadOnlyPalettedImage2D::depalettizeU(std::pmr::memory_resource* memRes) const
{
	return palette->depalettizeU(getIndices(),width, height, memRes);
}

IReadOnlyPalettedImage2D::IReadOnlyPalettedImage2D()
{

}

IReadOnlyPalettedImage2D::IReadOnlyPalettedImage2D(unsigned int width, unsigned int height)
	: IReadOnlyImage2D(width,height)
{

}

Format IReadOnlyPalettedImage2D::getFormat() const
{
	return Format::INDEXED;
}

void IReadOnlyPalettedImage2D::saveInto(DecodeTarget& destination) const
{
	destination.setFormat(Format::INDEXED);
	Palette& pal = destination.createPalette(palette->getFormat(),256,palette->getTransparentClrIndex());
	palette->saveInto(pal,true);
	Frame& frame = destination.addFrame(width, height);
	auto indices = getIndices();
	std::memcpy(frame.data.data(),indices.data(),indices.size_bytes());
}
void IReadOnlyPalettedImage2D::getPixel(const glm::uvec2& pos, glm::fvec4& colourKernel, Wrap wrap) const
{
	unsigned x,y;
	switch (wrap) {
	case Wrap::REPEAT:
	{
		x = pos.x % width;
		y = pos.y % height;
		break;
	}
	case Wrap::MIRRORED_REPEAT:
	{
		const unsigned doubleWidth = 2 * width;
		const unsigned doubleHeight = 2 * height;
		const unsigned modulo_double_x = pos.x % doubleWidth;
		const unsigned modulo_double_y = pos.y % doubleHeight;
		x = (modulo_double_x < width)
				? pos.x % width
				: (doubleWidth - 1 - (pos.x % width));
		y = (modulo_double_y < height)
				? pos.y % height
				: (doubleHeight - 1 - (pos.y % height));

		break;
	}
	case Wrap::CLAMP_TO_BORDER: // Can't tell the difference between the two
	case Wrap::CLAMP_TO_EDGE:
	{
		x = std::min(pos.x,width-1);
		y = std::min(pos.y,height-1);
		break;
	}
	}
	const uint8_t index = getIndices()[toLinearIndex(width,x,y)];
	palette->getColour(index,colourKernel);
}

void IReadOnlyPalettedImage2D::iterateOverPixels(const ColourIterator& program) const
{
	auto indices = getIndices();
	for(unsigned y = 0; y < height; ++y) {
		const uint8_t* const row = &indices[y*width];
		for(unsigned x = 0; x < width; ++x) {
			const uint8_t index = row[x];
			glm::fvec4 kernel;
			palette->getColour(index,kernel);
			program(glm::uvec2(x,y),kernel);
		}
	}
}

void IReadOnlyPalettedImage2D::iterateOverPixels(const ColourIterator2& program) const
{
	auto indices = getIndices();
	for(unsigned y = 0; y < height; ++y) {
		const uint8_t* const row = &indices[y*width];
		for(unsigned x = 0; x < width; ++x) {
			const uint8_t index = row[x];
			glm::fvec4 kernel;
			palette->getColour(index,kernel);
			program(glm::fvec2(static_cast<float>(x) * widthR,static_cast<float>(y) * heightR),kernel);
		}
	}
}

void IReadOnlyPalettedImage2D::iterateOverPixels(const ColourIterator& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const
{
	auto indices = getIndices();
	const unsigned maxX = std::max(dimensions.x, width);
	const unsigned maxY = std::max(dimensions.y, height);
	for(unsigned y = offset.y; y < maxY; ++y) {
		const uint8_t* const row = &indices[y*width];
		for(unsigned x = offset.x;x < maxX; ++x) {
			const uint8_t index = row[x];
			glm::fvec4 kernel;
			palette->getColour(index,kernel);
			program(glm::uvec2(x,y),kernel);
		}
	}
}

void IReadOnlyPalettedImage2D::iterateOverPixels(const ColourIterator2& program, const glm::uvec2& offset, const glm::uvec2& dimensions) const
{
	auto indices = getIndices();
	const unsigned maxX = std::max(dimensions.x, width);
	const unsigned maxY = std::max(dimensions.y, height);
	for(unsigned y = offset.y; y < maxY; ++y) {
		const uint8_t* const row = &indices[y*width];
		for(unsigned x = offset.x;x < maxX; ++x) {
			const uint8_t index = row[x];
			glm::fvec4 kernel;
			palette->getColour(index,kernel);
			program(glm::fvec2(static_cast<float>(x) * widthR,static_cast<float>(y) * heightR),kernel);
		}
	}
}

int_fast16_t IReadOnlyPalette::getTransparentClrIndex() const
{
	return transparentClrIndex;
}

void IReadOnlyPalette::setTransparentClrIndex(int_fast16_t newTransparentClrIndex)
{
	transparentClrIndex = newTransparentClrIndex;
}

void IReadOnlyPalette::saveInto(Palette& target, bool isPreallocated) const
{
	auto rawColours = getRawColours();
	if(!isPreallocated) {
		target.format = getFormat();
		target.data.resize(rawColours.size_bytes());
		target.transparentColorIndex = getTransparentClrIndex();
	}
	std::memcpy(target.data.data(),rawColours.data(),target.data.size());
}


const void* ReadOnlyPalettedImage2D::getRawPixels() const
{
	return indices.data();
}

const std::span<const uint8_t> ReadOnlyPalettedImage2D::getIndices() const
{
	return indices;
}

IResizeableImage2D::IResizeableImage2D()
{

}

IResizeableImage2D::IResizeableImage2D(unsigned int width, unsigned int height)
	: IMutableImage2D(width,height)
{

}

IMutablePalettedImage2D::IMutablePalettedImage2D()
{

}

IMutablePalettedImage2D::IMutablePalettedImage2D(unsigned int width, unsigned int height)
	: IReadOnlyPalettedImage2D(width, height)
{

}

IResizeablePalettedImage2D::IResizeablePalettedImage2D()
{

}

IResizeablePalettedImage2D::IResizeablePalettedImage2D(unsigned int width, unsigned int height)
	: IMutablePalettedImage2D(width, height)
{

}

ReadOnlyPalettedImage2D::ReadOnlyPalettedImage2D(const ReadOnlyPalettedImage2D& cpy)
	: indices(cpy.indices)
{
	this->palette = cpy.palette;
	IReadOnlyImage2D::operator=(cpy);
}

ReadOnlyPalettedImage2D& ReadOnlyPalettedImage2D::operator=(const ReadOnlyPalettedImage2D& cpy)
{
	this->indices = cpy.indices;
	this->palette = cpy.palette;
	IReadOnlyImage2D::operator=(cpy);
	return *this;
}

ReadOnlyPalettedImage2D::ReadOnlyPalettedImage2D(ReadOnlyPalettedImage2D&& mov)
	: indices(mov.indices)
{
	this->palette = std::move(mov.palette);
	IReadOnlyImage2D::operator=(mov);
}

ReadOnlyPalettedImage2D& ReadOnlyPalettedImage2D::operator=(ReadOnlyPalettedImage2D&& mov)
{
	this->indices = mov.indices;
	this->palette = std::move(mov.palette);
	IReadOnlyImage2D::operator=(mov);
	return *this;
}

ReadOnlyPalettedImage2D::ReadOnlyPalettedImage2D(const std::span<const uint8_t>& indices, unsigned int width, unsigned int height)
	: IReadOnlyPalettedImage2D(width, height), indices(indices)
{
}


void MutablePalettedImage2D::setIndices(const std::span<uint8_t>& newIndices)
{
	indices = newIndices;
}

MutablePalettedImage2D::MutablePalettedImage2D(const MutablePalettedImage2D& cpy)
	: indices(cpy.indices)
{
	IReadOnlyImage2D::operator=(cpy);
	this->palette = cpy.palette;
}

MutablePalettedImage2D& MutablePalettedImage2D::operator=(const MutablePalettedImage2D& cpy)
{
	this->indices = cpy.indices;
	this->palette = cpy.palette;
	IReadOnlyImage2D::operator=(cpy);
	return *this;
}

MutablePalettedImage2D::MutablePalettedImage2D(MutablePalettedImage2D&& mov)
	: indices(mov.indices)
{
	IReadOnlyImage2D::operator=(mov);
	this->palette = std::move(mov.palette);
}

MutablePalettedImage2D& MutablePalettedImage2D::operator=(MutablePalettedImage2D&& mov)
{
	this->indices = mov.indices;
	this->palette = std::move(mov.palette);
	IReadOnlyImage2D::operator=(mov);
	return *this;
}

MutablePalettedImage2D::MutablePalettedImage2D(const std::span<uint8_t>& indices, unsigned int width, unsigned int height)
	: IMutablePalettedImage2D(width,height), indices(indices)
{

}

const void* MutablePalettedImage2D::getRawPixels() const
{
	return indices.data();
}

const std::span<const uint8_t> MutablePalettedImage2D::getIndices() const
{
	return indices;
}

std::span<uint8_t> MutablePalettedImage2D::getIndices()
{
	return indices;
}


std::pmr::memory_resource* ResizeablePalettedImage2D::getMemRes() const
{
	return memRes;
}

void ResizeablePalettedImage2D::setMemRes(std::pmr::memory_resource* newMemRes)
{
	memRes = newMemRes;
}

ResizeablePalettedImage2D::ResizeablePalettedImage2D(const ResizeablePalettedImage2D& cpy)
	: indices(cpy.indices), memRes(cpy.memRes)
{
	this->palette = cpy.palette;
	IReadOnlyImage2D::operator=(cpy);
}

ResizeablePalettedImage2D::ResizeablePalettedImage2D(ResizeablePalettedImage2D&& mov)
	: indices(std::move(mov.indices)), memRes(mov.memRes)
{
	this->palette = std::move(mov.palette);
	IReadOnlyImage2D::operator=(mov);
}

ResizeablePalettedImage2D& ResizeablePalettedImage2D::operator=(const ResizeablePalettedImage2D& cpy)
{
	this->palette = cpy.palette;
	this->indices = cpy.indices;
	this->memRes = cpy.memRes;
	IReadOnlyImage2D::operator=(cpy);
	return *this;
}

ResizeablePalettedImage2D::ResizeablePalettedImage2D(unsigned int width, unsigned int height, std::pmr::memory_resource* memRes)
	: IResizeablePalettedImage2D(width, height), indices(memRes), memRes(memRes)
{
	indices.resize(width * height);
}

ResizeablePalettedImage2D::ResizeablePalettedImage2D(const std::span<const uint8_t>& indices, unsigned int width, unsigned int height, std::pmr::memory_resource* memRes)
	: IResizeablePalettedImage2D(width, height), indices(memRes), memRes(memRes)
{
	this->indices.resize(indices.size());
	std::memcpy(this->indices.data(),indices.data(),indices.size_bytes());
}

ResizeablePalettedImage2D& ResizeablePalettedImage2D::operator=(ResizeablePalettedImage2D&& mov)
{
	this->palette = std::move(mov.palette);
	this->indices = std::move(mov.indices);
	this->memRes = mov.memRes;
	IReadOnlyImage2D::operator=(mov);
	return *this;
}

const void* ResizeablePalettedImage2D::getRawPixels() const
{
	return indices.data();
}

const std::span<const uint8_t> ResizeablePalettedImage2D::getIndices() const
{
	return indices;
}

std::span<uint8_t> ResizeablePalettedImage2D::getIndices()
{
	return indices;
}

bool ResizeablePalettedImage2D::resize(unsigned int newWidth, unsigned int newHeight)
{
	std::pmr::vector<uint8_t> newIndices(memRes);
	newIndices.resize(newWidth*newHeight);
	const unsigned maxWidth = std::min(newWidth, width);
	const unsigned maxHeight = std::min(newHeight, height);
	for(unsigned y = 0; y < maxHeight; ++y) {
		std::memcpy(&newIndices[maxWidth*y],&indices[y*width],maxWidth*sizeof(uint8_t));
	}
	indices = std::move(newIndices);
	width = newWidth;
	height = newHeight;
	recalculateDimensions();
	return true;
}

}
}
}
