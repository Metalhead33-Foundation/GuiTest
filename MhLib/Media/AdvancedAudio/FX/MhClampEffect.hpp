#ifndef CLAMPEFFECT_HPP
#define CLAMPEFFECT_HPP
#include <MhLib/Media/AdvancedAudio/FX/SampleLevelEffect.hpp>

namespace MH33 {
namespace Audio {
namespace  FX {

class MH_ADV_AUDIO_API ClampEffect : public SampleLevelEffect
{
private:
	float maximumLevel;
protected:
	float doSampleLevelEffect(float input) const;
public:
	ClampEffect(float maximumLevel=0.0f);
	float getMaximumLevel() const;
	void setMaximumLevel(float value);
};

}
}
}
#endif // CLAMPEFFECT_HPP
