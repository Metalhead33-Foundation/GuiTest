#ifndef HRKSOUNDBUFFER_HPP
#define HRKSOUNDBUFFER_HPP
#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>
#include <Elvavena/Util/ElvIntrusiveArray.hpp>

namespace Hrk {

struct SoundBufferMetadata {
	// Metadata — small integers, pack nicely alongside the control block
	FrameCount                  frameCount;
	SampleRate                  sampleRate;
	ChannelCount                channels;
	// InterleavingType is relevant here too — you'll want to know this when
	// the buffer is consumed, and it's free to store (just one byte)
	InterleavingType            interleaving;
};

typedef Elv::Util::IntrusiveArray<float, SoundBufferMetadata,32> SoundBufferHandle;

}
#endif // HRKSOUNDBUFFER_HPP
