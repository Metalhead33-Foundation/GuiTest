#include <MhLib/Media/Image/MhImage2D.hpp>
#include <MhLib/Util/MhDither.hpp>
#include <MhLib/Util/MhEdgeFunction.hpp>
#include <MhLib/Media/Image/MhPNG.hpp>
namespace MH33 {
namespace Image {

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
		texelCoords += Util::LOOKUP[screenpos[1]&1][screenpos[0]&1];
		getPixel(glm::ivec2( static_cast<int>(std::round(texelCoords.x))%getWidth(),static_cast<int>(std::round(texelCoords.y))%getHeight() ),colourKernel, wrap);
		break;
		}
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
			const float w0 = Util::edgeFunction(coordEdgeBottomLeft, thirdPosition, tmp);
			const float w1 = Util::edgeFunction(thirdPosition, coordEdgeTopRight, tmp);
			const float w2 = Util::edgeFunction(coordEdgeTopRight, coordEdgeBottomLeft, tmp);
			colourKernel = (colourEdgeTopRight * w0) + (colourEdgeBottomLeft * w1) + (thirdColour * w2);
		} else {
			const float w0 = Util::edgeFunction(coordEdgeBottomLeft, coordEdgeTopRight, tmp);
			const float w1 = Util::edgeFunction(coordEdgeTopRight, thirdPosition, tmp);
			const float w2 = Util::edgeFunction(thirdPosition, coordEdgeBottomLeft, tmp);
			colourKernel = (thirdColour * w0) + (colourEdgeBottomLeft * w1) + (colourEdgeTopRight * w2);
		}
		break;
	}
	case BILINEAR:
		{
		const glm::fvec2 tmp = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF() );
		const glm::fvec2 coordEdgeTopLeft( std::floor(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeTopRight( std::ceil(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeBottomLeft( std::floor(tmp[0]), std::ceil(tmp[1]) );
		const glm::fvec2 coordEdgeBottomRight( std::ceil(tmp[0]), std::ceil(tmp[1]) );
		const glm::fvec2 weight = tmp - coordEdgeTopLeft;
		glm::fvec4 colourTopLeft, colourTopRight, colourBottomLeft, colourBottomRight;
		getPixel(glm::ivec2( static_cast<int>(coordEdgeTopLeft[0]) ,static_cast<int>(coordEdgeTopLeft[1]) ),colourTopLeft,wrap );
		getPixel(glm::ivec2( static_cast<int>(coordEdgeTopRight[0]) ,static_cast<int>(coordEdgeTopRight[1]) ),colourTopRight,wrap );
		getPixel(glm::ivec2( static_cast<int>(coordEdgeBottomLeft[0]) ,static_cast<int>(coordEdgeBottomLeft[1]) ),colourBottomLeft,wrap );
		getPixel(glm::ivec2( static_cast<int>(coordEdgeBottomRight[0]) ,static_cast<int>(coordEdgeBottomRight[1]) ),colourBottomRight,wrap );
		colourTopLeft *= ((1.0f-weight[0]) * (1.0f-weight[1]));
		colourTopRight *= (weight[0] * (1.0f-weight[1]));
		colourBottomLeft *= ((1.0f-weight[0]) * weight[1]);
		colourBottomRight *= (weight[0] * weight[1]);
		colourKernel = colourTopLeft + colourTopRight + colourBottomLeft + colourBottomRight;
		break;
		}
	default:
		break;
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
		a = (a >= Util::thresholdMatrix[pos.x % 4][pos.y % 4]) ? 1.0f : 0.0f;
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

void Image2D::blit(const Image2D& cpy, const glm::ivec2 offset, const glm::ivec2& dimensions)
{
	const int width = getWidth();
	const int height = getHeight();

	if(offset.x >= width || offset.y >= height) return;
	const int rowsToCopy = std::min(dimensions.y,height - offset.y);
	const int columnsToCopy = std::min(dimensions.x,width - offset.x);
	glm::ivec2 position(0,0);
	for(position.y = 0; position.y < rowsToCopy; ++position.y) {
		for(position.x = 0; position.x < columnsToCopy; ++position.x) {
			glm::fvec4 kernel = cpy.getPixel(position);
			setPixel(position+offset,kernel);
		}
	}
}

void Image2D::blit(const Image2D& cpy, const glm::ivec2 offset)
{
	const int width = getWidth();
	const int height = getHeight();
	const glm::ivec2 dimensions(cpy.getWidth(),cpy.getHeight());

	if(offset.x >= width || offset.y >= height) return;
	const int rowsToCopy = std::min(dimensions.y,height - offset.y);
	const int columnsToCopy = std::min(dimensions.x,width - offset.x);
	glm::ivec2 position(0,0);
	for(position.y = 0; position.y < rowsToCopy; ++position.y) {
		for(position.x = 0; position.x < columnsToCopy; ++position.x) {
			glm::fvec4 kernel = cpy.getPixel(position);
			setPixel(position+offset,kernel);
		}
	}
}

void Image2D::save(Io::Device& iodev) const
{
	const void * const rawPixelsStart = getRawPixels();
	const int width = getWidth();
	const int height = getHeight();
	const auto format = getFormat();
	const size_t bytesPerRow = byteSize(format) * static_cast<size_t>(width);
	const std::span<const std::byte> bytespan(static_cast<const std::byte*>(rawPixelsStart), bytesPerRow * height);
	PNG::encode(iodev,width,height,format,bytespan, 0.5f);

}

}
}
