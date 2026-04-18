#ifndef HRKPROXYPLAYABLE_HPP
#define HRKPROXYPLAYABLE_HPP
#include <Haruka/Core/HrkPlayable.hpp>
namespace Hrk {

class MH_HARUKA_API ProxyPlayable : public Playable
{
protected:
	Playable* playable = nullptr; //! Ho boy, this is a hard choice to make. On one hand, using raw pointers is brittle, and you normally want std::shared_ptr or std::weak_ptr. On the other hand, using raw pointers lets us allocate Playables on the stack, if need be.
public:
	const Playable* getPlayable() const;
	Playable* getPlayable();
	void setPlayable(Playable* newPlayable);
};

}
#endif // HRKPROXYPLAYABLE_HPP
