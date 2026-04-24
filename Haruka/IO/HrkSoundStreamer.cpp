#include "HrkSoundStreamer.hpp"
#include <Haruka/Core/HrkAudioError.hpp>
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
	// TODO: Add buffered prefetching if direct decode proves too expensive in realtime paths.
	// This concern is shared with ModuleStreamer.
	if(state.getPlayStatus() != PlayStatus::PLAYING) return FrameCount(0);
	if(!output.frameCount.var) return FrameCount(0);

	const ChannelCount sourceChannels = soundfile.getChannels();
	const SampleRate sourceSampleRate = soundfile.getFrameRate();
	const FrameCount sourceFrameCount = soundfile.getFrameNum();
	if(output.channels != sourceChannels) throw ChannelCountMismatchError(output.channels, sourceChannels);
	if(output.samplerate == SAMPLE_RATE_DONT_CARE) {
		output.samplerate = sourceSampleRate;
	} else if(output.samplerate != sourceSampleRate) {
		throw SamplerateMismatchError(output.samplerate, sourceSampleRate);
	}
	if(sourceChannels.var > 1) {
		if(output.interleaving == InterleavingType::DONT_CARE) {
			output.interleaving = InterleavingType::INTERLEAVED;
		} else if(output.interleaving != InterleavingType::INTERLEAVED) {
			throw InterleavingMismatchError(output.interleaving, InterleavingType::INTERLEAVED);
		}
	}
	if(!sourceFrameCount.var) return FrameCount(0);

	FrameCount processed(0);
	FrameCount remaining = output.frameCount;
	// TODO: Replace this direct-read path with a buffered strategy if needed.
	do {
		if(state.getCursor() >= sourceFrameCount) {
			if(state.getRepeat()) {
				state.setCursor(FrameIndex(0));
			} else {
				state.setPlayStatus(PlayStatus::STOPPED);
				break;
			}
		}
		const FrameCount toRead = std::min(remaining, sourceFrameCount - state.getCursor());
		if(!toRead.var) break;
		soundfile.seekSet(FrameCount(state.getCursor().var));
		float* dst = &output.dst[Hrk::framesToSamples(processed, output.channels).var];
		const FrameCount chunk = soundfile.readf(dst, toRead);
		if(!chunk.var) {
			if(!state.getRepeat()) {
				state.setPlayStatus(PlayStatus::STOPPED);
			}
			break;
		}
		state.setCursor(state.getCursor() + chunk);
		processed += chunk.var;
		remaining -= chunk.var;
	} while(remaining.var);
	return processed;
}

}
