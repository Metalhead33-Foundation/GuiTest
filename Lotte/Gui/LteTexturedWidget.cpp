#include "LteTexturedWidget.hpp"
#include <algorithm>
#include <utility>

namespace Lotte {
namespace Gui {

TexturedWidget::TexturedWidget(GuiTextureBinding binding)
	: binding_(std::move(binding))
{
	atlasCoords_.fill(TextureCoordDuo{});
}

TexturedWidget::TexturedWidget(GuiTextureBinding binding,
							   std::span<const TextureCoordDuo> coordDuos)
	: TexturedWidget(std::move(binding))
{
	setAtlasCoordinates(coordDuos);
}

TexturedWidget::TexturedWidget(GuiTextureBinding binding,
							   std::initializer_list<TextureCoordDuo> coordDuos)
	: TexturedWidget(std::move(binding))
{
	setAtlasCoordinates(coordDuos);
}

const TextureCoordDuo& TexturedWidget::atlasCoordinate(std::size_t index) const
{
	return atlasCoords_.at(index);
}

const TextureCoordDuo& TexturedWidget::currentAtlasCoordinate() const noexcept
{
	return atlasCoords_[stateIndex()];
}

void TexturedWidget::setAtlasCoordinate(std::size_t index, TextureCoordDuo coordinates)
{
	atlasCoords_.at(index) = coordinates;
}

void TexturedWidget::setAtlasCoordinates(std::span<const TextureCoordDuo> coordDuos)
{
	const std::size_t count = std::min(coordDuos.size(), atlasCoords_.size());
	std::copy_n(coordDuos.begin(), count, atlasCoords_.begin());
}

void TexturedWidget::setAtlasCoordinates(std::initializer_list<TextureCoordDuo> coordDuos)
{
	const std::size_t count = std::min(coordDuos.size(), atlasCoords_.size());
	std::copy_n(coordDuos.begin(), count, atlasCoords_.begin());
}

bool TexturedWidget::onClick(const glm::fvec2& offset, std::uint8_t button,
							 std::uint8_t state, std::uint8_t clicks)
{
	(void)offset;
	(void)button;
	(void)state;
	(void)clicks;
	// A plain textured widget is a display element. Interactive subclasses override
	// this and return true when they consume the event.
	return false;
}

void TexturedWidget::render(GuiRenderer& renderer)
{
	if (hidden() || binding_.texture == 0) return;
	const TextureCoordDuo& coordinates = currentAtlasCoordinate();
	TexturedQuad quad;
	quad.topLeft = topLeft();
	quad.bottomRight = bottomRight();
	quad.uv[0] = coordinates.first;
	quad.uv[1] = coordinates.second;
	quad.binding = binding_;
	renderer.render(quad);
}

void TexturedWidget::iterateOverChildren(const ChildIterator& iterator)
{
	(void)iterator;
}

void TexturedWidget::iterateOverChildren(const ConstChildIterator& iterator) const
{
	(void)iterator;
}

void TexturedWidget::timedUpdate(float deltaSeconds)
{
	(void)deltaSeconds;
}

} // namespace Gui
} // namespace Lotte
