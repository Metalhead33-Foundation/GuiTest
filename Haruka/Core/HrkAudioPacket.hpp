#ifndef HRKAUDIOPACKET_HPP
#define HRKAUDIOPACKET_HPP
#include <Euphemy/Media/Audio/EuphAudioIterator.hpp>
namespace Hrk {

using Euph::Media::Audio::ChannelCount;
using Euph::Media::Audio::FrameCount;
using Euph::Media::Audio::FrameIndex;
using Euph::Media::Audio::SampleCount;
using Euph::Media::Audio::SampleRate;

enum class PlayStatus : uint8_t {
	STOPPED,
	PLAYING,
	PAUSED
};

enum class InterleavingType : uint8_t {
	DONT_CARE = 0,
	INTERLEAVED,
	SEPARATE_CHANNELS
};
struct Input {
	const float* src;
	FrameCount frameCount;
	SampleRate samplerate;
	ChannelCount channels;
	InterleavingType interleaving;
};
struct Output {
	float* dst;
	FrameCount frameCount;
	SampleRate samplerate;
	ChannelCount channels;
	InterleavingType interleaving;
};
struct ModificationStream {
	const float* src;
	float* dst;
	FrameCount frameCount;
	SampleRate samplerate;
	ChannelCount channels;
	InterleavingType interleaving;
};

inline SampleCount framesToSamples(const FrameCount& frames, const ChannelCount& channelCount) {
	return Euph::Media::Audio::framesToSamples(frames, channelCount);
}

inline uintptr_t samplesToBytes(const SampleCount& samples) {
	return Euph::Media::Audio::samplesToBytes(samples);
}

};
#endif // HRKAUDIOPACKET_HPP
