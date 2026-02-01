#ifndef EUPHAUDIOITERATOR_HPP
#define EUPHAUDIOITERATOR_HPP
#include <Elvavena/Util/ElvIntegralIterator.hpp>
#include <cstdint>
#include <cmath>
#include <type_traits>
namespace Euph {
namespace Media {
namespace Audio {

/**
 * @typedef ChannelIndex
 * @brief Iterator type for channel indices (0-based)
 * @details Represents an iterator over audio channel indices
 * @see IntegralIterator
 */
typedef Elv::Util::IntegralIterator<uint_fast8_t> ChannelIndex;

/**
 * @typedef ChannelCount
 * @brief Iterable type representing the number of audio channels
 * @see IntegralIterable
 */
typedef Elv::Util::IntegralIterable<uint_fast8_t> ChannelCount;

/**
 * @typedef SampleIndex
 * @brief Iterator type for sample indices (0-based)
 * @details Represents an iterator over audio sample indices
 * @see IntegralIterator
 */
typedef Elv::Util::IntegralIterator<uintptr_t> SampleIndex;

/**
 * @typedef FrameIndex
 * @brief Iterator type for frame indices (0-based)
 * @details Represents an iterator over audio frame indices
 * @see IntegralIterator
 */
typedef Elv::Util::IntegralIterator<uintptr_t> FrameIndex;

/**
 * @typedef SampleCount
 * @brief Iterable type representing a count of audio samples
 * @see IntegralIterable
 */
typedef Elv::Util::IntegralIterable<uint_fast32_t> SampleCount;

/**
 * @typedef FrameRate
 * @brief Iterable type representing frames per second (sample rate)
 * @see IntegralIterable
 */
typedef Elv::Util::IntegralIterable<uint_fast32_t> FrameRate;

/**
 * @typedef FrameCount
 * @brief Iterable type representing a count of audio frames
 * @see IntegralIterable
 */
typedef Elv::Util::IntegralIterable<uint_fast32_t> FrameCount;

/**
 * @brief Convert sample count to byte count for float samples
 * @param samples Number of samples to convert
 * @return Number of bytes required to store the samples
 * @note Assumes each sample is stored as a float (4 bytes)
 */
inline uintptr_t samplesToBytes(const SampleCount& samples) { return samples.var * sizeof(float); }

/**
 * @brief Convert frame count to sample count
 * @param frames Number of frames to convert
 * @param channels Number of channels in the audio
 * @return Total number of samples (frames × channels)
 */
inline SampleCount framesToSamples(const FrameCount& frames, const ChannelCount& channels) {
	return SampleCount(frames.var * channels.var);
}

/**
 * @brief Convert sample count to frame count
 * @param samples Number of samples to convert
 * @param channels Number of channels in the audio
 * @return Number of frames (samples ÷ channels)
 * @note Uses integer division (fractional frames are truncated)
 */
inline FrameCount framesFromSamples(const SampleCount& samples, const ChannelCount& channels)  {
	return FrameCount(samples.var / uintptr_t(channels.var));
}

/**
 * @brief Convert frame count to time in seconds
 * @tparam SecondType Floating-point type to use for seconds (default: double)
 * @param frames Number of frames to convert
 * @param framerate Frame rate in frames per second
 * @return Time duration in seconds
 * @pre SecondType must be a floating-point type
 */
template<typename SecondType = double>
inline SecondType framesToSeconds(const FrameCount& frames, const FrameRate& framerate) {
	static_assert (std::is_floating_point<SecondType>(),"Must use floating point types when calculating seconds!");
	return SecondType(frames.var) / SecondType(framerate.var);
}

/**
 * @brief Convert time in seconds to frame count
 * @tparam SecondType Floating-point type of the input seconds (default: double)
 * @param seconds Time duration to convert
 * @param framerate Frame rate in frames per second
 * @return Number of frames (rounded up to nearest integer)
 * @pre SecondType must be a floating-point type
 * @note Uses ceiling to ensure all time is accounted for
 */
template<typename SecondType = double>
inline FrameCount framesFromSeconds(SecondType seconds, const FrameRate& framerate) {
	static_assert (std::is_floating_point<SecondType>(),"Must use floating point types when calculating seconds!");
	return FrameCount(uintptr_t(std::ceil(seconds * SecondType(framerate.var)) ) );
}

}
}
}
#endif // EUPHAUDIOITERATOR_HPP
