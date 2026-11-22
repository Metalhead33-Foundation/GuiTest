#ifndef KALDID3D11DEVICE_HPP
#define KALDID3D11DEVICE_HPP
/**
 * @file KaldiD3D11Device.hpp
 * @brief Direct3D 11 implementation of the Kaldi rendering device contract.
 *
 * @section kaldid3d11_device_ownership Device ownership
 *
 * The device owns a `D3D11::Context` - the `ID3D11Device`, its immediate context
 * and the swap chain - exactly like the GL backends own their context. That
 * context is only *definable* on Windows, so the header forward-declares it and
 * the implementation creates it inside `_WIN32`. On any other platform the
 * windowed constructor throws `std::runtime_error` and the device stays detached
 * (`hasContext() == false`, `capabilities()` reports nothing), which keeps the
 * class usable - and honest - from portable code.
 *
 * `capabilities()` is probed from the owned device through
 * `D3D11::probeCapabilities()`. `processCommandQueue()` is still a documented
 * `UnsupportedBaseline` stub: no command executor exists yet, so claiming
 * otherwise would be a lie.
 *
 * @note **Not validated.** `KaldiD3D11.pro` compiles the Direct3D translation
 * units inside a `win32` scope, so none of this can be built or run on this
 * repository's Linux host.
 */
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"
#include <SDL2/SDL_syswm.h>
#include <span>

namespace D3D11 {

/** @brief Owns the D3D11 device, immediate context and swap chain. Windows-only. */
class Context;

/**
 * @brief Direct3D 11 implementation of the Kaldi rendering device contract.
 */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates a detached device without a context. */
	Device();

	/**
	 * @brief Creates the D3D11 device and swap chain for @p window.
	 * @throws std::runtime_error On non-Windows platforms, where the backend
	 *         cannot exist at all.
	 * @throws std::invalid_argument When @p window has no Win32 handle.
	 * @throws std::runtime_error When device or swap-chain creation fails.
	 */
	explicit Device(const SDL_SysWMinfo& window);

	/** @brief Releases the context, swap chain and device. */
	~Device() override;

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;
	Device(Device&&) = delete;
	Device& operator=(Device&&) = delete;

	/** @brief The capabilities probed from the owned device, or none when detached. */
	const Kld::HalCapabilities& capabilities() const noexcept override;

	/** @brief Provisional: reports `UnsupportedBaseline` until an executor exists. */
	Kld::HalStatus processCommandQueue(std::span<const Kld::GfxOp> commands) override;

	/** @brief The owned context, or @c nullptr for a detached device. */
	Context* context() noexcept { return context_; }
	/** @brief The owned context, or @c nullptr for a detached device. */
	const Context* context() const noexcept { return context_; }
	/** @brief True when the device owns a D3D11 context. */
	bool hasContext() const noexcept { return context_ != nullptr; }

	/** @brief "Direct3D 11", or "none" when the device is detached. */
	const char* apiName() const noexcept;

private:
	// A raw pointer keeps this header compilable without <d3d11.h>; the
	// destructor deletes the context in the translation unit that defines it.
	Context* context_ = nullptr;
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
};

}

#endif // KALDID3D11DEVICE_HPP
