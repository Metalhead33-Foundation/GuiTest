#ifndef KALDIGLESDEVICE_H
#define KALDIGLESDEVICE_H
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"

namespace GLES {
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	Device();
};
}
#endif // KALDIGLESDEVICE_H
