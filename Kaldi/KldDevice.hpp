#ifndef KLDDEVICE_HPP
#define KLDDEVICE_HPP
#include <Kaldi/KaldiOperation.hpp>
#include <future>
#include <vector>
#include <mutex>

namespace Kld {

class Device {
protected:
	// Uh-oh... I intended Kaldi to be a header-only library.
	// Is this still kosher?
	std::vector<GfxOp> commandQueue;
	mutable std::mutex mutex;
public:
	virtual ~Device() = default;
	virtual void processCommandQeue() = 0;
	// So, uhhh... do we add a strictly non-virtual std::promise<uint64_t> createTexture(const OpCreateTexture2D& descriptor)?
	// That conflicts with the idea of a header-only library.
};

}
#endif // KLDDEVICE_HPP
