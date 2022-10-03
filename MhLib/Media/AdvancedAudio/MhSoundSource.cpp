#include <MhLib/Media/AdvancedAudio/MhSoundSource.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>
#include <cstring>

namespace MH33 {
namespace Audio {
FrameIndex SoundSource::getCursor() const
{
	return cursor;
}

void SoundSource::setCursor(const FrameIndex &value)
{
	cursor = value;
}

PlayStatus SoundSource::getState() const
{
	return state;
}

void SoundSource::setState(const PlayStatus &value)
{
	state = value;
	if(state == PlayStatus::STOPPED) cursor = FrameIndex(0);
}

bool SoundSource::getRepeating() const
{
	return repeating;
}

void SoundSource::setRepeating(bool value)
{
	repeating = value;
}

FrameRate SoundSource::getFrameRate() const
{
	return buffer ? buffer->getFrameRate() : FrameRate(0);
}

ChannelCount SoundSource::getChannelCount() const
{
	return buffer ? buffer->getChannelCount() : ChannelCount(0);
}

SoundSource::SoundSource()
	: buffer(nullptr), state(PlayStatus::STOPPED), repeating(false)
{

}

SoundSource::SoundSource(const sBuffer &value)
	: buffer(value), state(PlayStatus::STOPPED), repeating(false)
{

}

SoundSource::SoundSource(sBuffer &&value)
	: buffer(std::move(value)), state(PlayStatus::STOPPED), repeating(false)
{

}

FrameCount SoundSource::outputTo(const Output &dst)
{
	if(state != PlayStatus::PLAYING) return FrameCount(0);
	Input src;
	if(buffer) buffer->provideInput(src,cursor);
	else return FrameCount(0);
	if(dst.channels != src.channels) throw ChannelCountMismatchError(dst.channels,src.channels);
	if(dst.framerate != src.framerate) throw FrameratetMismatchError(dst.framerate,src.framerate);
	if(dst.channels.var > 1 && dst.interleaving != src.interleaving) throw InterleavingMismatchError(dst.interleaving,src.interleaving);
	FrameCount toSend = std::min(src.frameCount,dst.frameCount);
	if(toSend.var) {
		memcpy(dst.dst,src.src,samplesToBytes(framesToSamples(toSend,src.channels)));
		cursor.var += toSend.var;
	} else {
		if(!repeating) state = PlayStatus::STOPPED;
		cursor = FrameIndex(0);
	}
	return toSend;
}

const sBuffer& SoundSource::getBuffer() const
{
	return buffer;
}

sBuffer &SoundSource::getBuffer()
{
	return buffer;
}

void SoundSource::setBuffer(const sBuffer &value)
{
	buffer = value;
}

void SoundSource::setBuffer(sBuffer &&value)
{
	buffer = std::move(value);
}

}
}
