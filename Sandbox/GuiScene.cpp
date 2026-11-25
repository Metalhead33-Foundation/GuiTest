#include "GuiScene.hpp"
#include <Euphemy/Media/Image/EuphPNG.hpp>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Sandbox {

namespace {

/** @brief Maps a visual state to its index in @ref ButtonStyle::colours. */
constexpr std::size_t colourIndex(ButtonVisual visual) noexcept
{
	return static_cast<std::size_t>(visual);
}

} // namespace

ScriptTexture::ScriptTexture(std::unique_ptr<Lotte::Gfx::WriteableTexture2D> newTexture,
							 std::string path)
	: texture(std::move(newTexture)), sourcePath(std::move(path))
{
	if (texture && texture->isValid()) {
		bindingInfo.texture = texture->handle();
	}
}

std::uint32_t ScriptTexture::width() const noexcept
{
	return texture ? texture->width() : 0u;
}

std::uint32_t ScriptTexture::height() const noexcept
{
	return texture ? texture->height() : 0u;
}

ScriptButton::ScriptButton(Lotte::Gui::GuiTextureBinding binding, const ButtonStyle& newStyle)
	: Lotte::Gui::TexturedWidget(binding, newStyle.coordinates), style(newStyle)
{
	// A script-created button is usable unless it says otherwise: the legacy API
	// defaulted `enabled` to false, which every script then had to undo.
	setFlag<Lotte::Gui::WidgetStateFlag::Enabled>(true);
}

bool ScriptButton::onClick(const glm::fvec2& offset, std::uint8_t button,
						   std::uint8_t state, std::uint8_t clicks)
{
	(void)button;
	(void)clicks;
	if (!getFlag(Lotte::Gui::WidgetStateFlag::Enabled)) {
		return true;
	}
	// `offset` is widget-local, so "inside" is something the widget can decide
	// itself: that is what lets a release delivered outside the rectangle cancel
	// the press instead of activating it.
	const glm::fvec2 extent = size();
	const bool inside = offset.x >= 0.0f && offset.y >= 0.0f &&
						offset.x <= extent.x && offset.y <= extent.y;
	if (state != 0) {
		if (!inside) return true;
		pressInside = true;
		setFlag<Lotte::Gui::WidgetStateFlag::Clicked>(true);
		return true;
	}
	const bool completed = pressInside && inside;
	pressInside = false;
	setFlag<Lotte::Gui::WidgetStateFlag::Clicked>(false);
	if (completed) {
		activated();
	}
	return true;
}

void ScriptButton::setHovered(bool hovered)
{
	setFlag<Lotte::Gui::WidgetStateFlag::InMouseFocus>(hovered);
}

bool ScriptButton::pressed() const noexcept
{
	return getFlag(Lotte::Gui::WidgetStateFlag::Clicked);
}

void ScriptButton::setButtonStyle(const ButtonStyle& newStyle)
{
	style = newStyle;
	setAtlasCoordinates(style.coordinates);
}

glm::fvec4 ScriptButton::currentTint() const noexcept
{
	if (!getFlag(Lotte::Gui::WidgetStateFlag::Enabled)) {
		return style.colours[colourIndex(ButtonVisual::Disabled)];
	}
	if (getFlag(Lotte::Gui::WidgetStateFlag::Clicked)) {
		return style.colours[colourIndex(ButtonVisual::Pressed)];
	}
	if (getFlag(Lotte::Gui::WidgetStateFlag::InMouseFocus)) {
		return style.colours[colourIndex(ButtonVisual::Hover)];
	}
	return style.colours[colourIndex(ButtonVisual::Idle)];
}

void ScriptButton::render(Lotte::Gui::GuiRenderer& renderer)
{
	if (hidden()) return;
	const glm::fvec4 tint = currentTint();
	if (binding().texture != 0) {
		Lotte::Gui::ColouredTexturedQuad quad;
		quad.topLeft = topLeft();
		quad.bottomRight = bottomRight();
		const Lotte::Gui::TextureCoordDuo& coordinates = currentAtlasCoordinate();
		quad.uv[0] = coordinates.first;
		quad.uv[1] = coordinates.second;
		quad.binding = binding();
		quad.colour = tint;
		renderer.render(quad);
		return;
	}
	Lotte::Gui::ColouredQuad quad;
	quad.topLeft = topLeft();
	quad.bottomRight = bottomRight();
	quad.colour = tint;
	renderer.render(quad);
}

GuiScene::GuiScene(Lotte::Gfx::ResourceFactory& factory) : factory(factory) {}

GuiScene::TexturePtr GuiScene::loadTexture(Elv::Io::System& io, const std::string& path)
{
	const auto cached = textures.find(path);
	if (cached != textures.end()) {
		return cached->second;
	}
	Elv::Io::uDevice file = io.openUnique(path.c_str(), Elv::Io::Mode::READ);
	if (!file || !file->isValid()) {
		throw std::runtime_error("Could not open texture image '" + path + "'");
	}
	Euph::Media::Image::DecodeTarget decoded;
	Euph::Media::Image::PNG::decode(*file, decoded);
	auto surface = std::make_unique<Lotte::Gfx::WriteableTexture2D>(
		Lotte::Gfx::WriteableTexture2D::fromDecodeTarget(factory, decoded));
	if (!surface->isValid()) {
		throw std::runtime_error("Could not upload texture image '" + path + "'");
	}
	auto entry = std::make_shared<ScriptTexture>(std::move(surface), path);
	textures.emplace(path, entry);
	return entry;
}

void GuiScene::addButton(ButtonPtr button)
{
	if (!button) return;
	// The keep-alive list is what makes a script handle safe: `removeButton()`
	// only detaches the widget from the tree, it never frees it behind the
	// script's back.
	if (std::find(buttons.begin(), buttons.end(), button) == buttons.end()) {
		buttons.push_back(button);
	}
	root.addChild(std::move(button));
}

bool GuiScene::removeButton(const ScriptButton* button)
{
	return button != nullptr && root.removeChild(button);
}

void GuiScene::clear()
{
	root.clearChildren();
}

void GuiScene::setMousePosition(glm::fvec2 position)
{
	mouse = position;
	for (const auto& child : root.children()) {
		if (auto* button = dynamic_cast<ScriptButton*>(child.get())) {
			button->setHovered(!button->hidden() && button->contains(position));
		}
	}
}

void GuiScene::setMouseButton(glm::fvec2 position, std::uint8_t button, bool down,
							  std::uint8_t clicks)
{
	mouse = position;
	const std::uint8_t state = down ? 1u : 0u;
	for (const auto& child : root.children()) {
		auto* candidate = dynamic_cast<ScriptButton*>(child.get());
		if (candidate == nullptr || candidate->hidden()) continue;
		const bool inside = candidate->contains(position);
		candidate->setHovered(inside);
		if (inside) {
			candidate->onClick(position - candidate->topLeft(), button, state, clicks);
		} else if (candidate->pressed()) {
			// The release happened outside the button: deliver it so the widget
			// clears its pressed state, and let it decide not to activate.
			candidate->onClick(position - candidate->topLeft(), button, 0u, clicks);
		}
	}
}

void GuiScene::render(Lotte::Gui::GuiRenderer& renderer)
{
	Lotte::Gui::RenderList list;
	// The root itself is part of the list; its `render()` is a no-op by design.
	root.updateRenderList(list, 0, true, false);
	for (const auto& [zIndex, node] : list) {
		(void)zIndex;
		if (node != nullptr) node->render(renderer);
	}
}

} // namespace Sandbox
