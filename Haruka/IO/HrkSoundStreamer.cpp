#include "HrkSoundStreamer.hpp"
namespace Hrk {
const Euph::Media::Audio::SoundFile& SoundStreamer::getSoundfile() const
{
	return soundfile;
}

Euph::Media::Audio::SoundFile& SoundStreamer::getSoundfile()
{
	return soundfile;
}

SoundStreamer::SoundStreamer(Elv::Io::uDevice&& fileDev)
	: soundfile(std::move(fileDev), nullptr)
{}

const PlaybackState& SoundStreamer::getState() const
{
	return state;
}

PlaybackState& SoundStreamer::getState()
{
	return state;
}

FrameCount SoundStreamer::outputTo(Output& output)
{
	// TODO: We'll probably need to implement some sort of ringbuffer to store the samples.
	// I'm fairly sure that decoding on the go is going to be slow.
	// Also, this issue/conundrum/dilemma is shared with ModuleStreamer.
	if(state.getPlayStatus() != PlayStatus::PLAYING) return FrameCount(0);
	if(state.getCursor() >= soundfile.getFrameNum() ) {
		if(state.getRepeat()) { state.setCursor(FrameIndex(0)); }
		else {
			state.setPlayStatus(PlayStatus::STOPPED);
			return FrameCount(0);
		}
	} else soundfile.seekSet(FrameCount(state.getCursor().var)); // It would be so much better, if this happened as soon as we used setCursor!
	FrameCount processed(0);
	// TODO: Maybe replace this naive implementation with a ringbuffer?
	processed = soundfile.readf(output.dst,output.frameCount);
	// And we'll need a more ergonomic means of updating the cursor of the state.
	state.setCursor(state.getCursor() + processed);
	return processed;
}

}