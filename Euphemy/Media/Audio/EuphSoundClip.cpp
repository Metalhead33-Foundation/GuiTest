#include "EuphSoundClip.hpp"
#include <Euphemy/Media/Audio/EuphSoundFile.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace Euph {
namespace Media {
namespace Audio {

SoundClip::SoundClip() = default;

SoundClip::SoundClip(std::vector<float> newSamples, ChannelCount newChannels, SampleRate newRate)
	: samples(std::move(newSamples)), channelCount(newChannels), rate(newRate)
{
}

FrameCount SoundClip::frameCount() const noexcept
{
	if (channelCount.var == 0) {
		return FrameCount(0);
	}
	return FrameCount(static_cast<uint_fast32_t>(samples.size() / channelCount.var));
}

SampleCount SoundClip::sampleCount() const noexcept
{
	return SampleCount(static_cast<uint_fast32_t>(samples.size()));
}

double SoundClip::duration() const noexcept
{
	if (rate.var == 0) {
		return 0.0;
	}
	return double(frameCount().var) / double(rate.var);
}

SoundClip SoundClip::load(Elv::Io::uDevice device)
{
	if (!device || !device->isValid()) {
		throw std::runtime_error("SoundClip: the device is not readable");
	}
	SoundFile file(std::move(device));
	const FrameCount frames = file.getFrameNum();
	const ChannelCount channels = file.getChannels();
	const SampleRate sampleRate = file.getFrameRate();
	if (frames.var == 0 || channels.var == 0) {
		throw std::runtime_error("SoundClip: the stream holds no audio frames");
	}
	const std::size_t sampleCountValue =
		std::size_t(frames.var) * std::size_t(channels.var);
	std::vector<float> buffer(sampleCountValue);
	// libsndfile converts whatever the file holds (8-bit WAV included) into float in
	// one call; a short read is normal for a malformed tail and simply yields silence.
	const FrameCount read = file.readf(buffer.data(), frames);
	if (read.var == 0) {
		throw std::runtime_error("SoundClip: no samples could be decoded");
	}
	buffer.resize(std::size_t(read.var) * std::size_t(channels.var));
	return SoundClip(std::move(buffer), channels, sampleRate);
}

bool SoundClip::mapChannels(ChannelCount target)
{
	if (!valid() || target.var == 0 || target.var == channelCount.var) {
		return false;
	}
	const std::size_t frames = std::size_t(frameCount().var);
	const std::size_t sourceChannels = std::size_t(channelCount.var);
	const std::size_t targetChannels = std::size_t(target.var);
	std::vector<float> mapped(frames * targetChannels);
	for (std::size_t frame = 0; frame < frames; ++frame) {
		const float* source = samples.data() + frame * sourceChannels;
		float* destination = mapped.data() + frame * targetChannels;
		if (sourceChannels == 1) {
			// Mono becomes every channel: the same click, audible on both sides.
			std::fill(destination, destination + targetChannels, source[0]);
			continue;
		}
		if (targetChannels == 1) {
			float sum = 0.0f;
			for (std::size_t channel = 0; channel < sourceChannels; ++channel) {
				sum += source[channel];
			}
			destination[0] = sum / float(sourceChannels);
			continue;
		}
		for (std::size_t channel = 0; channel < targetChannels; ++channel) {
			destination[channel] = source[channel % sourceChannels];
		}
	}
	samples = std::move(mapped);
	channelCount = target;
	return true;
}

bool SoundClip::resample(SampleRate target, ResampleType type)
{
	if (!valid() || target.var == 0 || target.var == rate.var) {
		return false;
	}
	const std::size_t channels = std::size_t(channelCount.var);
	const std::size_t inputFrames = std::size_t(frameCount().var);
	const double ratio = double(target.var) / double(rate.var);
	// libsamplerate can emit a little more than the ratio suggests once the filter
	// ramp is accounted for; the capacity grows on demand below, this is a guess.
	std::size_t capacity = std::size_t(std::ceil(double(inputFrames) * ratio)) + 64;
	std::vector<float> converted(capacity * channels);
	FullResampler resampler(channelCount, type);
	std::size_t consumed = 0;
	std::size_t produced = 0;
	while (consumed < inputFrames) {
		if (produced >= capacity) {
			capacity = capacity * 2 + 64;
			converted.resize(capacity * channels);
		}
		resampler.set(samples.data() + consumed * channels,
					  converted.data() + produced * channels,
					  FrameCount(static_cast<uint_fast32_t>(inputFrames - consumed)),
					  FrameCount(static_cast<uint_fast32_t>(capacity - produced)), ratio);
		resampler.setEndOfInput(true);
		const int status = resampler.process();
		if (status != 0) {
			throw std::runtime_error("SoundClip: the resampler failed with code " +
									 std::to_string(status));
		}
		const std::size_t used = std::size_t(resampler.getInputFramesUsed().var);
		const std::size_t emitted = std::size_t(resampler.getOutputFramesGenerated().var);
		consumed += used;
		produced += emitted;
		if (used == 0 && emitted == 0) {
			// Nothing moved and nothing came out: another round cannot make progress.
			break;
		}
	}
	converted.resize(produced * channels);
	samples = std::move(converted);
	rate = target;
	return true;
}

bool SoundClip::conformTo(ChannelCount targetChannels, SampleRate targetRate)
{
	// Channels first: resampling a stereo clip and then folding it to mono would pay
	// for the conversion twice and could fold dither noise into the result.
	const bool remapped = mapChannels(targetChannels);
	const bool resampled = resample(targetRate);
	return remapped || resampled;
}

} // namespace Audio
} // namespace Media
} // namespace Euph
