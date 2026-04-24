#include "HrkSoundSource.hpp"
namespace Hrk {
const SoundBufferHandle& SoundSource::getBuffer() const
{
	return buffer;
}

void SoundSource::setBuffer(const SoundBufferHandle& newBuffer)
{
	buffer = newBuffer;
}

void SoundSource::setBuffer(SoundBufferHandle&& newBuffer)
{
	buffer = std::move(newBuffer);
}

Euph::Media::Audio::FrameCount SoundSource::outputTo(Output& output)
{
	if(state.getPlayStatus() != PlayStatus::PLAYING) return FrameCount(0);
	if(!buffer.valid()) return FrameCount(0);
	const auto& metadata = buffer.meta();
	if(state.getCursor() >= metadata.frameCount) {
		if(state.getRepeat()) state.setCursor(FrameIndex(0)) ;
		else {
			state.setPlayStatus(PlayStatus::STOPPED);
			return FrameCount(0);
		}
	}
	FrameCount processed(0);
	do {} while(false);
	return processed;
}

const PlaybackState& SoundSource::getState() const
{
	return state;
}

PlaybackState& SoundSource::getState()
{
	return state;
}

SoundSource::SoundSource() {}
}