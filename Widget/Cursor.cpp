#include "Cursor.hpp"
#include <limits>

namespace SYS {
static const float magicAlpha = 0.995f;

float TCursor::getMouseScale() const
{
	return mouseScale;
}

void TCursor::setMouseScale(float newMouseScale)
{
	mouseScale = newMouseScale;
}

void TCursor::init()
{
	dimensions = glm::ivec2(texture->getWidth(),texture->getHeight());
	const int maxCrawl = std::min(dimensions.x,dimensions.y);
	/*glm::vec4 kernel;
	for(int dist = 0; dist < maxCrawl; ++dist) {
		if(!dist) {
			texture->getPixel(glm::ivec2(0,0),kernel);
			if(kernel.w > magicAlpha) {
				topLeft = glm::ivec2(0,0);
				break;
			}
		} else if(topLeft.x >= 0 && topLeft.y >= 0) {
			break;
		}
		if(topLeft.y < 0) for(int x = 0; x < dist; ++x) {
			texture->getPixel(glm::ivec2(x,dist),kernel);
			if(kernel.w >= magicAlpha) {
				topLeft.y = dist;
			}
		}
		if(topLeft.x < 0) for(int y = 0; y < dist; ++y) {
			texture->getPixel(glm::ivec2(dist,y),kernel);
			if(kernel.w >= magicAlpha) {
				topLeft.x = dist;
			}
		}
	}*/
}

/*

	explicit TCursor(const MH33::GFX::sTexture2D& tex, float mouseScale = 1.0f);
	explicit TCursor(MH33::GFX::sTexture2D&& tex, float mouseScale = 1.0f);
	void render(MH33::GFX::GuiRenderingContext& renderer, const glm::fvec2& mousePos, const glm::fvec2& screenReciprocal);
*/

TCursor::TCursor(const MH33::GFX::sTexture2D& tex, float mouseScale)
	: texture(tex), topLeft(-1,-1), mouseScale(mouseScale)
{
	init();
}

TCursor::TCursor(MH33::GFX::sTexture2D&& tex, float mouseScale)
	: texture(std::move(tex)), topLeft(-1,-1), mouseScale(mouseScale)
{
	init();
}

void TCursor::render(MH33::GFX::GuiRenderingContext& renderer, const glm::fvec2& mousePos, const glm::fvec2& screenReciprocal)
{
	const glm::fvec2 topLeft = mousePos - (glm::fvec2(static_cast<float>(this->topLeft.x) * screenReciprocal.x,static_cast<float>(this->topLeft.y) * screenReciprocal.y) * mouseScale);
	const glm::fvec2 bottomRight = topLeft + (glm::fvec2(static_cast<float>(dimensions.x)*screenReciprocal.x,static_cast<float>(dimensions.y) * screenReciprocal.y) * mouseScale);

	renderer.texturedIMesh->ensureVertexCount(4);
	renderer.texturedIMesh->accessVertices([&topLeft,&bottomRight](void* dst, size_t s) {
		std::span<Renderer::TexturedWidgetVert> span(static_cast<Renderer::TexturedWidgetVert*>(dst),s);
		span[0].POS = glm::fvec2(std::min(topLeft.x,bottomRight.x),-1.0f * std::min(topLeft.y,bottomRight.y));
		span[0].TEXCOORD = glm::fvec2(0.0f, 0.0f);
		span[1].POS = glm::fvec2(std::max(topLeft.x,bottomRight.x),-1.0f * std::min(topLeft.y,bottomRight.y));
		span[1].TEXCOORD = glm::fvec2(1.0f, 0.0f);
		span[2].POS = glm::fvec2(std::min(topLeft.x,bottomRight.x),-1.0f * std::max(topLeft.y,bottomRight.y));
		span[2].TEXCOORD = glm::fvec2(0.0f, 1.0f);
		span[3].POS = glm::fvec2(std::max(topLeft.x,bottomRight.x),-1.0f * std::max(topLeft.y,bottomRight.y));
		span[3].TEXCOORD = glm::fvec2(1.0f, 1.0f);
	});
	renderer.texturedPipeline.renderTriangles(*renderer.texturedIMesh,*texture);
}
}
