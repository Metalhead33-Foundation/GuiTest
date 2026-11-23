#ifndef LTEWIDGET_HPP
#define LTEWIDGET_HPP
/**
 * @file LteWidget.hpp
 * @brief Abstract widget base: state flags, geometry, visibility and signals.
 *
 * Salvaged from `GUI/MhWidget.{hpp,cpp}` on the `GuiTest-legacy2` tree. The class
 * had the right responsibilities - a bitmask of interaction states, a rectangle,
 * a hidden flag and two change notifications - but its implementation had three
 * problems that are fixed here:
 *
 * - `sigslot::signal` was used for the notifications. Lotte already has
 *   @ref Elv::Util::Signal, so the dependency is gone and listeners can use
 *   @ref Elv::Util::ScopedConnection for lifetime safety.
 * - The legacy signal passed raw pointers to `topLeft`/`bottomRight` in an
 *   inconsistent order, and passed `this` where `sigslot` expected a shared
 *   pointer. The new signals deliver the widget pointer plus both rectangle
 *   corners **by value**, in `(topLeft, bottomRight)` order.
 * - `hidden`, `topLeft` and `bottomRight` were uninitialized. They now default to
 *   false and zero.
 *
 * The state is a bitmask, not an enum: `Enabled | Clicked | InMouseFocus` is a
 * valid combination and is what `TexturedWidget` uses to index its per-state
 * texture coordinates. @ref WidgetStateCount is the number of possible
 * combinations (8), the replacement for the legacy `FULL_STATE` enumerator.
 *
 * Input is intentionally backend-neutral. The `onClick()` signature keeps the
 * legacy `(offset, button, state, clicks)` shape, but Lotte does not include SDL;
 * a future input router will fill those bytes from the active backend and call
 * @ref Widget::contains() first. Hit testing and event routing are not in this
 * class.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/Gui/LteGuiRenderable.hpp>
#include <Elvavena/Util/ElvSignal.hpp>
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>

namespace Lotte {
namespace Gui {

/** @brief One interaction-state bit. */
enum class WidgetStateFlag : std::uint32_t {
	Enabled = 1u << 0,      /**< The widget accepts input and is drawn normally. */
	Clicked = 1u << 1,      /**< The primary button is held / the widget is toggled. */
	InMouseFocus = 1u << 2  /**< The pointer is over the widget. */
};

/** @brief Bitmask of @ref WidgetStateFlag values. */
using WidgetState = std::uint32_t;

/** @brief Bits that are meaningful inside a @ref WidgetState. */
inline constexpr WidgetState WidgetStateMask = 0x7u;
/** @brief Number of possible state combinations, including the empty state. */
inline constexpr std::size_t WidgetStateCount = std::size_t(WidgetStateMask) + 1u;

/**
 * @brief Abstract base of every interactive GUI element.
 *
 * A concrete widget has to implement `onClick()`, `timedUpdate()`, `render()` and
 * the two child-iteration methods. Containers override the child iterators;
 * leaves implement them as no-ops.
 */
class MH_LOTTE_API Widget : public Renderable {
public:
	/** @brief State-change notification: widget, old state, new state. */
	using StateSignal = Elv::Util::Signal<Widget*, WidgetState, WidgetState>;
	/** @brief Geometry notification: widget, top-left, bottom-right. */
	using PositionSignal = Elv::Util::Signal<Widget*, glm::fvec2, glm::fvec2>;

	/** @brief Creates an enabled-neutral widget at `(0, 0)` with zero size. */
	Widget();

