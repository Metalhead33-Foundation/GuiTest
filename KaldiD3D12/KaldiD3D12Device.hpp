#ifndef KALDID3D12DEVICE_HPP
#define KALDID3D12DEVICE_HPP
/**
 * @file KaldiD3D12Device.hpp
 * @brief Declares the KaldiD3D12Device API in the KaldiD3D12 module.
 *
 * This header is part of the public declaration surface for KaldiD3D12.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"

namespace D3D12 {
/** @brief Direct3D 12 implementation of the Kaldi rendering device contract. */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates an empty Direct3D 12 device wrapper. */
	Device();
};
}
#endif // KALDID3D12DEVICE_HPP
