#ifndef FRAMERATECONVERTER_HPP
#define FRAMERATECONVERTER_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp>
#include <MhLib/Media/Audio/MhResampler.hpp>
#include <vector>

namespace MH33 {
namespace Audio {

class MH_ADV_AUDIO_API FramerateConverter : public Playable
{
public:
	const ChannelCount channels;
	const ResampleType resampleType;
	const FrameCount bufferSize;
private:
	sPlayable playable;
	CallbackResampler resampler;
	static long mh_sf_callback( void *self, float **data );
	std::vector<float> buffer;
	float speed;
public:
	FramerateConverter(FrameCount bufferSize, ChannelCount channels, float speed = 1.0f, ResampleType resampleType = ResampleType::SINC_FASTEST);
	const CallbackResampler& getResampler() const;
	CallbackResampler& getResampler();
	const sPlayable& getPlayable() const;
	void setPlayable(const sPlayable &value);
	FrameCount outputTo(const Output& dst);
	FrameRate getFrameRate() const;
	ChannelCount getChannelCount() const;
	float getSpeed() const;
	void setSpeed(float value);
};

}
}

#endif // FRAMERATECONVERTER_HPP
