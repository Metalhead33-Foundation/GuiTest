#ifndef MHAUDIOPLAYABLE_HPP
#define MHAUDIOPLAYABLE_HPP
#include <MhLib/Media/AdvancedAudio/MhAudioPacket.hpp>
#include <memory>
namespace MH33 {
namespace Audio {

DEFINE_CLASS(Playable)
class Playable {
public:
	virtual ~Playable() = default;
	virtual FrameCount outputTo(const Output& dst) = 0;
	virtual FrameRate getFrameRate() const = 0;
	virtual ChannelCount getChannelCount() const = 0;
};

}
}
#endif // MHAUDIOPLAYABLE_HPP
