#include "HrkPlaybackStatePlayable.hpp"
namespace Hrk {
const PlaybackState& PlaybackStatePlayable::getState() const
{
	return state;
}

PlaybackState& PlaybackStatePlayable::getState()
{
	return state;
}
}