	/** @brief Current state bitmask. */
	WidgetState state() const noexcept { return state_; }
	/** @brief True when every bit in @p flag is set. */
	bool getFlag(WidgetStateFlag flag) const noexcept;
	/**
	 * @brief Sets or clears one state bit.
	 * @param enabled New value of the bit.
	 * @param flag Bit to change.
	 *
	 * Emits @ref stateChanged only when the resulting bitmask differs.
	 */
	void setFlag(bool enabled, WidgetStateFlag flag);
	/** @brief Convenience overload: `widget.getFlag<WidgetStateFlag::Enabled>()`. */
	template <WidgetStateFlag flag>
	bool getFlag() const noexcept
	{
		return getFlag(flag);
	}
	/** @brief Convenience overload: `widget.setFlag<WidgetStateFlag::Enabled>(true)`. */
	template <WidgetStateFlag flag>
	void setFlag(bool enabled)
	{
		setFlag(enabled, flag);
	}

	/** @brief Index in `[0, WidgetStateCount)` used by per-state assets. */
	std::size_t stateIndex() const noexcept { return std::size_t(state_ & WidgetStateMask); }

	/** @brief Top-left corner in pixels. */
	const glm::fvec2& topLeft() const noexcept { return topLeft_; }
	/** @brief Bottom-right corner in pixels. */
	const glm::fvec2& bottomRight() const noexcept { return bottomRight_; }
	/** @brief `bottomRight - topLeft`, not forced positive. */
	glm::fvec2 size() const noexcept { return bottomRight_ - topLeft_; }

	/**
	 * @brief Replaces the top-left corner and emits @ref positionChanged.
	 * @param newTopLeft New top-left corner in pixels.
	 */
	void setTopLeft(const glm::fvec2& newTopLeft);
	/**
	 * @brief Replaces the bottom-right corner and emits @ref positionChanged.
	 * @param newBottomRight New bottom-right corner in pixels.
	 */
	void setBottomRight(const glm::fvec2& newBottomRight);
	/**
	 * @brief Replaces both corners and emits @ref positionChanged once.
	 * @param newTopLeft New top-left corner in pixels.
	 * @param newBottomRight New bottom-right corner in pixels.
	 */
	void setPosition(const glm::fvec2& newTopLeft, const glm::fvec2& newBottomRight);

	/**
	 * @brief True when @p point lies inside the rectangle.
	 *
	 * The rectangle is normalized component-wise, so a caller may set the corners
	 * in either order. The edges are inclusive.
	 */
	bool contains(const glm::fvec2& point) const noexcept;

	/** @brief True when @ref render() should not draw the widget. */
	bool hidden() const noexcept { return hidden_; }
	/** @brief Enables or disables drawing. */
	void setHidden(bool newHidden) noexcept { hidden_ = newHidden; }

	/**
	 * @brief Delivers a click in widget-local coordinates.
	 * @param offset Click position relative to @ref topLeft().
	 * @param button Backend mouse-button index.
	 * @param state Backend mouse-button state byte (pressed/released flags).
	 * @param clicks Click count for the event.
	 * @return True when the widget handled the event.
	 */
	virtual bool onClick(const glm::fvec2& offset, std::uint8_t button,
						 std::uint8_t state, std::uint8_t clicks) = 0;

	/** @brief Emitted only when the state bitmask actually changes. */
	StateSignal stateChanged;
	/** @brief Emitted by every geometry setter. */
	PositionSignal positionChanged;

protected:
	/**
	 * @brief Publishes a new state bitmask.
	 * @param newState New bitmask; no signal is emitted when it equals the old one.
	 */
	void changeState(WidgetState newState);

	/**
	 * @brief Per-frame animation hook for a concrete widget.
	 * @param deltaSeconds Elapsed time since the previous update, in seconds.
	 *
	 * The base class does not schedule updates; a future root/event loop will.
	 */
	virtual void timedUpdate(float deltaSeconds) = 0;

private:
	/** @brief Current interaction-state bitmask. */
	WidgetState state_ = 0;
	/** @brief When true, renderers skip the widget. */
	bool hidden_ = false;
	/** @brief Top-left corner in pixels. */
	glm::fvec2 topLeft_ = glm::fvec2(0.0f);
	/** @brief Bottom-right corner in pixels. */
	glm::fvec2 bottomRight_ = glm::fvec2(0.0f);
};

} // namespace Gui
} // namespace Lotte

#endif // LTEWIDGET_HPP

