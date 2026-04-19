#ifndef HRKAUDIOPACKET_HPP
#define HRKAUDIOPACKET_HPP
#include <Euphemy/Media/Audio/EuphAudioIterator.hpp>
namespace Hrk {

using Euph::Media::Audio::ChannelCount;
using Euph::Media::Audio::FrameCount;
using Euph::Media::Audio::FrameIndex;
using Euph::Media::Audio::SampleCount;
using Euph::Media::Audio::SampleRate;

enum class PlayStatus : uint8_t {
	STOPPED,
	PLAYING,
	PAUSED
};

enum class InterleavingType : uint8_t {
	DONT_CARE = 0,
	INTERLEAVED,
	SEPARATE_CHANNELS
};

/**
 * @brief Sentinel sample rate value meaning "unspecified / don't care".
 *
 * This value is primarily used with @ref Output::samplerate in `Playable::outputTo(Output&)`.
 * When the caller sets `samplerate` to `SAMPLE_RATE_DONT_CARE`, the callee may choose its own
 * native sample rate, and should write the actual chosen rate back to the same field when known.
 *
 * Contract summary for output packets:
 * - `output.samplerate != SAMPLE_RATE_DONT_CARE`:
 *   callee should try to honor this exact rate.
 * - `output.samplerate == SAMPLE_RATE_DONT_CARE`:
 *   callee is free to choose and should report the effective rate by mutating `output.samplerate`.
 *
 * This enables graph nodes to negotiate sample rate without requiring a separate query API.
 */
inline const SampleRate SAMPLE_RATE_DONT_CARE = SampleRate(0);

/**
 * @brief Read-only audio packet for ingest/input operations.
 */
struct Input {
	const float* src;
	FrameCount frameCount;
	/**
	 * @brief Sample rate of `src` in Hz.
	 *
	 * `SAMPLE_RATE_DONT_CARE` should generally not be used for input streams, because
	 * consumers cannot infer a reliable source rate from immutable input metadata.
	 */
	SampleRate samplerate;
	ChannelCount channels;
	InterleavingType interleaving;
};

/**
 * @brief Mutable audio packet for renderer/playable output.
 */
struct Output {
	float* dst;
	FrameCount frameCount;
	/**
	 * @brief Requested or resolved sample rate in Hz.
	 *
	 * The caller may pass `SAMPLE_RATE_DONT_CARE` to indicate that any compatible
	 * sample rate is acceptable. In that case, `Playable::outputTo(Output&)` implementations
	 * should write back the actual effective sample rate once known.
	 *
	 * If this field is non-zero, implementations should interpret it as a concrete request
	 * and either honor it or fail explicitly (for example with a mismatch error).
	 */
	SampleRate samplerate;
	ChannelCount channels;
	InterleavingType interleaving;
};

/**
 * @brief Read/write packet for in-place or transform-style processing stages.
 */
struct ModificationStream {
	const float* src;
	float* dst;
	FrameCount frameCount;
	/**
	 * @brief Stream sample rate in Hz.
	 *
	 * For transform nodes that can negotiate or select a rate dynamically, this field follows
	 * the same zero-sentinel semantics as @ref Output::samplerate.
	 */
	SampleRate samplerate;
	ChannelCount channels;
	InterleavingType interleaving;
};

inline SampleCount framesToSamples(const FrameCount& frames, const ChannelCount& channelCount) {
	return Euph::Media::Audio::framesToSamples(frames, channelCount);
}

inline uintptr_t samplesToBytes(const SampleCount& samples) {
	return Euph::Media::Audio::samplesToBytes(samples);
}

};
#endif // HRKAUDIOPACKET_HPP
