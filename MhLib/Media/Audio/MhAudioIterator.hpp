#ifndef MHAUDIOITERATOR_HPP
#define MHAUDIOITERATOR_HPP
#include <MhLib/Util/MhIntegralIterator.hpp>
#include <cstdint>
#include <cmath>
#include <type_traits>
namespace MH33 {
namespace Audio {

typedef Util::IntegralIterator<uint_fast8_t> ChannelIndex;
typedef Util::IntegralIterable<uint_fast8_t> ChannelCount;
typedef Util::IntegralIterator<uintptr_t> SampleIndex;
typedef Util::IntegralIterator<uintptr_t> FrameIndex;
typedef Util::IntegralIterable<uint_fast32_t> SampleCount;
typedef Util::IntegralIterable<uint_fast32_t> FrameRate;
typedef Util::IntegralIterable<uint_fast32_t> FrameCount;

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

/*struct SampleCount : public Util::IntegralIterable<uint_fast32_t> {
	typedef Util::IntegralIterable<uint_fast32_t> BaseType;
	SampleCount(const BaseType& other) : BaseType(other) {

	}
	explicit SampleCount(uint_fast32_t num=0) : BaseType(num) {

	}
	inline uintptr_t toBytes() const { return var * sizeof(float); }
};
struct FrameRate : public Util::IntegralIterable<uint_fast32_t> {
	typedef Util::IntegralIterable<uint_fast32_t> BaseType;
	FrameRate(const BaseType& other) : BaseType(other) {

	}
	explicit FrameRate(uint_fast32_t num=0) : BaseType(num) {

	}
};
struct FrameCount : public Util::IntegralIterable<uint_fast32_t> {
	typedef Util::IntegralIterable<uint_fast32_t> BaseType;
	FrameCount(const BaseType& other) : BaseType(other) {

	}
	explicit FrameCount(uint_fast32_t num=0) : BaseType(num) {

	}
	static inline FrameCount fromSamples(const SampleCount& samples, const ChannelCount& channels) {
		return FrameCount(samples.var / uintptr_t(channels.var));
	}
	inline SampleCount toSamples(ChannelCount channels) const { return SampleCount(var * uintptr_t(channels.var) ); }
	inline uintptr_t toBytes(ChannelCount channels) const { return SampleCount(var * uintptr_t(channels.var) ).toBytes(); }
	template<typename SecondType = double> inline SecondType toSeconds(const FrameRate& framerate) const {
		static_assert (std::is_floating_point<SecondType>(),"Must use floating point types when calculating seconds!");
		return SecondType(var) / SecondType(framerate.var);
	}
	template<typename SecondType = double> static inline FrameCount fromSeconds(SecondType seconds, const FrameRate& framerate) {
		static_assert (std::is_floating_point<SecondType>(),"Must use floating point types when calculating seconds!");
		return FrameCount(uintptr_t(std::ceil(seconds * SecondType(framerate.var)) ) );
	}
};*/

}
}
#endif // MHAUDIOITERATOR_HPP
