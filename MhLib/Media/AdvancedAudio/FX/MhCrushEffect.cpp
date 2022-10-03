#include <MhLib/Media/AdvancedAudio/FX/MhCrushEffect.hpp>
namespace MH33 {
namespace Audio {
namespace  FX {


inline float pushToUnsigned(float input)
{
	return (input+2.0f)/2.0f;
}
inline float pushToSigned(float input)
{
	return (input*2.0f)-1.0f;
}

float CrushEffect::getMaxLevel() const
{
	return maxLevel;
}

void CrushEffect::setMaxLevel(float value)
{
	maxLevel = value;
}

float CrushEffect::doSampleLevelEffect(float input) const
{
	return pushToSigned(std::floor((pushToUnsigned(input)*maxLevel)+0.5f)/maxLevel);
}

CrushEffect::CrushEffect(float maxLevel) : maxLevel(maxLevel)
{

}

}
}
}
