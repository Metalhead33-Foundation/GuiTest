#ifndef HRKRESAMPLER_HPP
#define HRKRESAMPLER_HPP
/**
 * @file HrkResampler.hpp
 * @brief Declares the HrkResampler API in the Haruka/Core module.
 *
 * This header is part of the public declaration surface for Haruka/Core.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/Core/HrkProxyPlayable.hpp>
#include <Euphemy/Media/Audio/EuphResampler.hpp>
#include <vector>
#include <memory_resource>
#include <span>
namespace Hrk {

/** @brief Resampling quality and algorithm selector re-exported from Euphemy. */
typedef Euph::Media::Audio::ResampleType ResampleType;

/** @brief Proxy playable that resamples its source into the requested output rate. */
class MH_HARUKA_API Resampler : public ProxyPlayable
{
public:
	/** @brief Number of channels expected in the stream. */
	const ChannelCount channels;
	/** @brief Resampling algorithm used internally. */
	const ResampleType resampleType;
	/** @brief Internal processing buffer capacity in frames. */
	const FrameCount bufferSize;
private:
	float speed;
	SampleRate inSampleRate = SAMPLE_RATE_DONT_CARE;
	FrameCount bufferedInputFrames = FrameCount(0);
	Euph::Media::Audio::FullResampler resampler;
	std::pmr::vector<float> buffer; // Size: (inChannels*outChannels)+(inChannels*frameCount)? Why? Because start contains the matrix. Starting offset of the scratch memory is at (inChannels*outChannels).
public:
	/** @brief Creates a resampler with fixed channel count, speed, and quality mode. */
	Resampler(FrameCount bufferSize, ChannelCount channels, float speed = 1.0f, ResampleType resampleType = ResampleType::SINC_FASTEST, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Resamples the proxied playable into `output`. */
	FrameCount outputTo(Output& output) override;
};

}
#endif // HRKRESAMPLER_HPP
