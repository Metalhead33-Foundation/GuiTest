#include "TexturedWidget.hpp"
#include <SDL2/SDL.h>

namespace SYS {

TexturedWidget::TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight, const sTexture& texdis, const sTexture& textact, const sTexture& textclick, const std::vector<bool>& alphabitmap)
	: topLeft(topLeft), bottomRight(bottomRight), w(texdis->getWidth()), h(texdis->getHeight()), wf(texdis->getWidthF()), hf(texdis->getHeightF()),
	  alpha(std::move(alphabitmap)),
	  textureDistact(texdis),
	  textureActive(textact),
	  textureClicked(textclick),
	  state(OUT_OF_FOCUS)
{

}

TexturedWidget::TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight, sTexture&& texdis, sTexture&& textact, sTexture&& textclick, std::vector<bool>&& alphabitmap)
	: topLeft(topLeft), bottomRight(bottomRight), w(texdis->getWidth()), h(texdis->getHeight()), wf(texdis->getWidthF()), hf(texdis->getHeightF()),
	  alpha(std::move(alphabitmap)),
	  textureDistact(std::move(texdis)),
	  textureActive(std::move(textact)),
	  textureClicked(std::move(textclick)),
	  state(OUT_OF_FOCUS)
{

}

const glm::fvec2& TexturedWidget::getTopLeft() const
{
	return topLeft;
}

void TexturedWidget::setTopLeft(const glm::fvec2& newTopLeft)
{
	topLeft = newTopLeft;
}

const glm::fvec2& TexturedWidget::getBottomRight() const
{
	return bottomRight;
}

void TexturedWidget::offsetPos(const glm::fvec2& posOffset)
{
	topLeft += posOffset;
	bottomRight += posOffset;
}

glm::ivec2 TexturedWidget::translateOffset(const glm::fvec2& offset) const
{
	return glm::ivec2(static_cast<int>(std::round(offset.x * wf)),static_cast<int>(std::round(offset.y * hf)));
}

void TexturedWidget::render(MH33::GFX::GuiRenderingContext& renderer)
{
	renderer.texturedIMesh->ensureVertexCount(4);
	renderer.texturedIMesh->accessVertices([this](void* dst, size_t s) {
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
	/*renderer.texturedIMesh->ensureIndexCount(6);
	renderer.texturedIMesh->accessIndices([](std::span<uint32_t>& indices) {
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 1;
		indices[4] = 2;
		indices[5] = 3;
	});*/
	switch (state) {
		case OUT_OF_FOCUS: renderer.texturedPipeline.renderTriangles(*renderer.texturedIMesh,*textureDistact); break;
		case IN_FOCUS: renderer.texturedPipeline.renderTriangles(*renderer.texturedIMesh,*textureActive); break;
		case CLICKED: renderer.texturedPipeline.renderTriangles(*renderer.texturedIMesh,*textureClicked); break;
	}
}

void TexturedWidget::setBottomRight(const glm::fvec2& newBottomRight)
{
	bottomRight = newBottomRight;
}

bool TexturedWidget::onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks)
{
	const auto noffset = translateOffset(offset);
	if(alpha[(noffset.y*w)+noffset.x]) {
		if(mousestate == SDL_PRESSED) {
		state = CLICKED;
		return true;
		} else {
			state = IN_FOCUS;
			return false;
		}
	} else {
		state = OUT_OF_FOCUS;
		return false;
	}
}

bool TexturedWidget::onHover(const glm::fvec2& offset, const glm::fvec2& relativePosToLast)
{
	const auto noffset = translateOffset(offset);
	if( !alpha[(noffset.y*w)+noffset.x] ) {
		state = OUT_OF_FOCUS;
		return false;
	}
	if(state == CLICKED) {
		offsetPos(relativePosToLast);
	} else state = IN_FOCUS;
	return true;
}

void TexturedWidget::onOutOfFocus()
{
	state = OUT_OF_FOCUS;
}
}


bool SYS::TexturedWidget::getIsActive() const
{
	return state != OUT_OF_FOCUS;
}

bool SYS::TexturedWidget::getIsClicked() const
{
	return state == CLICKED;
}
