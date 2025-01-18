#include "EuphImage.hpp"
#include <Elvavena/Util/ElvDither.hpp>
#include <Elvavena/Util/ElvEdgeFunction.hpp>
#include <cstring>
namespace Euph {
namespace Media {
namespace Image {

unsigned Image2D::getWidth() const
{
	return width;
}

unsigned Image2D::getHeight() const
{
	return height;
}

unsigned Image2D::getStride() const
{
	return stride;
}

float Image2D::getWidthF() const
{
	return widthF;
}

float Image2D::getHeightF() const
{
	return heightF;
}

float Image2D::getWidthR() const
{
	return widthR;
}

float Image2D::getHeightR() const
{
	return heightR;
}

void Image2D::recalculateDimensions()
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

void Image2D::sample(const glm::fvec2& pos, const glm::ivec2& screenpos, glm::fvec4& colourKernel, TextureFiltering filteringType, Wrap wrap) const
{
	switch (filteringType) {
	case NEAREST_NEIGHBOUR:
		getPixel(glm::ivec2( static_cast<int>( std::round(pos.x*getWidthF())) % getWidth() , static_cast<int>(std::round(pos.y * getHeightF())) % getHeight() ),
				 colourKernel, wrap);
		break;
	case DITHERED:
		{
		glm::fvec2 texelCoords = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF());
		texelCoords += Elv::Util::LOOKUP[screenpos[1]&1][screenpos[0]&1];
		getPixel(glm::ivec2( static_cast<int>(std::round(texelCoords.x))%getWidth(),static_cast<int>(std::round(texelCoords.y))%getHeight() ),colourKernel, wrap);
		break;
		}
	case BILINEAR:
	case THREE_POINT: {
		const glm::fvec2 tmp = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF() );
		const glm::fvec2 coordEdgeTopLeft( std::floor(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeTopRight( std::ceil(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeBottomLeft( std::floor(tmp[0]), std::ceil(tmp[1]) );
		const glm::fvec2 coordEdgeBottomRight( std::ceil(tmp[0]), std::ceil(tmp[1]) );
		const bool bottomRightIsUsed = ((tmp.y - coordEdgeTopLeft.y) * (tmp.x - coordEdgeTopLeft.x)) >= 0.25f;
		const glm::fvec2& thirdPosition = bottomRightIsUsed ? coordEdgeBottomRight : coordEdgeTopLeft;
		glm::fvec4 colourEdgeTopRight, colourEdgeBottomLeft, thirdColour;
		getPixel(glm::ivec2( static_cast<int>(coordEdgeTopRight[0]) ,static_cast<int>(coordEdgeTopRight[1]) ),colourEdgeTopRight,wrap );
		getPixel(glm::ivec2( static_cast<int>(coordEdgeBottomLeft[0]) ,static_cast<int>(coordEdgeBottomLeft[1]) ),colourEdgeBottomLeft,wrap );
		getPixel(glm::ivec2( static_cast<int>(thirdPosition[0]) ,static_cast<int>(thirdPosition[1]) ),thirdColour,wrap );
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

bool Image2D::setPixelWithBlending(const glm::ivec2& pos, const glm::fvec4& colourKernel, AlphaBlending blendingType)
{
	float a = colourKernel.w;
	switch (blendingType) {
	case ALPHA_TESTING:
	{
		if(a >= 0.99607843137255f) {
			setPixelDithered(pos,colourKernel);
			return true;
		} else return false;
		break;
	}
	case ALPHA_DITHERING:
	{
		a = (a >= Elv::Util::thresholdMatrix[pos.x % 4][pos.y % 4]) ? 1.0f : 0.0f;
		if(a >= 0.99607843137255f) {
			setPixelDithered(pos,colourKernel);
			return true;
		} else return false;
		break;
	}
	case ALPHA_BLENDING:
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

void Image2D::saveInto(DecodeTarget& destination) const
{
	const Format format = getFormat();
	size_t sizeInBytes = static_cast<size_t>(getWidth() * getHeight()) * pixelByteSize(format);
	destination.setFormat(format);
	destination.setIsAnimated(false);
	auto& frame = destination.addFrame(getWidth(),getHeight());
	memcpy(frame.data.data(),getRawPixels(),sizeInBytes);
}

}
}
}
