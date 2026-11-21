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
	ringBuffer.clear();
}

void RingBufferedStreamer::reconfigureIfNeeded(Euph::Media::Audio::ChannelCount streamChannels, Euph::Media::Audio::SampleRate streamSampleRate)
{
	if(streamChannels.var == 0) throw std::runtime_error("Invalid channel count.");
	if(configuredChannels == streamChannels && configuredSampleRate == streamSampleRate) return;
	configuredChannels = streamChannels;
	configuredSampleRate = streamSampleRate;
	const uintptr_t frameCapacity = std::max<uintptr_t>(uintptr_t(1), uintptr_t(ringCapacityFrames.var));
	ringBuffer = SampleBuffer(frameCapacity * uintptr_t(streamChannels.var),
		std::pmr::polymorphic_allocator<float>(memoryResource));
	decoderSynced = false;
}

Euph::Media::Audio::FrameCount RingBufferedStreamer::bufferedFrames() const
{
	if(configuredChannels.var == 0) return FrameCount(0);
	return FrameCount(static_cast<FrameCount::dataType>(ringBuffer.size() / configuredChannels.var));
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
	for(;;) {
		if(bufferedFrames().var >= target.var) break;
		auto writeSpan = ringBuffer.writeSpan();
		if(writeSpan.empty()) break;
		const uintptr_t spanFrames = writeSpan.size() / streamChannels.var;
		if(!spanFrames) break;
		const FrameCount wanted(std::min<uintptr_t>(target.var - bufferedFrames().var, spanFrames));
		const FrameCount decoded = decodeFrames(writeSpan.data(), wanted, streamChannels, streamSampleRate);
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
		const uintptr_t actual = std::min<uintptr_t>(decoded.var, wanted.var);
		ringBuffer.commitWrite(actual * streamChannels.var);
		decoderCursor += actual;
	}
	return bufferedFrames();
}

Euph::Media::Audio::FrameCount RingBufferedStreamer::copyBufferedToOutput(Output& output, Euph::Media::Audio::FrameCount outputFrameOffset, Euph::Media::Audio::FrameCount frameCount, Euph::Media::Audio::ChannelCount streamChannels) const
{
	if(!frameCount.var) return FrameCount(0);
	if(output.interleaving == InterleavingType::INTERLEAVED || output.interleaving == InterleavingType::DONT_CARE) {
		uintptr_t remainingFrames = frameCount.var;
		uintptr_t sourceSampleOffset = 0;
		uintptr_t destinationSampleOffset = outputFrameOffset.var * streamChannels.var;
		while(remainingFrames) {
			const auto source = ringBuffer.readSpan(sourceSampleOffset);
			if(source.empty()) break;
			const uintptr_t chunkFrames = std::min<uintptr_t>(remainingFrames,
				source.size() / streamChannels.var);
			const uintptr_t chunkSamples = chunkFrames * streamChannels.var;
			std::memcpy(&output.dst[destinationSampleOffset], source.data(),
				chunkSamples * sizeof(float));
			destinationSampleOffset += chunkSamples;
			sourceSampleOffset += chunkSamples;
			remainingFrames -= chunkFrames;
		}
		return FrameCount(static_cast<FrameCount::dataType>(frameCount.var - remainingFrames));
	}

	if(output.interleaving == InterleavingType::SEPARATE_CHANNELS) {
		for(uintptr_t frame = 0; frame < frameCount.var; ++frame) {
			const uintptr_t sourceSample = frame * streamChannels.var;
			const uintptr_t outFrame = outputFrameOffset.var + frame;
			for(uintptr_t channel = 0; channel < streamChannels.var; ++channel) {
				output.dst[(channel * output.frameCount.var) + outFrame] = ringBuffer[sourceSample + channel];
			}
		}
		return frameCount;
	}

	throw std::runtime_error("Invalid interleaving type.");
}

RingBufferedStreamer::RingBufferedStreamer(Euph::Media::Audio::FrameCount ringCapacityFrames, std::pmr::memory_resource* memRes)
	: ringCapacityFrames(FrameCount(std::max<FrameCount::dataType>(1, ringCapacityFrames.var))),
	memoryResource(memRes != nullptr ? memRes : get_default_memory_resource()),
	ringBuffer(uintptr_t(std::max<FrameCount::dataType>(1, ringCapacityFrames.var)),
		std::pmr::polymorphic_allocator<float>(memoryResource)),
	expectedStateCursor(FrameIndex(0)), decoderCursor(FrameIndex(0)),
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
		const FrameCount buffered = bufferedFrames();
		if(!buffered.var) break;
		const FrameCount toCopy(std::min<uintptr_t>(remaining.var, buffered.var));
		copyBufferedToOutput(output, processed, toCopy, streamChannels);
		ringBuffer.consume(toCopy.var * streamChannels.var);
		processed += toCopy.var;
		remaining -= toCopy.var;
		advanceCursorInternal(toCopy);
	} while(remaining.var);

	return processed;
}

void RingBufferedStreamer::setRingBufferFrameCapacity(Euph::Media::Audio::FrameCount newCapacityFrames)
{
	ringCapacityFrames = FrameCount(std::max<FrameCount::dataType>(1, newCapacityFrames.var));
	decoderSynced = false;
	const uintptr_t channels = configuredChannels.var ? configuredChannels.var : uintptr_t(1);
	ringBuffer = SampleBuffer(uintptr_t(ringCapacityFrames.var) * channels,
		std::pmr::polymorphic_allocator<float>(memoryResource));
}

Euph::Media::Audio::FrameCount RingBufferedStreamer::getRingBufferFrameCapacity() const
{
	return ringCapacityFrames;
}

}