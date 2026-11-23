#ifndef LTEGUIRENDERABLE_HPP
#define LTEGUIRENDERABLE_HPP
/**
 * @file LteGuiRenderable.hpp
 * @brief Abstract render-tree node with z-index accumulation and child iteration.
 *
 * Salvaged from `GUI/MhGuiRenderable.{hpp,cpp}` on the `GuiTest-legacy2` tree. The
 * original was a tiny base class with one `int32_t` z-index, two `std::function`
 * child iterators and two `updateRenderList()` overloads. It is kept because the
 * idea is sound and because every future widget needs it:
 *
 * - a node contributes its own z-index to its parent's accumulated index, so a
 *   child is always relative to its container;
 * - a root call sorts the finished flat list once, instead of every node sorting
 *   its own children;
 * - `descending` is an explicit argument because a GUI may want either painting
 *   order (higher z on top, or lower z on top, depending on the coordinate
 *   convention of the caller).
 *
 * Changes from the legacy version:
 *
 * - `Renderer` became @ref Lotte::Gui::GuiRenderer, the contract already used by
 *   `LteGuiRenderer.hpp`.
 * - Sorting is `std::stable_sort`, so nodes with equal accumulated z keep their
 *   child-iteration order. The old `std::sort` made equal-z ties unspecified.
 * - `zIndex_` is initialized to zero. The legacy member was uninitialized until a
 *   subclass happened to set it.
 * - The render list holds non-owning pointers; the tree that produced it must stay
 *   alive while the list is consumed.
 */
#include <Lotte/LotteLib.hpp>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace Lotte {
namespace Gui {

class GuiRenderer;
class Renderable;

/** @brief One flat render-list entry: accumulated z-index and non-owning node. */
using IndexedRenderable = std::pair<std::int32_t, Renderable*>;

/** @brief Flat list produced by @ref Renderable::updateRenderList(). */
using RenderList = std::vector<IndexedRenderable>;

/**
 * @brief Base class of every drawable GUI node.
 *
 * A node can be a leaf (a textured widget), a container (a root or layout node), or
 * both (a panel that draws a background and owns children). The class deliberately
 * does not implement visibility, input or layout: those belong to the widget layer
 * or to future composite nodes.
 */
class MH_LOTTE_API Renderable {
public:
	/** @brief Callback invoked for every direct child. */
	using ChildIterator = std::function<void(Renderable&)>;
	/** @brief Callback invoked for every direct child of a const node. */
	using ConstChildIterator = std::function<void(const Renderable&)>;

	/** @brief Releases the node through the base interface. */
	virtual ~Renderable() = default;

	/**
	 * @brief Appends this node and all descendants to a flat list, then sorts it.
	 * @param renderList Destination; appended to, never cleared.
	 * @param parentsZIndex Accumulated z-index of the parent chain.
	 * @param isRoot When true, the finished list is sorted by accumulated z.
	 * @param descending When true, larger z-index values are painted first.
	 */
	void updateRenderList(RenderList& renderList, std::int32_t parentsZIndex = 0,
						  bool isRoot = false, bool descending = false);
	/**
	 * @brief Appends the tree and tracks the minimum/maximum accumulated z-index.
	 * @param renderList Destination; appended to, never cleared.
	 * @param minIndex Minimum accumulated z-index; callers normally initialize it
	 *        to `INT32_MAX`.
	 * @param maxIndex Maximum accumulated z-index; callers normally initialize it
	 *        to `INT32_MIN`.
	 * @param parentsZIndex Accumulated z-index of the parent chain.
	 * @param isRoot When true, the finished list is sorted by accumulated z.
	 * @param descending When true, larger z-index values are painted first.
	 */
	void updateRenderList(RenderList& renderList, std::int32_t& minIndex,
						  std::int32_t& maxIndex, std::int32_t parentsZIndex = 0,
						  bool isRoot = false, bool descending = false);

	/** @brief Submits this node's own geometry; containers usually do nothing. */
	virtual void render(GuiRenderer& renderer) = 0;
	/** @brief Invokes @p iterator for every direct mutable child. */
	virtual void iterateOverChildren(const ChildIterator& iterator) = 0;
	/** @brief Invokes @p iterator for every direct const child. */
	virtual void iterateOverChildren(const ConstChildIterator& iterator) const = 0;

	/** @brief This node's local z-index. */
	std::int32_t zIndex() const noexcept { return zIndex_; }
	/** @brief Replaces the local z-index; the next update pass sees the change. */
	void setZIndex(std::int32_t newZIndex) noexcept { zIndex_ = newZIndex; }

protected:
	/**
	 * @brief Sorts a flat render list by accumulated z-index.
	 * @param renderList List to sort in place.
	 * @param descending When true, larger z-index values come first.
	 * @note Stable: equal z-index entries retain insertion order.
	 */
	static void sortRenderList(RenderList& renderList, bool descending = false);

private:
	/** @brief Local z-index; relative to the accumulated parent index. */
	std::int32_t zIndex_ = 0;
};

} // namespace Gui
} // namespace Lotte

#endif // LTEGUIRENDERABLE_HPP
