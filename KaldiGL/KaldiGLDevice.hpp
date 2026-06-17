#ifndef KALDIGLDEVICE_HPP
#define KALDIGLDEVICE_HPP
/**
 * @file KaldiGLDevice.hpp
 * @brief Declares the KaldiGLDevice API in the KaldiGL module.
 *
 * This header is part of the public declaration surface for KaldiGL.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"

namespace GL {
/** @brief OpenGL implementation of the Kaldi rendering device contract. */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates an empty OpenGL device wrapper. */
	Device();
};
}
#endif // KALDIGLDEVICE_HPP
