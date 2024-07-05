#include "MhGuiRootNode.hpp"
namespace MH33 {
namespace GUI {

RootNode::RootNode()
{
	zIndex = 0;
}

void RootNode::render(Renderer& renderer)
{
	(void)renderer;
}

void RootNode::iterateOverChildren(const ChildIterator& iterator)
{
	for(auto& it : children) {
		iterator(*it);
	}
}

void RootNode::iterateOverChildren(const ConstChildIterator& iterator) const
{
	for(const auto& it : children) {
		iterator(*it);
	}
}
}
}
