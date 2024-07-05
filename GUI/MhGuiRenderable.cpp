#include "MhGuiRenderable.hpp"
#include <algorithm>
namespace MH33 {
namespace GUI {

void Renderable::sortRenderList(RenderList& renderList, bool descending)
{
	if(descending) std::sort(renderList.begin(),renderList.end(), [](const IndexedRenderable& a, const IndexedRenderable& b) {
		return a.first > b.first;
	});
	else std::sort(renderList.begin(),renderList.end(), [](const IndexedRenderable& a, const IndexedRenderable& b) {
		return a.first < b.first;
	});
}

void Renderable::updateRenderList(RenderList& renderList, int32_t parentsZIndex, bool isRoot, bool descending)
{
	int32_t indexToPush = parentsZIndex + zIndex;
	renderList.push_back( std::make_pair(indexToPush, this) );
	iterateOverChildren([&](Renderable& child) {
		child.updateRenderList(renderList,indexToPush,false,descending);
	});
	if(isRoot) {
		sortRenderList(renderList, descending);
	}
}

void Renderable::updateRenderList(RenderList& renderList, int32_t& minIndex, int32_t& maxIndex, int32_t parentsZIndex, bool isRoot, bool descending)
{
	int32_t indexToPush = parentsZIndex + zIndex;
	minIndex = std::min(minIndex,indexToPush);
	maxIndex = std::max(maxIndex,indexToPush);
	renderList.push_back( std::make_pair(indexToPush, this) );
	iterateOverChildren([&](Renderable& child) {
		child.updateRenderList(renderList,minIndex,maxIndex,indexToPush,false,descending);
	});
	if(isRoot) {
		sortRenderList(renderList, descending);
	}
}

}
}
