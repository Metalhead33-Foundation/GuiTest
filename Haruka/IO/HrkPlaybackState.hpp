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
	FrameIndex getCursor() const;
	void setCursor(Euph::Media::Audio::FrameIndex newCursor);
	PlayStatus getPlayStatus() const;
	void setPlayStatus(PlayStatus newPlayStatus);
	bool getRepeat() const;
	void setRepeat(bool newRepeat);
};
}

#endif // HRKPLAYBACKSTATE_HPP
