#include "LteGuiRenderable.hpp"
#include <algorithm>

namespace Lotte {
namespace Gui {

void Renderable::sortRenderList(RenderList& renderList, bool descending)
{
	// Stable sorting is what makes equal z-index ties deterministic: sibling order
	// from iterateOverChildren() survives the sort.
	std::stable_sort(renderList.begin(), renderList.end(),
					 [descending](const IndexedRenderable& a, const IndexedRenderable& b) {
						 return descending ? a.first > b.first : a.first < b.first;
					 });
}

void Renderable::updateRenderList(RenderList& renderList, std::int32_t parentsZIndex,
								  bool isRoot, bool descending)
{
	const std::int32_t indexToPush = parentsZIndex + zIndex_;
	renderList.emplace_back(indexToPush, this);
	iterateOverChildren([&](Renderable& child) {
		child.updateRenderList(renderList, indexToPush, false, descending);
	});
	if (isRoot) sortRenderList(renderList, descending);
}

void Renderable::updateRenderList(RenderList& renderList, std::int32_t& minIndex,
								  std::int32_t& maxIndex, std::int32_t parentsZIndex,
								  bool isRoot, bool descending)
{
	const std::int32_t indexToPush = parentsZIndex + zIndex_;
	minIndex = std::min(minIndex, indexToPush);
	maxIndex = std::max(maxIndex, indexToPush);
	renderList.emplace_back(indexToPush, this);
	iterateOverChildren([&](Renderable& child) {
		child.updateRenderList(renderList, minIndex, maxIndex, indexToPush, false, descending);
	});
	if (isRoot) sortRenderList(renderList, descending);
}

} // namespace Gui
} // namespace Lotte
