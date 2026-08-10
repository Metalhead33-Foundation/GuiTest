#include "HrkProxyPlayable.hpp"
#include <utility>
namespace Hrk {

const Playable* ProxyPlayable::getPlayable() const
{
	return playable;
}

Playable* ProxyPlayable::getPlayable()
{
	return playable;
}

void ProxyPlayable::setPlayable(Playable* newPlayable)
{
	ownedPlayable.reset();
	playable = newPlayable;
}

void ProxyPlayable::setPlayable(std::shared_ptr<Playable> newPlayable)
{
	ownedPlayable = std::move(newPlayable);
	playable = ownedPlayable.get();
}

void ProxyPlayable::setPlayable(std::nullptr_t)
{
	ownedPlayable.reset();
	playable = nullptr;
}

}
