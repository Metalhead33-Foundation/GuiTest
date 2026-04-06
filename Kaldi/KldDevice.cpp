#include <Kaldi/KldDevice.hpp>

namespace Kld {

void Device::flushCommandQueue()
{
	std::lock_guard<std::mutex> lock_grd(mutex);
	processCommandQeue();
	commandQueue.clear();
}

void Device::pushCommand(const GfxOp& op)
{
	std::lock_guard<std::mutex> lock_grd(mutex);
	commandQueue.push_back(op);
}

}