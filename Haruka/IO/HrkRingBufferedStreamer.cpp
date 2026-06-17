#include "HrkRingBufferedStreamer.hpp"
namespace Hrk {

void RingBufferedStreamer::setCursorInternal(Euph::Media::Audio::FrameIndex newCursor)
{
	state.setCursor(newCursor);
	expectedStateCursor = newCursor;
}

void RingBufferedStreamer::advanceCursorInternal(Euph::Media::Audio::FrameCount advanceBy)
{
	setCursorInternal(state.getCursor() + advanceBy);
}

void RingBufferedStreamer::clearBufferedFrames()
{
	bufferedFrames = FrameCount(0);
	ringReadFrame = 0;
}

void RingBufferedStreamer::reconfigureIfNeeded(Euph::Media::Audio::ChannelCount streamChannels, Euph::Media::Audio::SampleRate streamSampleRate)
{
	if(streamChannels.var == 0) throw std::runtime_error("Invalid channel count.");
	if(configuredChannels == streamChannels && configuredSampleRate == streamSampleRate) return;
	configuredChannels = streamChannels;
	configuredSampleRate = streamSampleRate;
	const uintptr_t frameCapacity = std::max<uintptr_t>(uintptr_t(1), uintptr_t(ringCapacityFrames.var));
	ringBuffer = std::pmr::vector<float>(frameCapacity * uintptr_t(streamChannels.var), ringBuffer.get_allocator().resource());
	clearBufferedFrames();
	decoderSynced = false;
}

bool RingBufferedStreamer::syncDecoderToStateCursor(Euph::Media::Audio::ChannelCount streamChannels, Euph::Media::Audio::SampleRate streamSampleRate)
{
	if(!decoderSynced || state.getCursor() != expectedStateCursor) {
		clearBufferedFrames();
		if(!seekDecoder(state.getCursor(), streamChannels, streamSampleRate)) return false;
		decoderCursor = state.getCursor();
		expectedStateCursor = state.getCursor();
		decoderSynced = true;
	}
	return true;
}

Euph::Media::Audio::FrameCount RingBufferedStreamer::fillBufferedFrames(Euph::Media::Audio::FrameCount desiredFrames, Euph::Media::Audio::ChannelCount streamChannels, Euph::Media::Audio::SampleRate streamSampleRate)
{
	const FrameCount target(std::min(desiredFrames.var, ringCapacityFrames.var));
	do {
		if(bufferedFrames.var >= target.var) break;
		const FrameCount freeFrames(ringCapacityFrames.var - bufferedFrames.var);
		if(!freeFrames.var) break;
		const uintptr_t writeFrame = (ringReadFrame + bufferedFrames.var) % ringCapacityFrames.var;
		const FrameCount contiguousFrames(std::min<uintptr_t>(freeFrames.var, ringCapacityFrames.var - writeFrame));
		float* writePtr = &ringBuffer[writeFrame * streamChannels.var];
		const FrameCount decoded = decodeFrames(writePtr, contiguousFrames, streamChannels, streamSampleRate);
		if(!decoded.var) {
			if(state.getRepeat()) {
				if(!rewindDecoder(streamChannels, streamSampleRate)) break;
				decoderCursor = FrameIndex(0);
				continue;
			}
			state.setPlayStatus(PlayStatus::STOPPED);
			expectedStateCursor = state.getCursor();
			break;
		}
		bufferedFrames += decoded.var;
		decoderCursor += decoded.var;
	} while(bufferedFrames.var < target.var);
	return bufferedFrames;
}

Euph::Media::Audio::FrameCount RingBufferedStreamer::copyBufferedToOutput(Output& output, Euph::Media::Audio::FrameCount outputFrameOffset, Euph::Media::Audio::FrameCount frameCount, Euph::Media::Audio::ChannelCount streamChannels) const
{
	if(!frameCount.var) return FrameCount(0);
	if(output.interleaving == InterleavingType::INTERLEAVED || output.interleaving == InterleavingType::DONT_CARE) {
		const uintptr_t firstChunkFrames = std::min<uintptr_t>(frameCount.var, ringCapacityFrames.var - ringReadFrame);
		const uintptr_t firstChunkSamples = firstChunkFrames * streamChannels.var;
		const uintptr_t dstSampleOffset = outputFrameOffset.var * streamChannels.var;
		const uintptr_t srcSampleOffset = ringReadFrame * streamChannels.var;
		std::memcpy(&output.dst[dstSampleOffset], &ringBuffer[srcSampleOffset], firstChunkSamples * sizeof(float));
		const uintptr_t secondChunkFrames = frameCount.var - firstChunkFrames;
		if(secondChunkFrames) {
			const uintptr_t secondChunkSamples = secondChunkFrames * streamChannels.var;
			std::memcpy(&output.dst[dstSampleOffset + firstChunkSamples], ringBuffer.data(), secondChunkSamples * sizeof(float));
		}
		return frameCount;
	}

	if(output.interleaving == InterleavingType::SEPARATE_CHANNELS) {
		for(uintptr_t frame = 0; frame < frameCount.var; ++frame) {
			const uintptr_t ringFrame = (ringReadFrame + frame) % ringCapacityFrames.var;
			const float* srcFrame = &ringBuffer[ringFrame * streamChannels.var];
			const uintptr_t outFrame = outputFrameOffset.var + frame;
			for(uintptr_t channel = 0; channel < streamChannels.var; ++channel) {
				output.dst[(channel * output.frameCount.var) + outFrame] = srcFrame[channel];
			}
		}
		return frameCount;
	}

	throw std::runtime_error("Invalid interleaving type.");
}

RingBufferedStreamer::RingBufferedStreamer(Euph::Media::Audio::FrameCount ringCapacityFrames, std::pmr::memory_resource* memRes)
	: ringCapacityFrames(FrameCount(std::max<FrameCount::dataType>(1, ringCapacityFrames.var))),
	ringBuffer(uintptr_t(std::max<FrameCount::dataType>(1, ringCapacityFrames.var)), memRes),
	bufferedFrames(0), ringReadFrame(0), expectedStateCursor(FrameIndex(0)), decoderCursor(FrameIndex(0)),
	decoderSynced(false), configuredChannels(ChannelCount(0)), configuredSampleRate(SAMPLE_RATE_DONT_CARE)
{}

Euph::Media::Audio::FrameCount RingBufferedStreamer::outputTo(Output& output)
{
	if(state.getPlayStatus() != PlayStatus::PLAYING) return FrameCount(0);
	if(!output.frameCount.var) return FrameCount(0);

	ChannelCount streamChannels(0);
	SampleRate streamSampleRate(SAMPLE_RATE_DONT_CARE);
	resolveOutputFormat(output, streamChannels, streamSampleRate);

	if(output.interleaving == InterleavingType::DONT_CARE) {
		output.interleaving = InterleavingType::INTERLEAVED;
	}

	reconfigureIfNeeded(streamChannels, streamSampleRate);
	if(!syncDecoderToStateCursor(streamChannels, streamSampleRate)) {
		state.setPlayStatus(PlayStatus::STOPPED);
		expectedStateCursor = state.getCursor();
		return FrameCount(0);
	}

	FrameCount processed(0);
	FrameCount remaining = output.frameCount;
	do {
		fillBufferedFrames(remaining, streamChannels, streamSampleRate);
		if(!bufferedFrames.var) break;
		const FrameCount toCopy(std::min(remaining.var, bufferedFrames.var));
		copyBufferedToOutput(output, processed, toCopy, streamChannels);
		ringReadFrame = (ringReadFrame + toCopy.var) % ringCapacityFrames.var;
		bufferedFrames -= toCopy.var;
		processed += toCopy.var;
		remaining -= toCopy.var;
		advanceCursorInternal(toCopy);
	} while(remaining.var);

	return processed;
}

void RingBufferedStreamer::setRingBufferFrameCapacity(Euph::Media::Audio::FrameCount newCapacityFrames)
{
	ringCapacityFrames = FrameCount(std::max<FrameCount::dataType>(1, newCapacityFrames.var));
	clearBufferedFrames();
	decoderSynced = false;
	if(configuredChannels.var) {
		const uintptr_t sampleCapacity = uintptr_t(ringCapacityFrames.var) * uintptr_t(configuredChannels.var);
		ringBuffer = std::pmr::vector<float>(sampleCapacity, ringBuffer.get_allocator().resource());
	} else {
		ringBuffer = std::pmr::vector<float>(uintptr_t(ringCapacityFrames.var), ringBuffer.get_allocator().resource());
	}
}

Euph::Media::Audio::FrameCount RingBufferedStreamer::getRingBufferFrameCapacity() const
{
	return ringCapacityFrames;
}

}