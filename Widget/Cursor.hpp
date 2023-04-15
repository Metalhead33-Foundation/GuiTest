#ifndef CURSOR_HPP
#define CURSOR_HPP
//#include "../Texture/Texture.hpp"
//#include "../Pipeline/GuiRenderer.hpp"
#include <MhLib/Media/GFX/MhTexture.hpp>
#include <Renderer/Shared/GuiRenderer.hpp>

namespace SYS {
class TCursor
{
private:
	MH33::GFX::sTexture2D texture;
	glm::ivec2 topLeft;
	glm::ivec2 dimensions;
	float mouseScale;
	void init();
public:
	explicit TCursor(const MH33::GFX::sTexture2D& tex, float mouseScale = 1.0f);
	explicit TCursor(MH33::GFX::sTexture2D&& tex, float mouseScale = 1.0f);
	void render(MH33::GFX::GuiRenderingContext& renderer, const glm::fvec2& mousePos, const glm::fvec2& screenReciprocal);
	float getMouseScale() const;
	void setMouseScale(float newMouseScale);
};
typedef std::shared_ptr<TCursor> sCursor;
}

#endif // CURSOR_HPP
