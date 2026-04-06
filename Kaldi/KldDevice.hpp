#ifndef KLDDEVICE_HPP
#define KLDDEVICE_HPP
#include <Kaldi/KaldiLib.hpp>
#include <Kaldi/KaldiOperation.hpp>
#include <future>
#include <vector>
#include <mutex>

namespace Kld {

class MH_KALDI_API Device {
protected:
	std::vector<GfxOp> commandQueue;
	mutable std::mutex mutex;
	virtual void processCommandQeue() = 0;
public:
	virtual ~Device() = default;
	void flushCommandQueue();
	void pushCommand(const GfxOp& op);
};

}
#endif // KLDDEVICE_HPP
