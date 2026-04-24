#ifndef HRKPLAYBACKSTATE_HPP
#define HRKPLAYBACKSTATE_HPP
#include <Haruka/Core/HrkPlayable.hpp>

namespace Hrk {
class MH_HARUKA_API PlaybackState
{
private:
	FrameIndex cursor;
	PlayStatus playStatus;
	bool repeat;
public:
	PlaybackState();
	PlaybackState(const PlaybackState& cpy) = default;
	PlaybackState(PlaybackState&& mov) = default;
	PlaybackState& operator=(const PlaybackState& cpy) = default;
	PlaybackState& operator=(PlaybackState&& mov) = default;
	FrameIndex getCursor() const;
	void setCursor(Euph::Media::Audio::FrameIndex newCursor);
	PlayStatus getPlayStatus() const;
	void setPlayStatus(PlayStatus newPlayStatus);
	bool getRepeat() const;
	void setRepeat(bool newRepeat);
};

class IHasPlaybackState {
public:
	virtual ~IHasPlaybackState() = default;
	virtual const PlaybackState& getState() const = 0;
	virtual PlaybackState& getState() = 0;
};

}

#endif // HRKPLAYBACKSTATE_HPP
