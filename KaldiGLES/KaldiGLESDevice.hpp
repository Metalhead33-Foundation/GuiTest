#ifndef KALDIGLESDEVICE_H
#define KALDIGLESDEVICE_H
/**
 * @file KaldiGLESDevice.hpp
 * @brief Declares the KaldiGLESDevice API in the KaldiGLES module.
 *
 * This header is part of the public declaration surface for KaldiGLES.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"

namespace GLES {
/** @brief OpenGL ES implementation of the Kaldi rendering device contract. */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates an empty OpenGL ES device wrapper. */
	Device();
};
}
#endif // KALDIGLESDEVICE_H
