#ifndef HRKAUDIOPACKET_HPP
#define HRKAUDIOPACKET_HPP
/**
 * @file HrkAudioPacket.hpp
 * @brief Declares the HrkAudioPacket API in the Haruka/Core module.
 *
 * This header is part of the public declaration surface for Haruka/Core.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Euphemy/Media/Audio/EuphAudioIterator.hpp>
namespace Hrk {

/** @brief Audio channel count type re-exported from Euphemy. */
using Euph::Media::Audio::ChannelCount;
/** @brief Audio frame count type re-exported from Euphemy. */
using Euph::Media::Audio::FrameCount;
/** @brief Audio frame index type re-exported from Euphemy. */
using Euph::Media::Audio::FrameIndex;
/** @brief Audio sample count type re-exported from Euphemy. */
using Euph::Media::Audio::SampleCount;
/** @brief Audio sample rate type re-exported from Euphemy. */
using Euph::Media::Audio::SampleRate;

/** @brief Transport state for playback-oriented audio objects. */
enum class PlayStatus : uint8_t {
	STOPPED,
	PLAYING,
	PAUSED
};

/** @brief Memory layout of multi-channel audio samples. */
enum class InterleavingType : uint8_t {
	DONT_CARE = 0,
	INTERLEAVED,
	SEPARATE_CHANNELS
};

/**
 * @brief Sentinel sample rate value meaning "unspecified / don't care".
 *
 * This value is primarily used with `Output::samplerate` in `Playable::outputTo(Output&)`.
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
	/** @brief Source sample pointer. */
	const float* src;
	/** @brief Number of frames available at @ref src. */
	FrameCount frameCount;
	/**
	 * @brief Sample rate of `src` in Hz.
	 *
	 * `SAMPLE_RATE_DONT_CARE` should generally not be used for input streams, because
	 * consumers cannot infer a reliable source rate from immutable input metadata.
	 */
	SampleRate samplerate;
	/** @brief Number of channels in the input packet. */
	ChannelCount channels;
	/** @brief Channel interleaving layout of @ref src. */
	InterleavingType interleaving;
};

/**
 * @brief Mutable audio packet for renderer/playable output.
 */
struct Output {
	/** @brief Destination sample pointer. */
	float* dst;
	/** @brief Number of frames requested or available at @ref dst. */
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
	/** @brief Number of output channels. */
	ChannelCount channels;
	/** @brief Channel interleaving layout expected at @ref dst. */
	InterleavingType interleaving;
};

/**
 * @brief Read/write packet for in-place or transform-style processing stages.
 */
struct ModificationStream {
	/** @brief Source sample pointer. */
	const float* src;
	/** @brief Destination sample pointer. */
	float* dst;
	/** @brief Number of frames to transform. */
	FrameCount frameCount;
	/**
	 * @brief Stream sample rate in Hz.
	 *
	 * For transform nodes that can negotiate or select a rate dynamically, this field follows
	 * the same zero-sentinel semantics as `Output::samplerate`.
	 */
	SampleRate samplerate;
	/** @brief Number of channels in the stream. */
	ChannelCount channels;
	/** @brief Channel interleaving layout for source and destination buffers. */
	InterleavingType interleaving;
};

/** @brief Converts a frame count and channel count into a total sample count. */
inline SampleCount framesToSamples(const FrameCount& frames, const ChannelCount& channelCount) {
	return Euph::Media::Audio::framesToSamples(frames, channelCount);
}

/** @brief Converts a sample count into a byte count for float samples. */
inline uintptr_t samplesToBytes(const SampleCount& samples) {
	return Euph::Media::Audio::samplesToBytes(samples);
}

};
#endif // HRKAUDIOPACKET_HPP
