#include <MhLib/Media/AdvancedAudio/FX/SampleLevelEffect.hpp>
namespace MH33 {
namespace Audio {
namespace  FX {

void SampleLevelEffect::doEffect(const ModificationStream &modstream)
{
	SampleCount procSamples = framesToSamples(modstream.frameCount,modstream.channels);
	for(const auto it : procSamples) {
		modstream.dst[it] = doSampleLevelEffect(modstream.src[it]);
	}
}

bool SampleLevelEffect::requiresDeinterlacing()
{
	return false;
}

}
}
}
