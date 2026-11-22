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
#include <span>

namespace D3D11 {
/**
 * @brief Direct3D 11 implementation of the Kaldi rendering device contract.
 *
 * @warning **PROVISIONAL / NON-FUNCTIONAL.** This class owns no `ID3D11Device`,
 * so `capabilities()` reports `Kld::halCapabilitiesNone()` and
 * `processCommandQueue()` answers `HalStatusCode::UnsupportedBaseline`. The
 * intended structure is documented in `KaldiD3D11Device.cpp`. The D3D11
 * translation units are compiled only inside the `win32` scope of the `.pro`, so
 * none of this can be validated on the Linux host.
 */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates an empty Direct3D 11 device wrapper. */
	Device();

	/** @brief No usable capabilities are probed until the device owns a context. */
	const Kld::HalCapabilities& capabilities() const noexcept override;

	/** @brief Provisional: always reports `UnsupportedBaseline`. */
	Kld::HalStatus processCommandQueue(std::span<const Kld::GfxOp> commands) override;

private:
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
};
}

#endif // KALDID3D11DEVICE_HPP
