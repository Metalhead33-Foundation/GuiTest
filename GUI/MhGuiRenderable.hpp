#ifndef MHGUIRENDERABLE_HPP
#define MHGUIRENDERABLE_HPP
#include <MhLib/Util/MhGlobals.hpp>
#include <vector>
#include <functional>

namespace MH33 {
namespace GUI {

class Renderer;
DEFINE_CLASS(Renderable)
typedef std::pair<int32_t,pRenderable> IndexedRenderable;
typedef std::vector<IndexedRenderable> RenderList;

class Renderable {
public:
	typedef std::function<void(Renderable&)> ChildIterator;
	typedef std::function<void(const Renderable&)> ConstChildIterator;
protected:
	int32_t zIndex;
	static void sortRenderList(RenderList& renderList, bool descending = false);
public:
	virtual ~Renderable() = default;
	void updateRenderList(RenderList& renderList, int32_t parentsZIndex = 0, bool isRoot = false, bool descending = false);
	void updateRenderList(RenderList& renderList, int32_t& minIndex, int32_t& maxIndex, int32_t parentsZIndex = 0, bool isRoot = false, bool descending = false);
	virtual void render(Renderer& renderer) = 0;
	virtual void iterateOverChildren(const ChildIterator& iterator) = 0;
	virtual void iterateOverChildren(const ConstChildIterator& iterator) const = 0;
};

}
}

#endif // MHGUIRENDERABLE_HPP
