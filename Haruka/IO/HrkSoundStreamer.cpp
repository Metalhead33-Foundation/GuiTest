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

SoundStreamer::SoundStreamer(Elv::Io::uDevice&& fileDev, FrameCount ringCapacityFrames, std::pmr::memory_resource* memRes)
	: RingBufferedStreamer(ringCapacityFrames, memRes), soundfile(std::move(fileDev), nullptr)
{}

void SoundStreamer::resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate)
{
	streamChannels = soundfile.getChannels();
	streamSampleRate = soundfile.getFrameRate();
	if(output.channels != streamChannels) throw ChannelCountMismatchError(output.channels, streamChannels);
	if(output.samplerate == SAMPLE_RATE_DONT_CARE) {
		output.samplerate = streamSampleRate;
	} else if(output.samplerate != streamSampleRate) {
		throw SamplerateMismatchError(output.samplerate, streamSampleRate);
	}
	if(streamChannels.var > 1) {
		if(output.interleaving == InterleavingType::DONT_CARE) {
			output.interleaving = InterleavingType::INTERLEAVED;
		} else if(output.interleaving != InterleavingType::INTERLEAVED && output.interleaving != InterleavingType::SEPARATE_CHANNELS) {
			throw InterleavingMismatchError(output.interleaving, InterleavingType::INTERLEAVED);
		}
	}
}

FrameCount SoundStreamer::decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate)
{
	(void)streamChannels;
	return soundfile.readf(dstInterleaved, frameCount);
}

bool SoundStreamer::seekDecoder(FrameIndex cursor, ChannelCount, SampleRate)
{
	if(!soundfile.getFrameNum().var) return false;
	if(cursor >= soundfile.getFrameNum()) {
		if(!state.getRepeat()) return false;
		const auto totalFrames = soundfile.getFrameNum().var;
		cursor = FrameIndex(cursor.var % totalFrames);
	}
	soundfile.seekSet(FrameCount(cursor.var));
	return true;
}

bool SoundStreamer::rewindDecoder(ChannelCount, SampleRate)
{
	soundfile.seekSet(FrameCount(0));
	return true;
}

}
