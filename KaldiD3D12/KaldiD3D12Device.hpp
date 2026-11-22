#ifndef KALDID3D12DEVICE_HPP
#define KALDID3D12DEVICE_HPP
/**
 * @file KaldiD3D12Device.hpp
 * @brief Direct3D 12 implementation of the Kaldi rendering device contract.
 *
 * @section kaldid3d12_device_ownership Device ownership
 *
 * The device owns a `D3D12::Context` - the `ID3D12Device`, its command queue,
 * swap chain, descriptor heaps and per-frame allocators. That context is only
 * *definable* on Windows, so the header forward-declares it and the
 * implementation creates it inside `_WIN32`. On any other platform the windowed
 * constructor throws `std::runtime_error` and the device stays detached
 * (`hasContext() == false`, `capabilities()` reports nothing).
 *
 * `capabilities()` is probed from the owned device through
 * `D3D12::probeCapabilities()`. `processCommandQueue()` is still a documented
 * `UnsupportedBaseline` stub: D3D12 needs an explicit command-list recorder,
 * descriptor-heap manager and fence-driven retirement queue before a single
 * portable command can be executed correctly.
 *
 * @note **Not validated.** `KaldiD3D12.pro` compiles the Direct3D translation
 * units inside a `win32` scope, so none of this can be built or run on this
 * repository's Linux host.
 */
#include <Kaldi/KldDevice.hpp>
#include "KldImplLib.h"
#include <SDL2/SDL_syswm.h>
#include <span>

namespace D3D12 {

/** @brief Owns the D3D12 device, queues, heaps and swap chain. Windows-only. */
class Context;

/**
 * @brief Direct3D 12 implementation of the Kaldi rendering device contract.
 */
class MH_KALDI_IMPL_API Device : public Kld::Device
{
public:
	/** @brief Creates a detached device without a context. */
	Device();

	/**
	 * @brief Creates the D3D12 device and swap chain for @p window.
	 * @throws std::runtime_error On non-Windows platforms, where the backend
	 *         cannot exist at all.
	 * @throws std::invalid_argument When @p window has no Win32 handle.
	 * @throws std::runtime_error When device, queue or swap-chain creation fails.
	 */
	explicit Device(const SDL_SysWMinfo& window);

	/** @brief Waits for the queue to drain and releases every object. */
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
	/** @brief True when the device owns a D3D12 context. */
	bool hasContext() const noexcept { return context_ != nullptr; }

	/** @brief "Direct3D 12", or "none" when the device is detached. */
	const char* apiName() const noexcept;

private:
	// A raw pointer keeps this header compilable without <d3d12.h>; the
	// destructor deletes the context in the translation unit that defines it.
	Context* context_ = nullptr;
	Kld::HalCapabilities capabilities_ = Kld::halCapabilitiesNone();
};

}

#endif // KALDID3D12DEVICE_HPP
