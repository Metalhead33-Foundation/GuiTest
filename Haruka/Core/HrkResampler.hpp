#ifndef HRKRESAMPLER_HPP
#define HRKRESAMPLER_HPP
#include <Haruka/Core/HrkProxyPlayable.hpp>
#include <Euphemy/Media/Audio/EuphResampler.hpp>
#include <vector>
#include <memory_resource>
#include <span>
namespace Hrk {

typedef Euph::Media::Audio::ResampleType ResampleType;

class MH_HARUKA_API Resampler : public ProxyPlayable
{
public:
	const ChannelCount channels;
	const ResampleType resampleType;
	const FrameCount bufferSize;
private:
	float speed;
	SampleRate inSampleRate = SAMPLE_RATE_DONT_CARE;
	FrameCount bufferedInputFrames = FrameCount(0);
	Euph::Media::Audio::FullResampler resampler;
	std::pmr::vector<float> buffer; // Size: (inChannels*outChannels)+(inChannels*frameCount)? Why? Because start contains the matrix. Starting offset of the scratch memory is at (inChannels*outChannels).
public:
	Resampler(FrameCount bufferSize, ChannelCount channels, float speed = 1.0f, ResampleType resampleType = ResampleType::SINC_FASTEST, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	FrameCount outputTo(Output& output) override;
};

}
#endif // HRKRESAMPLER_HPP
