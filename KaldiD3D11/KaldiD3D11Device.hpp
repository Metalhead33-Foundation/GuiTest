#ifndef KALDID3D11DEVICE_HPP
#define KALDID3D11DEVICE_HPP
/**
 * @file KaldiD3D11Device.hpp
 * @brief Declares the KaldiD3D11Device API in the KaldiD3D11 module.
 *
 * This header is part of the public declaration surface for KaldiD3D11.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"

namespace D3D11 {
/** @brief Direct3D 11 implementation of the Kaldi rendering device contract. */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates an empty Direct3D 11 device wrapper. */
	Device();
};
}

#endif // KALDID3D11DEVICE_HPP
