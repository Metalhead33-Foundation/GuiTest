#include <MhLib/Media/AdvancedAudio/MhPositionalPanner.hpp>

namespace MH33 {
namespace Audio {

float PositionalPanner::getAttenuation() const
{
	return attenuation;
}

void PositionalPanner::setAttenuation(float value)
{
	attenuation = value;
}

PositionalPanner::sPosition PositionalPanner::getSoundOrigin() const
{
	return soundOrigin;
}

void PositionalPanner::setSoundOrigin(const sPosition &value)
{
	soundOrigin = value;
}

PositionalPanner::sPosition PositionalPanner::getListenerPosition() const
{
	return listenerPosition;
}

void PositionalPanner::setListenerPosition(const sPosition &value)
{
	listenerPosition = value;
}

PositionalPanner::PositionalPanner(FrameCount bufferSize) : panner(bufferSize), soundOrigin(nullptr), listenerPosition(nullptr), attenuation(0.0f)
{

}

const AmbisonicPanner &PositionalPanner::getPanner() const
{
	return panner;
}

AmbisonicPanner &PositionalPanner::getPanner()
{
	return panner;
}

FrameCount PositionalPanner::outputTo(const Output &dst)
{
	if(soundOrigin && listenerPosition) {
		const glm::vec3 diff = *listenerPosition - *soundOrigin;
		// const glm::vec2 horizontal(diff.x,diff.y);
		// const glm::vec2 elevation(diff.y,diff.z);
		panner.setAngle(atan2f(-1.0f*diff.x,-1.0f*diff.z),
					  atan2f(diff.y,sqrtf(diff.x*diff.x + diff.z*diff.z))
					  );
		panner.setDistance(glm::distance(*soundOrigin,*listenerPosition)*attenuation);
		return panner.outputTo(dst);
	}
	else return FrameCount(0);
}

FrameRate PositionalPanner::getFrameRate() const
{
	return panner.getFrameRate();
}

ChannelCount PositionalPanner::getChannelCount() const
{
	return panner.getChannelCount();
}

sPlayable PositionalPanner::getPlayable() const
{
	return panner.getPlayable();
}

void PositionalPanner::setPlayable(const sPlayable &value)
{
	panner.setPlayable(value);
}

}
}
