#ifndef TEXTUREDWIDGET_H
#define TEXTUREDWIDGET_H
#include "IWidget.hpp"
#include <vector>

namespace SYS {
class TexturedWidget : public IWidget
{
public:
	typedef MH33::GFX::sTexture2D sTexture;
protected:
	const int w,h;
	float wf,hf;
	glm::fvec2 topLeft, bottomRight;
	std::vector<bool> alpha;
	const sTexture textureDistact, textureActive, textureClicked;
	enum {
		OUT_OF_FOCUS,
		IN_FOCUS,
		CLICKED
	} state;
public:
	TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight,
				   const sTexture& texdis, const sTexture& textact, const sTexture& textclick, const std::vector<bool>& alphabitmap);
	TexturedWidget(const glm::fvec2& topLeft, const glm::fvec2& bottomRight,
				   sTexture&& texdis, sTexture&& textact, sTexture&& textclick, std::vector<bool>&& alphabitmap);
	const glm::fvec2& getTopLeft() const override;
	void setTopLeft(const glm::fvec2& newTopLeft);
	const glm::fvec2& getBottomRight() const override;
	void setBottomRight(const glm::fvec2& newBottomRight);
	void offsetPos(const glm::fvec2& posOffset);
	glm::ivec2 translateOffset(const glm::fvec2& offset) const;

	// IWidget interface
	void render(MH33::GFX::GuiRenderingContext& renderer) override;
	bool onClick(const glm::fvec2& offset, uint8_t button, uint8_t mousestate, uint8_t clicks) override;
	bool onHover(const glm::fvec2& offset, const glm::fvec2& relativePosToLast) override;
	void onOutOfFocus() override;
	bool getIsActive() const override;
	bool getIsClicked() const override;
};
}

#endif // TEXTUREDWIDGET_H
