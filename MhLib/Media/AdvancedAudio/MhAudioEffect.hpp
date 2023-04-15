#ifndef MHAUDIOEFFECT_HPP
#define MHAUDIOEFFECT_HPP
#include <MhLib/Media/AdvancedAudio/MhAudioPacket.hpp>
#include <memory>
namespace MH33 {
namespace Audio {

DEFINE_CLASS(Effect)
class Effect {
public:
	virtual ~Effect() = default;
	virtual void doEffect(const ModificationStream& modstream) = 0;
	virtual bool requiresDeinterlacing() = 0;
};

}
}
#endif // MHAUDIOEFFECT_HPP
