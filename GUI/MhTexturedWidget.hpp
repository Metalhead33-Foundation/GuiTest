#ifndef MHTEXTUREDWIDGET_HPP
#define MHTEXTUREDWIDGET_HPP
#include <GUI/MhWidget.hpp>
#include <array>
#include <initializer_list>
namespace MH33 {
namespace GUI {

class TexturedWidget : public Widget
{
public:
	typedef std::pair<glm::fvec2, glm::fvec2> TextureCoordDuo;
	typedef std::array<TextureCoordDuo, static_cast<uint32_t>(WidgetStateFlags::FULL_STATE)> AtlasCoordinateCollector;
private:
	uintptr_t textureAtlasId;
	AtlasCoordinateCollector atlasCoords;
public:
	TexturedWidget(void* textureAtlasId = nullptr);
	TexturedWidget(void* textureAtlasId, const std::span<const TextureCoordDuo>& coordDuos);
	TexturedWidget(void* textureAtlasId, const std::initializer_list<const TextureCoordDuo>& coordDuos);
	void render(Renderer& renderer) override;
	void* getTextureAtlasId() const;
	void setTextureAtlasId(void* newTextureAtlasId);
	const AtlasCoordinateCollector& getAtlasCoords() const;
	AtlasCoordinateCollector& getAtlasCoords();
};

}
}
#endif // MHTEXTUREDWIDGET_HPP
