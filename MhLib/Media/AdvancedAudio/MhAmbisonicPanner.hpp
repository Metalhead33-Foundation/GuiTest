#ifndef AMBISONICPANNER_HPP
#define AMBISONICPANNER_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp>
#include <vector>
namespace MH33 {
namespace Audio {

class MH_ADV_AUDIO_API AmbisonicPanner : public Playable
{
private:
	float horizontalAngle,elevationAngle,distance,sincos,coscos;
	std::vector<float> buffer;
	sPlayable playable;
	void syncCoefficients();
	FrameCount fillBuffer(FrameCount maxFrames);
public:
	AmbisonicPanner(FrameCount bufferSize);
	FrameCount outputTo(const Output& dst);
	FrameRate getFrameRate() const;
	ChannelCount getChannelCount() const;
	float getHorizontalAngle() const;
	void setHorizontalAngle(float value);
	float getElevationAngle() const;
	void setElevationAngle(float value);
	void setAngle(float horizontalAngle, float elevationAngle);
	sPlayable getPlayable() const;
	void setPlayable(const sPlayable &value);
	float getDistance() const;
	void setDistance(float value);
};

}
}
#endif // AMBISONICPANNER_HPP
