#ifndef LTEGUIROOTNODE_HPP
#define LTEGUIROOTNODE_HPP
/**
 * @file LteGuiRootNode.hpp
 * @brief Owning root of a GUI render tree.
 *
 * Salvaged from `GUI/MhGuiRootNode.{hpp,cpp}` on the `GuiTest-legacy2` tree. The
 * legacy root owned a `std::unordered_set<shared_ptr<Renderable>>` but offered no
 * way to add, remove or inspect children, so it could never actually be used. This
 * version keeps shared ownership but makes the tree usable:
 *
 * - children are stored in a **vector**, in insertion order. The z-index sort is
 *   stable, so equal-z siblings still paint in the order they were added; the old
 *   `unordered_set` made that order implementation-defined.
 * - `addChild()` ignores null and duplicate pointers, and `removeChild()` accepts
 *   either a `shared_ptr` or a raw pointer.
 * - `render()` is still a no-op: the root only establishes the tree. The caller
 *   creates the flat `RenderList` with `updateRenderList()` and invokes
 *   `render()` on each entry. A future GUI loop can wrap that in one call.
 *
 * The root does not store parent pointers, so there is no ownership cycle and a
 * child can be moved between roots by removing it from one and adding it to the
 * other.
 */
#include <Lotte/LotteLib.hpp>
#include <Lotte/Gui/LteGuiRenderable.hpp>
#include <cstddef>
#include <memory>
#include <vector>

namespace Lotte {
namespace Gui {

/** @brief Root node of a GUI tree; owns its children through `shared_ptr`. */
class MH_LOTTE_API RootNode : public Renderable {
public:
	/** @brief Shared ownership type used for children. */
	using ChildPtr = std::shared_ptr<Renderable>;

	/** @brief Creates an empty root at z-index zero. */
	RootNode();

	/**
	 * @brief Adds a child if it is non-null and not already present.
	 * @param child Child to add; ownership is shared with the caller.
	 */
	void addChild(ChildPtr child);
	/**
	 * @brief Removes a child by pointer identity.
	 * @param child Child to remove.
	 * @return True when a child was removed.
	 */
	bool removeChild(const Renderable* child);
	/**
	 * @brief Removes a child by pointer identity.
	 * @param child Child to remove.
	 * @return True when a child was removed.
	 */
	bool removeChild(const ChildPtr& child) { return removeChild(child.get()); }
	/** @brief Removes every child. */
	void clearChildren();

	/** @brief Children in insertion order. */
	const std::vector<ChildPtr>& children() const noexcept { return children_; }
	/** @brief Number of direct children. */
	std::size_t childCount() const noexcept { return children_.size(); }

	/// @name Renderable
	///@{
	/** @brief The root has no geometry of its own. */
	void render(GuiRenderer& renderer) override;
	/** @brief Invokes @p iterator for every direct child, in insertion order. */
	void iterateOverChildren(const ChildIterator& iterator) override;
	/** @brief Invokes @p iterator for every direct child, in insertion order. */
	void iterateOverChildren(const ConstChildIterator& iterator) const override;
	///@}

private:
	/** @brief Owned direct children, in insertion order. */
	std::vector<ChildPtr> children_;
};

} // namespace Gui
} // namespace Lotte

#endif // LTEGUIROOTNODE_HPP
