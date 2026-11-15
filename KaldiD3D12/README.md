# KaldiD3D12

Direct3D 12 backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub, and currently not buildable.** The C ABI is exported and the shared
translation helpers compile, but no rendering is implemented: `D3D12::Device` declares no
overrides for `Kld::Device`'s pure virtual functions, so it is abstract and nothing can be
submitted to it yet. `D3D12Util.cpp` is additionally gated behind `win32` because it needs
`<directx/d3d12.h>`, and there is no working Windows build of this backend yet - the code
is written but untested.

Requires [SPIRV-Cross](https://github.com/khronosgroup/spirv-cross) to be built with `cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSPIRV_CROSS_SHARED=ON`.
Otherwise, the compiler will complain about linking static SPIRV-Cross
libraries (`libspirv-cross-core.a`) into a shared library (`.so`) or a
Position-Independent Executable (PIE), when the static libraries were
compiled without Position-Independent Code (`-fPIC`)

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiD3D12` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiD3D12Device.*`, `KldImplLib.h`, `D3D12Util.*` (win32 only), plus the shared sources from `KaldiD3DCommon` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; links `spirv-cross-core` and `spirv-cross-hlsl` for SPIR-V to HLSL cross-compilation (a backend concern, not a Kaldi or Euphemy dependency). The previous `-lspirv-cross-glsl` link was wrong: this backend emits HLSL. |
| Platform | Windows (Direct3D 12); the portable `.so` stub target is still generated on other hosts |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiD3D12Device.hpp` | `D3D12::Device : Kld::Device` placeholder class (constructor only) |
| `KaldiD3D12Device.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"Direct3D12 Renderer"`, version is `"0.0.1"`, last error is `nullptr` |
| `D3D12Context.hpp` / `.cpp` | `D3D12::Context`: factory, adapter pick, device, direct queue, flip-model swap chain, RTV heap, fence-based `waitForGpu()`, `present()` and `resize()`; **Windows only, unverifiable here** |
| `D3D12Util.hpp` / `D3D12Util.cpp` | `Kld::VertexDescriptor` -> owning `D3D12_INPUT_ELEMENT_DESC` layout, `DecodeTarget` -> `D3D12_SUBRESOURCE_DATA`; **Windows only** |
| `../KaldiD3DCommon/D3DUtil.*` | Euphemy `Format` and `Kld::PrimitiveType` -> `DXGI_FORMAT`, depth/compression queries, 256-byte alignment and mip-count helpers; Windows only |
| `../KaldiD3DCommon/D3DSPirvShader.*` | SPIR-V (`Kld::ShaderBinaryDescriptor`) -> HLSL source (Shader Model 5.1 default, `D3D::direct3D12Options()`); portable |

Headers come from [`extern/DirectX-Headers`](https://github.com/microsoft/DirectX-Headers)
(MIT, official Microsoft repository, included as `<directx/d3d12.h>` /
`<directx/dxgiformat.h>`). Do **not** put `extern/DXSDK/Include` on the same include path:
it contains an older `d3d12.h`/`dxgiformat.h` and the two header sets cannot be mixed.

## TODO

- [ ] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with limits probed from the D3D12 device.
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [ ] Create the factory/device/command queue/swapchain and the descriptor heaps needed by the Kaldi resource model.
- [ ] Translate the Kaldi render-pass and binding-layout descriptors into D3D12 root signatures and pipeline state objects.
- [ ] Feed `D3D::spirvToHlsl()` output through `dxc` to DXIL (or `D3DCompile` to DXBC) and create the shader objects.
- [ ] Implement fences, resource states/barriers and deferred resource retirement.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Get the backend to compile against a real Windows toolchain.
- [ ] Add a smoke test that creates a device, uploads a texture and presents a cleared frame.
