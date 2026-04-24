#ifndef HRKRINGBUFFEREDSTREAMER_HPP
#define HRKRINGBUFFEREDSTREAMER_HPP
#include <Haruka/IO/HrkPlaybackStatePlayable.hpp>
#include <algorithm>
#include <cstring>
#include <memory_resource>
#include <stdexcept>
#include <vector>

namespace Hrk {

inline std::pmr::memory_resource* get_default_memory_resource() {
	return std::pmr::get_default_resource();
}

class MH_HARUKA_API RingBufferedStreamer : public PlaybackStatePlayable
{
private:
	FrameCount ringCapacityFrames;
	std::pmr::vector<float> ringBuffer;
	FrameCount bufferedFrames;
	uintptr_t ringReadFrame;
	FrameIndex expectedStateCursor;
	FrameIndex decoderCursor;
	bool decoderSynced;
	ChannelCount configuredChannels;
	SampleRate configuredSampleRate;
	void setCursorInternal(FrameIndex newCursor);
	void advanceCursorInternal(FrameCount advanceBy);
	void clearBufferedFrames();
	void reconfigureIfNeeded(ChannelCount streamChannels, SampleRate streamSampleRate);
	bool syncDecoderToStateCursor(ChannelCount streamChannels, SampleRate streamSampleRate);
	FrameCount fillBufferedFrames(FrameCount desiredFrames, ChannelCount streamChannels, SampleRate streamSampleRate);
	FrameCount copyBufferedToOutput(Output& output, FrameCount outputFrameOffset, FrameCount frameCount, ChannelCount streamChannels) const;
protected:
	virtual void resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate) = 0;
	virtual FrameCount decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate streamSampleRate) = 0;
	virtual bool seekDecoder(FrameIndex cursor, ChannelCount streamChannels, SampleRate streamSampleRate) = 0;
	virtual bool rewindDecoder(ChannelCount streamChannels, SampleRate streamSampleRate) = 0;
public:
	RingBufferedStreamer(FrameCount ringCapacityFrames = FrameCount(8192), std::pmr::memory_resource* memRes = get_default_memory_resource());
	FrameCount outputTo(Output& output) final;
	void setRingBufferFrameCapacity(FrameCount newCapacityFrames);
	FrameCount getRingBufferFrameCapacity() const;
};

}

#endif // HRKRINGBUFFEREDSTREAMER_HPP
