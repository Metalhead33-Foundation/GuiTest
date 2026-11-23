#include "LteGuiRootNode.hpp"
#include <algorithm>
#include <utility>

namespace Lotte {
namespace Gui {

RootNode::RootNode() = default;

void RootNode::addChild(ChildPtr child)
{
	if (!child) return;
	const auto existing = std::find(children_.begin(), children_.end(), child);
	if (existing == children_.end()) children_.push_back(std::move(child));
}

bool RootNode::removeChild(const Renderable* child)
{
	const auto existing = std::find_if(children_.begin(), children_.end(),
									   [child](const ChildPtr& candidate) {
										   return candidate.get() == child;
									   });
	if (existing == children_.end()) return false;
	children_.erase(existing);
	return true;
}

void RootNode::clearChildren()
{
	children_.clear();
}

void RootNode::render(GuiRenderer& renderer)
{
	(void)renderer;
}

void RootNode::iterateOverChildren(const ChildIterator& iterator)
{
	for (const ChildPtr& child : children_) iterator(*child);
}

void RootNode::iterateOverChildren(const ConstChildIterator& iterator) const
{
	for (const ChildPtr& child : children_) iterator(*child);
}

} // namespace Gui
} // namespace Lotte
