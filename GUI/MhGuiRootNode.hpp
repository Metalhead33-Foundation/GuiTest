#ifndef MHGUIROOTNODE_HPP
#define MHGUIROOTNODE_HPP
#include <GUI/MhGuiRenderable.hpp>
#include <unordered_set>
namespace MH33 {
namespace GUI {

class RootNode : public Renderable
{
private:
	std::unordered_set<sRenderable> children;
public:
	RootNode();
	void render(Renderer& renderer) override;
	void iterateOverChildren(const ChildIterator& iterator) override;
	void iterateOverChildren(const ConstChildIterator& iterator) const override;
};

}
}

#endif // MHGUIROOTNODE_HPP
