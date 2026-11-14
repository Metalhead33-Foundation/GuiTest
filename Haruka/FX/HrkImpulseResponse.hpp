#ifndef HRKIMPULSERESPONSE_HPP
#define HRKIMPULSERESPONSE_HPP
/**
 * @file HrkImpulseResponse.hpp
 * @brief Owns a multi-channel impulse response (IR) used by convolution effects.
 *
 * This is the modern replacement for `fftconvolver::IrBuffer` /
 * `fftconvolver::sIrBuffer` from the `NewBeginning` branch, which the old
 * `MhConvolver` borrowed from the vendored FFTConvolver sources. Euphemy now
 * vendors FFTConvolver as a low-level primitive that only knows
 * `(samples, length)` pairs, so the "IR with a sample rate and channel layout"
 * concept lives here instead, in Haruka's own vocabulary.
 *
 * The sample storage is channel-planar (`channelCount` consecutive planes of
 * `frameCount` samples each), because that is the layout FFTConvolver consumes
 * and the layout @ref Hrk::FX::Convolver records in.
 *
 * The type is header-only and immutable-by-convention once published: impulse
 * responses are shared between effects through `std::shared_ptr`, and mutating
 * one after it has been handed to a convolver is a data race.
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <span>
#include <utility>

namespace Hrk {

/**
 * @brief Channel-planar impulse response with an explicit sample rate.
 */
class ImpulseResponse {
private:
	ChannelCount channelCount;
	FrameCount frameCount;
	SampleRate sampleRate;
	std::pmr::vector<float> samples;
public:
	/**
	 * @brief Creates a mono impulse response.
	 * @param frameCount Number of samples in the response.
	 * @param sampleRate Sample rate the response was generated for.
	 * @param memRes Memory resource backing the sample storage.
	 */
	explicit ImpulseResponse(FrameCount frameCount, SampleRate sampleRate, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: channelCount(1), frameCount(frameCount), sampleRate(sampleRate),
		  samples(framesToSamples(frameCount, ChannelCount(1)).var, memRes) {}

	/**
	 * @brief Creates a multi-channel impulse response.
	 * @param channelCount Number of channel planes.
	 * @param frameCount Number of samples per channel plane.
	 * @param sampleRate Sample rate the response was generated for.
	 * @param memRes Memory resource backing the sample storage.
	 */
	ImpulseResponse(ChannelCount channelCount, FrameCount frameCount, SampleRate sampleRate, std::pmr::memory_resource* memRes = std::pmr::get_default_resource())
		: channelCount(channelCount), frameCount(frameCount), sampleRate(sampleRate),
		  samples(framesToSamples(frameCount, channelCount).var, memRes) {}

	/** @brief Returns the number of channel planes. */
	ChannelCount getChannelCount() const { return channelCount; }
	/** @brief Returns the number of samples per channel plane. */
	FrameCount getFrameCount() const { return frameCount; }
	/** @brief Returns the sample rate the response was generated for. */
	SampleRate getSampleRate() const { return sampleRate; }

	/** @brief Returns the mutable plane of channel @a channel. */
	std::span<float> getChannel(ChannelIndex channel)
	{
		return std::span<float>(samples.data() + uintptr_t(channel.var) * uintptr_t(frameCount.var), uintptr_t(frameCount.var));
	}
	/** @brief Returns the immutable plane of channel @a channel. */
	std::span<const float> getChannel(ChannelIndex channel) const
	{
		return std::span<const float>(samples.data() + uintptr_t(channel.var) * uintptr_t(frameCount.var), uintptr_t(frameCount.var));
	}

	/** @brief Returns the raw channel-planar sample storage. */
	float* data() { return samples.data(); }
	/** @brief Returns the raw channel-planar sample storage. */
	const float* data() const { return samples.data(); }

	/** @brief Returns the total number of stored samples across all channels. */
	SampleCount getSampleCount() const { return SampleCount(samples.size()); }

	/** @brief Zeroes every stored sample. */
	void clear() { std::fill(samples.begin(), samples.end(), 0.0f); }
};

/** @brief Mutable shared handle to an impulse response. */
typedef std::shared_ptr<ImpulseResponse> sImpulseResponse;
/** @brief Immutable shared handle to an impulse response. */
typedef std::shared_ptr<const ImpulseResponse> scImpulseResponse;

}

#endif // HRKIMPULSERESPONSE_HPP
