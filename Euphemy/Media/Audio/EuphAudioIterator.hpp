#ifndef EUPHAUDIOITERATOR_HPP
#define EUPHAUDIOITERATOR_HPP
#include <Elvavena/Util/ElvIntegralIterator.hpp>
#include <cstdint>
#include <cmath>
#include <type_traits>
namespace Euph {
namespace Media {
namespace Audio {

typedef Elv::Util::IntegralIterator<uint_fast8_t> ChannelIndex;
typedef Elv::Util::IntegralIterable<uint_fast8_t> ChannelCount;
typedef Elv::Util::IntegralIterator<uintptr_t> SampleIndex;
typedef Elv::Util::IntegralIterator<uintptr_t> FrameIndex;
typedef Elv::Util::IntegralIterable<uint_fast32_t> SampleCount;
typedef Elv::Util::IntegralIterable<uint_fast32_t> FrameRate;
typedef Elv::Util::IntegralIterable<uint_fast32_t> FrameCount;

inline uintptr_t samplesToBytes(const SampleCount& samples) { return samples.var * sizeof(float); }
inline SampleCount framesToSamples(const FrameCount& frames, const ChannelCount& channels) {
	return SampleCount(frames.var * channels.var);
}
inline FrameCount framesFromSamples(const SampleCount& samples, const ChannelCount& channels)  {
	return FrameCount(samples.var / uintptr_t(channels.var));
}
template<typename SecondType = double> inline SecondType framesToSeconds(const FrameCount& frames, const FrameRate& framerate) {
	static_assert (std::is_floating_point<SecondType>(),"Must use floating point types when calculating seconds!");
	return SecondType(frames.var) / SecondType(framerate.var);
}
template<typename SecondType = double> inline FrameCount framesFromSeconds(SecondType seconds, const FrameRate& framerate) {
	static_assert (std::is_floating_point<SecondType>(),"Must use floating point types when calculating seconds!");
	return FrameCount(uintptr_t(std::ceil(seconds * SecondType(framerate.var)) ) );
}

}
}
}
#endif // EUPHAUDIOITERATOR_HPP
