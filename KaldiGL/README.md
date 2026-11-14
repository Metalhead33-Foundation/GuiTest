# KaldiGL

OpenGL 4.x backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, but no rendering is
implemented. `GL::Device` declares no overrides for `Kld::Device`'s pure virtual
functions, so it is abstract and nothing can be submitted to it yet.

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiGL` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Files | `KaldiGLDevice.hpp` / `KaldiGLDevice.cpp` / `KldImplLib.h` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; links `spirv-cross-core` and `spirv-cross-glsl` for SPIR-V to GLSL/HLSL cross-compilation (a backend concern, not a Kaldi or Euphemy dependency) |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiGLDevice.hpp` | `GL::Device : Kld::Device` placeholder class (constructor only) |
| `KaldiGLDevice.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"OpenGL Renderer"`, version is `"0.0.1"`, last error is `nullptr` |

## TODO

- [ ] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with probed limits.
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [ ] Create and own an OpenGL 4.x context from the `SDL_SysWMinfo` passed to `kld_create_rendering_device`.
- [ ] Implement the backend-owned `DefaultFramebuffer` and the `BeginFrame`/`EndFrame`/`Present` path, plus explicit swapchains.
- [ ] Implement deferred resource retirement for destroy commands.
- [ ] Translate shader binaries (Kaldi passes SPIR-V words) to GLSL, or document the required toolchain.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and clears the default framebuffer.
