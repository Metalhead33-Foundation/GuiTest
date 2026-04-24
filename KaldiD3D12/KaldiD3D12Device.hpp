#ifndef KALDID3D12DEVICE_HPP
#define KALDID3D12DEVICE_HPP
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"

namespace D3D12 {
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	Device();
};
}
#endif // KALDID3D12DEVICE_HPP
