#ifndef KLDDEVICE_HPP
#define KLDDEVICE_HPP
#include <Kaldi/KaldiOperation.hpp>
#include <span>

namespace Kld {

class Device {
public:
	virtual ~Device() = default;
	virtual void processCommandQeue(std::span<const GfxOp> commands) = 0;
};

}
#endif // KLDDEVICE_HPP
