#ifndef MHCURSOR_HPP
#define MHCURSOR_HPP
#include <GUI/MhGuiRenderable.hpp>
#include <glm/glm.hpp>

namespace MH33 {

namespace Image {
class Image2D;
}

namespace GUI {

DEFINE_CLASS(Cursor)
class Cursor : public Renderable
{
private:
	uintptr_t textureId;
	glm::fvec2 position;
	glm::fvec2 textureOffset;
	glm::fvec2 textureSize;
	float textureScale;
	unsigned crawlAlpha(const std::vector<bool>& alphaVect, unsigned width, unsigned height);
public:
	Cursor();
	Cursor(const Image::Image2D& image, uintptr_t textureId, const glm::fvec2& screenResolutionReciprocal, float textureScale=1.0f, bool alphaCrawl = true);
	void render(Renderer& renderer) override;
	void iterateOverChildren(const ChildIterator& iterator) override;
	void iterateOverChildren(const ConstChildIterator& iterator) const override;
	uintptr_t getTextureId() const;
	void setTextureId(uintptr_t newTextureId);
	const glm::fvec2& getPosition() const;
	void setPosition(const glm::fvec2& newPosition);
	const glm::fvec2& getTextureSize() const;
	void setTextureSize(const glm::fvec2& newTextureSize);
	float getTextureScale() const;
	void setTextureScale(float newTextureScale);
	const glm::fvec2& getTextureOffset() const;
	void setTextureOffset(const glm::fvec2& newTextureOffset);
};

}
}

#endif // MHCURSOR_HPP
