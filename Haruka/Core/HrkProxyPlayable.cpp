#include "HrkProxyPlayable.hpp"
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
	playable = newPlayable;
}

}
