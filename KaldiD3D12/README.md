# KaldiD3D12

Direct3D 12 backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, but no rendering is
implemented. `D3D12::Device` declares no overrides for `Kld::Device`'s pure virtual
functions, so it is abstract and nothing can be submitted to it yet.

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiD3D12` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiD3D12Device.hpp` / `KaldiD3D12Device.cpp` / `KldImplLib.h` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)` |
| Platform | Windows (Direct3D 12); the target is still generated on other hosts where the toolchain allows it |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiD3D12Device.hpp` | `D3D12::Device : Kld::Device` placeholder class (constructor only) |
| `KaldiD3D12Device.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"Direct3D12 Renderer"`, version is `"0.0.1"`, last error is `nullptr` |

## TODO

- [ ] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with limits probed from the D3D12 device.
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [ ] Create the factory/device/command queue/swapchain and the descriptor heaps needed by the Kaldi resource model.
- [ ] Translate the Kaldi render-pass and binding-layout descriptors into D3D12 root signatures and pipeline state objects.
- [ ] Translate Kaldi shader binaries (SPIR-V words) to DXIL, or document the required toolchain.
- [ ] Implement fences, resource states/barriers and deferred resource retirement.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and presents a cleared frame.
