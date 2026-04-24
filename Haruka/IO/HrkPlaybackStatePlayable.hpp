#ifndef HRKPLAYBACKSTATEPLAYABLE_HPP
#define HRKPLAYBACKSTATEPLAYABLE_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <Haruka/IO/HrkPlaybackState.hpp>

namespace Hrk {

class MH_HARUKA_API PlaybackStatePlayable : public Playable, public IHasPlaybackState
{
protected:
	PlaybackState state;
public:
	const PlaybackState& getState() const override;

	PlaybackState& getState() override;
};

}

#endif // HRKPLAYBACKSTATEPLAYABLE_HPP
