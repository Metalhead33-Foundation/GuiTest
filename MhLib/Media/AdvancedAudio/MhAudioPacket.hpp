#ifndef MHAUDIOPACKET_HPP
#define MHAUDIOPACKET_HPP
#include <MhLib/Media/Audio/MhAudioIterator.hpp>
namespace MH33 {
namespace Audio {

enum InterleavingType {
	DONT_CARE = 0,
	INTERLEAVED,
	SEPARATE_CHANNELS
};

struct Input {
	const float* src;
	FrameCount frameCount;
	FrameRate framerate;
	ChannelCount channels;
	InterleavingType interleaving;
};
struct Output {
	float* dst;
	FrameCount frameCount;
	FrameRate framerate;
	ChannelCount channels;
	InterleavingType interleaving;
};
struct ModificationStream {
	const float* src;
	float* dst;
	FrameCount frameCount;
	FrameRate framerate;
	ChannelCount channels;
	InterleavingType interleaving;
};

}
}
#endif // MHAUDIOPACKET_HPP
