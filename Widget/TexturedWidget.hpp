#ifndef TEXTUREDWIDGET_H
#define TEXTUREDWIDGET_H

#include "IWidget.hpp"
#include "../Texture/Texture.hpp"
#include <vector>

class TexturedWidget : public IWidget
{
public:
	typedef std::shared_ptr<Texture> sTexture;
protected:
	const int w,h;
	float wf,hf;
	glm::fvec2 topLeft, bottomRight;
	std::vector<bool> alpha;
	sTexture textureDistact, textureActive, textureClicked;
	enum {
		OUT_OF_FOCUS,
		IN_FOCUS,
		CLICKED
	} state;
public:
	TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight,
				   const sTexture& texdis, const sTexture& textact, const sTexture& textclick);
	TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight,
				   sTexture&& texdis, sTexture&& textact, sTexture&& textclick);
	const glm::fvec2& getTopLeft() const override;
	void setTopLeft(const glm::fvec2& newTopLeft);
	const glm::fvec2& getBottomRight() const override;
	void setBottomRight(const glm::fvec2& newBottomRight);
	void offsetPos(const glm::fvec2& posOffset);
	glm::ivec2 translateOffset(const glm::fvec2& offset) const;

	// IWidget interface
	void render(GuiRenderer& renderer) override;
	bool onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) override;
	bool onHover(const glm::fvec2& offset, const glm::fvec2& relativePosToLast) override;
	void onOutOfFocus() override;
};

#endif // TEXTUREDWIDGET_H
