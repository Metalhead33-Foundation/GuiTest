#include "HrkSoundSource.hpp"
#include <Haruka/Core/HrkAudioError.hpp>
#include <cstring>
namespace Hrk {
const SoundBufferHandle& SoundSource::getBuffer() const
{
	return buffer;
}

void SoundSource::setBuffer(const SoundBufferHandle& newBuffer)
{
	buffer = newBuffer;
}

void SoundSource::setBuffer(SoundBufferHandle&& newBuffer)
{
	buffer = std::move(newBuffer);
}

Euph::Media::Audio::FrameCount SoundSource::outputTo(Output& output)
{
	if(state.getPlayStatus() != PlayStatus::PLAYING) return FrameCount(0);
	if(!buffer.valid()) return FrameCount(0);
	const auto& metadata = buffer.meta();
	if(!output.frameCount.var) return FrameCount(0);
	if(!metadata.frameCount.var) return FrameCount(0);
	// Early error checking
	if(output.channels != metadata.channels) throw ChannelCountMismatchError(output.channels,metadata.channels);
	if(output.samplerate == SAMPLE_RATE_DONT_CARE) {
		output.samplerate = metadata.sampleRate;
	} else if(output.samplerate != metadata.sampleRate) {
		throw SamplerateMismatchError(output.samplerate,metadata.sampleRate);
	}
	if(metadata.channels.var > 1 && output.interleaving != metadata.interleaving) throw InterleavingMismatchError(output.interleaving,metadata.interleaving);
	FrameCount processed(0);
	FrameCount remaining = output.frameCount;
	do {
		if(state.getCursor() >= metadata.frameCount) {
			if(state.getRepeat()) {
				state.setCursor(FrameIndex(0));
			} else {
				state.setPlayStatus(PlayStatus::STOPPED);
				break;
			}
		}
		const FrameCount toProcess = std::min(remaining, metadata.frameCount - state.getCursor());
		if(!toProcess.var) break;
		const SampleCount dstOffset = Hrk::framesToSamples(processed, output.channels);
		// Copy contiguous frames from the in-memory buffer.
		memcpy(&output.dst[dstOffset.var], &buffer.data()[state.getCursor().var * metadata.channels.var],
			   toProcess.var * metadata.channels.var * sizeof(float));
		state.setCursor(state.getCursor() + toProcess);
		processed += toProcess.var;
		remaining -= toProcess.var;
	} while(remaining.var);
	return processed;
}

const PlaybackState& SoundSource::getState() const
{
	return state;
}

PlaybackState& SoundSource::getState()
{
	return state;
}

SoundSource::SoundSource() {}
}
