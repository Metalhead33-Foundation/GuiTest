#include "Texture.hpp"
#include "../Util/Dither.hpp"
#include <cmath>

bool Texture::setPixelWithBlending(const glm::ivec2 &pos, const glm::fvec4 &colourKernel, AlphaBlending blendingType)
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
		a = (a >= thresholdMatrix[pos.x % 4][pos.y % 4]) ? 1.0f : 0.0f;
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

void Texture::sample(const glm::fvec2 &pos, const glm::ivec2 &screenpos, glm::fvec4 &colourKernel, TextureFiltering filteringType, Wrap wrap) const
{
	switch (filteringType) {
	case NEAREST_NEIGHBOUR:
		getPixel(glm::ivec2( static_cast<int>( std::round(pos.x*getWidthF())) % getWidth() , static_cast<int>(std::round(pos.y * getHeightF())) % getHeight() ),
				 colourKernel, wrap);
		break;
	case DITHERED:
		{
		glm::fvec2 texelCoords = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF());
		texelCoords += LOOKUP[screenpos[1]&1][screenpos[0]&1];
		getPixel(glm::ivec2( static_cast<int>(std::round(texelCoords.x))%getWidth(),static_cast<int>(std::round(texelCoords.y))%getHeight() ),colourKernel, wrap);
		break;
		}
	case BILINEAR:
		{
		const int w = getWidth();
		const int h = getHeight();
		const glm::fvec2 tmp = glm::fvec2(pos.x * getWidthF(),pos.y * getHeightF() );
		const glm::fvec2 coordEdgeTopLeft( std::floor(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeTopRight( std::ceil(tmp[0]), std::floor(tmp[1]) );
		const glm::fvec2 coordEdgeBottomLeft( std::floor(tmp[0]), std::ceil(tmp[1]) );
		const glm::fvec2 coordEdgeBottomRight( std::ceil(tmp[0]), std::ceil(tmp[1]) );
		const glm::fvec2 weight = tmp - coordEdgeTopLeft;
		glm::fvec4 colourTopLeft, colourTopRight, colourBottomLeft, colourBottomRight;
		getPixel(glm::ivec2( static_cast<int>(coordEdgeTopLeft[0]) % w,static_cast<int>(coordEdgeTopLeft[1]) % h ),colourTopLeft,wrap );
		getPixel(glm::ivec2( static_cast<int>(coordEdgeTopRight[0]) % w,static_cast<int>(coordEdgeTopRight[1]) % h ),colourTopRight,wrap );
		getPixel(glm::ivec2( static_cast<int>(coordEdgeBottomLeft[0]) % w,static_cast<int>(coordEdgeBottomLeft[1]) % h ),colourBottomLeft,wrap );
		getPixel(glm::ivec2( static_cast<int>(coordEdgeBottomRight[0]) % w,static_cast<int>(coordEdgeBottomRight[1]) % h ),colourBottomRight,wrap );
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
