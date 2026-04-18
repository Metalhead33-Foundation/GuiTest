#ifndef HRKPLAYABLE_HPP
#define HRKPLAYABLE_HPP
#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>
namespace Hrk {

class Playable {
public:
	virtual ~Playable() = default;
	virtual FrameCount outputTo(Output& output) = 0;
};

};
#endif // HRKPLAYABLE_HPP
