#include "HrkSoundBuffer.hpp"

namespace Hrk {

SoundBufferHandle makeSoundBuffer(const float* frames, FrameCount frameCount,
  ChannelCount channels, SampleRate sampleRate,
  InterleavingType interleaving, std::pmr::memory_resource* memRes)
{
	if (frames == nullptr || frameCount.var == 0 || channels.var == 0) {
		return SoundBufferHandle();
	}
	SoundBufferMetadata metadata;
	metadata.frameCount = frameCount;
	metadata.sampleRate = sampleRate;
	metadata.channels = channels;
	metadata.interleaving = interleaving;
	const std::size_t samples = std::size_t(frameCount.var) * std::size_t(channels.var);
	SoundBufferHandle buffer = SoundBufferHandle::allocate_uninitialised(
memRes, samples, metadata);
	if (!buffer.valid()) {
		return buffer;
	}
	// The slab is deliberately *uninitialised*: float is trivially copyable, so one
	// memcpy is both correct and cheaper than constructing element by element.
	std::memcpy(buffer.data(), frames, samples * sizeof(float));
	return buffer;
}

} // namespace Hrk
