# KaldiD3D11

Direct3D 11 backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, but no rendering is
implemented. `D3D11::Device` declares no overrides for `Kld::Device`'s pure virtual
functions, so it is abstract and nothing can be submitted to it yet.

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiD3D11` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiD3D11Device.hpp` / `KaldiD3D11Device.cpp` / `KldImplLib.h` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; links `spirv-cross-core` and `spirv-cross-glsl` for SPIR-V to GLSL/HLSL cross-compilation (a backend concern, not a Kaldi or Euphemy dependency) |
| Platform | Windows (Direct3D 11); the target is still generated on other hosts where the toolchain allows it |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiD3D11Device.hpp` | `D3D11::Device : Kld::Device` placeholder class (constructor only) |
| `KaldiD3D11Device.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"Direct3D11 Renderer"`, version is `"0.0.1"`, last error is `nullptr` |

## TODO

- [ ] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with limits probed from the D3D11 device.
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [ ] Create the `ID3D11Device`/`ID3D11DeviceContext` and an `IDXGISwapChain` from the `SDL_SysWMinfo` HWND.
- [ ] Translate Kaldi shader binaries (SPIR-V words) to HLSL/DXBC, or document the required toolchain.
- [ ] Implement descriptors, render targets, the default framebuffer/swapchain path and explicit swapchains.
- [ ] Implement deferred resource retirement for destroy commands.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and clears the default framebuffer.
