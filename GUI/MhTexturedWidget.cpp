#include "MhTexturedWidget.hpp"
#include <GUI/MhGuiRenderer.hpp>
namespace MH33 {
namespace GUI {

void* TexturedWidget::getTextureAtlasId() const
{
	return reinterpret_cast<void*>(textureAtlasId);
}

void TexturedWidget::setTextureAtlasId(void* newTextureAtlasId)
{
	textureAtlasId = reinterpret_cast<uintptr_t>(newTextureAtlasId);
}

const TexturedWidget::AtlasCoordinateCollector& TexturedWidget::getAtlasCoords() const
{
	return atlasCoords;
}

TexturedWidget::AtlasCoordinateCollector& TexturedWidget::getAtlasCoords()
{
	return atlasCoords;
}

TexturedWidget::TexturedWidget(void* textureAtlasId)
	: textureAtlasId(reinterpret_cast<uintptr_t>(textureAtlasId))
{

}

TexturedWidget::TexturedWidget(void* textureAtlasId, const std::span<const TextureCoordDuo>& coordDuos)
	: textureAtlasId(reinterpret_cast<uintptr_t>(textureAtlasId))
{
	std::memcpy(atlasCoords.data(),coordDuos.data(),std::min(atlasCoords.size(),coordDuos.size()) * sizeof(TextureCoordDuo));
}

TexturedWidget::TexturedWidget(void* textureAtlasId, const std::initializer_list<const TextureCoordDuo>& coordDuos)
{
	std::copy(coordDuos.begin(), coordDuos.end(), atlasCoords.begin());
}

void TexturedWidget::render(Renderer& renderer)
{
	if(!hidden) renderer.render( TexturedQuad {
						 .topLeft = topLeft,
						 .bottomRight = bottomRight,
						 .tcoord = { atlasCoords[state].first, atlasCoords[state].second },
						 .texturePtr = textureAtlasId, }
					 );
}

}
}
