#ifndef SAMPLELEVELEFFECT_HPP
#define SAMPLELEVELEFFECT_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioEffect.hpp>
namespace MH33 {
namespace Audio {
namespace  FX {

class MH_ADV_AUDIO_API SampleLevelEffect : public Effect {
protected:
	virtual float doSampleLevelEffect(float input) const = 0;
public:
	virtual ~SampleLevelEffect() = default;
	void doEffect(const ModificationStream& modstream);
	bool requiresDeinterlacing();
};

}
}
}
#endif // SAMPLELEVELEFFECT_HPP
