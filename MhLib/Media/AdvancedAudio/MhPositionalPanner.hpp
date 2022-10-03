#ifndef POSITIONALPANNER_HPP
#define POSITIONALPANNER_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAmbisonicPanner.hpp>
#include <glm/glm.hpp>
namespace MH33 {
namespace Audio {

class MH_ADV_AUDIO_API PositionalPanner : public Playable
{
public:
	typedef std::shared_ptr<glm::vec3> sPosition;
private:
	AmbisonicPanner panner;
	sPosition soundOrigin;
	sPosition listenerPosition;
	float attenuation;
public:
	PositionalPanner(FrameCount bufferSize);
	const AmbisonicPanner& getPanner() const;
	AmbisonicPanner& getPanner();

	FrameCount outputTo(const Output& dst);
	FrameRate getFrameRate() const;
	ChannelCount getChannelCount() const;
	sPlayable getPlayable() const;
	void setPlayable(const sPlayable &value);
	float getAttenuation() const;
	void setAttenuation(float value);
	sPosition getSoundOrigin() const;
	void setSoundOrigin(const sPosition &value);
	sPosition getListenerPosition() const;
	void setListenerPosition(const sPosition &value);
};

}
}
#endif // POSITIONALPANNER_HPP
