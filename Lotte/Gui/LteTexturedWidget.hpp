#ifndef LTETEXTUREDWIDGET_HPP
#define LTETEXTUREDWIDGET_HPP
/**
 * @file LteTexturedWidget.hpp
 * @brief Display-only widget whose atlas rectangle is selected by its state.
 *
 * Salvaged from `GUI/MhTexturedWidget.{hpp,cpp}` on the `GuiTest-legacy2` tree.
 * The legacy class was a useful idea - one texture coordinate pair per state
 * combination, indexed by the state bitmask - but it exposed the texture as a
 * `void*` and used `reinterpret_cast`. Here the texture is a
 * @ref Lotte::Gui::GuiTextureBinding, the same type the GUI renderer already
 * consumes, so there is no cast and no hidden ownership.
 *
 * The widget is deliberately concrete but inert: it draws its current rectangle,
 * does not click and does not animate. Interactive or themed widgets will derive
 * from it (or from @ref Lotte::Gui::Widget directly) and override `onClick()`,
 * `render()` or `timedUpdate()`.
 *
 * @note `Widget` signals are not copyable, so `TexturedWidget` is non-copyable
 *       like every other widget. Store widgets through `std::shared_ptr` in
 *       @ref Lotte::Gui::RootNode.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/Gui/LteGuiRenderer.hpp>
#include <Lotte/Gui/LteWidget.hpp>
#include <glm/glm.hpp>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>

namespace Lotte {
namespace Gui {

/**
 * @brief Two corners of an atlas rectangle.
 *
 * `first` is the top-left UV and `second` is the bottom-right UV. The names are
 * kept from the legacy `std::pair` so existing atlas-coordinate tables port with
 * minimal edits.
 */
struct TextureCoordDuo {
	/** @brief Top-left UV. */
	glm::fvec2 first = glm::fvec2(0.0f);
	/** @brief Bottom-right UV. */
	glm::fvec2 second = glm::fvec2(0.0f);
};

/**
 * @brief One atlas rectangle per state combination.
 *
 * Index with @ref Lotte::Gui::Widget::stateIndex(); the array has
 * @ref Lotte::Gui::WidgetStateCount entries, including the empty state.
 */
using AtlasCoordinateCollector = std::array<TextureCoordDuo, WidgetStateCount>;

/** @brief A textured, non-interactive widget. */
class MH_LOTTE_API TexturedWidget : public Widget {
public:
	/**
	 * @brief Creates a widget with no texture; every state maps to a zero UV.
	 * @param binding Texture and sampler used by every state.
	 */
	explicit TexturedWidget(GuiTextureBinding binding = {});

	/**
	 * @brief Creates a widget and copies a per-state atlas table.
	 * @param binding Texture and sampler used by every state.
	 * @param coordDuos Source table; copied up to @ref WidgetStateCount entries.
	 */
	TexturedWidget(GuiTextureBinding binding, std::span<const TextureCoordDuo> coordDuos);

	/**
	 * @brief Creates a widget from an initializer-list atlas table.
	 * @param binding Texture and sampler used by every state.
	 * @param coordDuos Source table; copied up to @ref WidgetStateCount entries.
	 */
	TexturedWidget(GuiTextureBinding binding, std::initializer_list<TextureCoordDuo> coordDuos);

	/** @brief The texture and sampler used by every state. */
	const GuiTextureBinding& binding() const noexcept { return binding_; }
	/** @brief Replaces the texture and sampler. */
	void setBinding(GuiTextureBinding binding) noexcept { binding_ = binding; }

	/** @brief The full per-state atlas table. */
	const AtlasCoordinateCollector& atlasCoordinates() const noexcept { return atlasCoords_; }
	/** @brief Mutable access to the per-state atlas table. */
	AtlasCoordinateCollector& atlasCoordinates() noexcept { return atlasCoords_; }
	/** @brief Atlas rectangle for one explicit state index; throws `std::out_of_range`. */
	const TextureCoordDuo& atlasCoordinate(std::size_t index) const;
	/** @brief Atlas rectangle selected by the current state. */
	const TextureCoordDuo& currentAtlasCoordinate() const noexcept;
	/** @brief Replaces one state's rectangle; throws `std::out_of_range`. */
	void setAtlasCoordinate(std::size_t index, TextureCoordDuo coordinates);
	/** @brief Copies a span; entries beyond the source length are left unchanged. */
	void setAtlasCoordinates(std::span<const TextureCoordDuo> coordDuos);
	/** @brief Copies an initializer list; entries beyond its length are left unchanged. */
	void setAtlasCoordinates(std::initializer_list<TextureCoordDuo> coordDuos);

	/// @name Widget
	///@{
	/** @brief Display-only widget: clicks are ignored. */
	bool onClick(const glm::fvec2& offset, std::uint8_t button,
				 std::uint8_t state, std::uint8_t clicks) override;
	/** @brief Submits one @ref TexturedQuad for the current state. */
	void render(GuiRenderer& renderer) override;
	/** @brief Leaves: no children. */
	void iterateOverChildren(const ChildIterator& iterator) override;
	/** @brief Leaves: no children. */
	void iterateOverChildren(const ConstChildIterator& iterator) const override;
	///@}

protected:
	/** @brief No animation. */
	void timedUpdate(float deltaSeconds) override;

private:
	/** @brief Texture and sampler bound for every state. */
	GuiTextureBinding binding_ {};
	/** @brief Atlas rectangles indexed by @ref Widget::stateIndex(). */
	AtlasCoordinateCollector atlasCoords_ {};
};

} // namespace Gui
} // namespace Lotte

#endif // LTETEXTUREDWIDGET_HPP
