#include "HrkPlaybackState.hpp"

namespace Hrk {

Euph::Media::Audio::FrameIndex PlaybackState::getCursor() const
{
	return cursor;
}

void PlaybackState::setCursor(FrameIndex newCursor)
{
	cursor = newCursor;
}

PlayStatus PlaybackState::getPlayStatus() const
{
	return playStatus;
}

void PlaybackState::setPlayStatus(PlayStatus newPlayStatus)
{
	playStatus = newPlayStatus;
	if(newPlayStatus == PlayStatus::STOPPED)
		cursor.var = 0;
}

bool PlaybackState::getRepeat() const
{
	return repeat;
}

void PlaybackState::setRepeat(bool newRepeat)
{
	repeat = newRepeat;
}

PlaybackState::PlaybackState() {}

}