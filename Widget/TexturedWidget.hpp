#ifndef TEXTUREDWIDGET_H
#define TEXTUREDWIDGET_H

#include "IWidget.hpp"
#include <vector>

class TexturedWidget : public IWidget
{
public:
	typedef std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> uTexture;
protected:
	const int w,h;
	float wf,hf;
	glm::vec2 topLeft, bottomRight;
	std::vector<bool> alpha;
	uTexture textureDistact, textureActive, textureClicked;
	enum {
		OUT_OF_FOCUS,
		IN_FOCUS,
		CLICKED
	} state;
public:
	TexturedWidget(const glm::vec2& topLeft, const glm::vec2& bottomRight, SDL_Renderer& renderer, int w, int h,
				   const uint32_t* texdis, const uint32_t* textact, const uint32_t* textclick);
	const glm::vec2& getTopLeft() const override;
	void setTopLeft(const glm::vec2& newTopLeft);
	const glm::vec2& getBottomRight() const override;
	void setBottomRight(const glm::vec2& newBottomRight);
	void offsetPos(const glm::vec2& posOffset);
	glm::ivec2 translateOffset(const glm::vec2& offset) const;

	// IWidget interface
	void render(SDL_Renderer& renderer, glm::ivec4 viewport) override;
	bool onClick(const glm::vec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) override;
	bool onHover(const glm::vec2& offset, const glm::vec2& relativePosToLast) override;
	void onOutOfFocus() override;
};

#endif // TEXTUREDWIDGET_H
