#include "MhCursor.hpp"
#include <GUI/MhGuiRenderer.hpp>
#include <MhLib/Media/Image/MhImage2D.hpp>
namespace MH33 {
namespace GUI {

uintptr_t Cursor::getTextureId() const
{
	return textureId;
}

void Cursor::setTextureId(uintptr_t newTextureId)
{
	textureId = newTextureId;
}

const glm::fvec2& Cursor::getPosition() const
{
	return position;
}

void Cursor::setPosition(const glm::fvec2& newPosition)
{
	position = newPosition;
}

const glm::fvec2& Cursor::getTextureSize() const
{
	return textureSize;
}

void Cursor::setTextureSize(const glm::fvec2& newTextureSize)
{
	textureSize = newTextureSize;
}

float Cursor::getTextureScale() const
{
	return textureScale;
}

void Cursor::setTextureScale(float newTextureScale)
{
	textureScale = newTextureScale;
}

const glm::fvec2& Cursor::getTextureOffset() const
{
	return textureOffset;
}

void Cursor::setTextureOffset(const glm::fvec2& newTextureOffset)
{
	textureOffset = newTextureOffset;
}

/*
	glm::fvec2 position;
	glm::fvec2 textureOffset;
	glm::fvec2 textureSize;
	float textureScale;
*/

unsigned Cursor::crawlAlpha(const std::vector<bool>& alphaVect, unsigned width, unsigned height)
{
	const unsigned maxCrawl = std::min(width,height);
	for(unsigned dist = 0; dist < maxCrawl; ++dist) {
		const unsigned index = (dist*width)+dist;
		bool sampledAlpha = alphaVect[index];
		if(sampledAlpha) {
			return index;
		}
	}
	return 0;
}

Cursor::Cursor()
	: textureId(0), position(0.0f,0.0f), textureOffset(0.0f, 0.0f), textureSize(0.0f, 0.0f), textureScale(1.0f)
{
}

Cursor::Cursor(const Image::Image2D& image, uintptr_t textureId, const glm::fvec2& screenResolutionReciprocal, float textureScale, bool alphaCrawl)
	: textureId(textureId), position(0.0f,0.0f), textureScale(textureScale)
{
	textureSize = screenResolutionReciprocal * glm::fvec2(static_cast<float>(image.getWidth()),static_cast<float>(image.getHeight()));
	textureSize.y *= -1.0f;
	if(alphaCrawl) {
		std::vector<bool> alphaVector;
		image.produceAlphaMap(alphaVector,0.995f);
		unsigned alphaDistance = crawlAlpha(alphaVector,image.getWidth(),image.getHeight());
		textureOffset = screenResolutionReciprocal * static_cast<float>(alphaDistance);
	} else {
		textureOffset = screenResolutionReciprocal * (textureSize / 2.0f);
	}
}

void Cursor::render(Renderer& renderer)
{
	// textureId
	if(textureId) {
	const auto calculatedOffset = (position - textureOffset);
	renderer.render( TexturedQuad{ .topLeft = calculatedOffset,
								   .bottomRight = calculatedOffset + (textureSize * textureScale),
								   .tcoord = { { 0.0f, 0.0f }, { 1.0f, 1.0f } },
									.texturePtr = textureId }
					 );
	}
}

void Cursor::iterateOverChildren(const ChildIterator& iterator)
{
	(void)iterator;
}

void Cursor::iterateOverChildren(const ConstChildIterator& iterator) const
{
	(void)iterator;
}

}
}
