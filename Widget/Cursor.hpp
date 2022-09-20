#ifndef CURSOR_HPP
#define CURSOR_HPP
#include "../Texture/Texture.hpp"
#include "../Pipeline/GuiRenderer.hpp"

namespace SYS {
class TCursor
{
private:
	sTexture texture;
	glm::ivec2 topLeft;
	glm::ivec2 dimensions;
	float mouseScale;
	void init();
public:
	explicit TCursor(const sTexture& tex, float mouseScale = 1.0f);
	explicit TCursor(sTexture&& tex, float mouseScale = 1.0f);
	void render(GuiRenderer& renderer, const glm::fvec2& mousePos, const glm::fvec2& screenReciprocal);
	float getMouseScale() const;
	void setMouseScale(float newMouseScale);
};
typedef std::shared_ptr<TCursor> sCursor;
}

#endif // CURSOR_HPP
