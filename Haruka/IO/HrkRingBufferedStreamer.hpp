#ifndef HRKRINGBUFFEREDSTREAMER_HPP
#define HRKRINGBUFFEREDSTREAMER_HPP
/**
 * @file HrkRingBufferedStreamer.hpp
 * @brief Declares the HrkRingBufferedStreamer API in the Haruka/IO module.
 *
 * This header is part of the public declaration surface for Haruka/IO.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/IO/HrkPlaybackStatePlayable.hpp>
#include <algorithm>
#include <cstring>
#include <memory_resource>
#include <stdexcept>
#include <vector>

namespace Hrk {

/** @brief Returns the default PMR resource used by Haruka streamers. */
inline std::pmr::memory_resource* get_default_memory_resource() {
	return std::pmr::get_default_resource();
}

/** @brief Base class for pull-based decoders with a seekable ring buffer. */
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
	/** @brief Resolves the stream format that should satisfy an output request. */
	virtual void resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate) = 0;
	/** @brief Decodes interleaved frames into `dstInterleaved`. */
	virtual FrameCount decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate streamSampleRate) = 0;
	/** @brief Seeks the decoder to the requested cursor. */
	virtual bool seekDecoder(FrameIndex cursor, ChannelCount streamChannels, SampleRate streamSampleRate) = 0;
	/** @brief Rewinds the decoder to the beginning of the stream. */
	virtual bool rewindDecoder(ChannelCount streamChannels, SampleRate streamSampleRate) = 0;
public:
	/** @brief Creates a streamer with the requested ring capacity. */
	RingBufferedStreamer(FrameCount ringCapacityFrames = FrameCount(8192), std::pmr::memory_resource* memRes = get_default_memory_resource());
	/** @brief Produces frames from the ring buffer and decoder. */
	FrameCount outputTo(Output& output) final;
	/** @brief Resizes the ring buffer capacity in frames. */
	void setRingBufferFrameCapacity(FrameCount newCapacityFrames);
	/** @brief Returns the ring buffer capacity in frames. */
	FrameCount getRingBufferFrameCapacity() const;
};

}

#endif // HRKRINGBUFFEREDSTREAMER_HPP
