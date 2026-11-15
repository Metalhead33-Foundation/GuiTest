# KaldiGL

OpenGL 4.3 core backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, but no rendering is
implemented. `GL::Device` declares no overrides for `Kld::Device`'s pure virtual
functions, so it is abstract and nothing can be submitted to it yet. The translation
helpers that the future implementation needs already exist in
[`KaldiGLCommon`](../KaldiGLCommon/README.md).

**General idea:** we'll use both GLX and EGL for Unix-based systems,
WGL on Windows. GLX vs EGL usage will depend on configuration settings (via `Euph::Conf::Configuration`),
environment variables, etc.

Requires [SPIRV-Cross](https://github.com/khronosgroup/spirv-cross) to be built with `cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSPIRV_CROSS_SHARED=ON`.
Otherwise, the compiler will complain about linking static SPIRV-Cross
libraries (`libspirv-cross-core.a`) into a shared library (`.so`) or a
Position-Independent Executable (PIE), when the static libraries were
compiled without Position-Independent Code (`-fPIC`)

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiGL` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Dialect macro | `GL_IMPLEMENTATION` (selects namespace `GL`, `<glad/gl.h>`) |
| Files | `KaldiGLDevice.hpp` / `KaldiGLDevice.cpp` / `KldImplLib.h`, plus the shared sources from `KaldiGLCommon` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; links `spirv-cross-core` and `spirv-cross-glsl` for SPIR-V to GLSL cross-compilation (a backend concern, not a Kaldi or Euphemy dependency) |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiGLDevice.hpp` | `GL::Device : Kld::Device` placeholder class (constructor only) |
| `KaldiGLDevice.cpp` | Stub exports: create returns `nullptr`, destroy is a no-op, renderer name is `"OpenGL Renderer"`, version is `"0.0.1"`, last error is `nullptr` |
| `../KaldiGLCommon/GlUtil.*` | Euphemy `Format` -> GL format translation, `DecodeTarget`/`ImageView` texture uploads, `Kld::VertexDescriptor` -> vertex attribute state, `Kld::BufferMapAccess` -> `GL_MAP_*` |
| `../KaldiGLCommon/GlSpirvShader.*` | SPIR-V (`Kld::ShaderBinaryDescriptor`) -> GLSL 4.30 text, with optional external shader-cache hooks |

## TODO

- [ ] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with probed limits.
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [ ] Create and own an OpenGL 4.3 core context from the `SDL_SysWMinfo` passed to `kld_create_rendering_device`.
- [ ] Implement the backend-owned `DefaultFramebuffer` and the `BeginFrame`/`EndFrame`/`Present` path, plus explicit swapchains.
- [ ] Implement deferred resource retirement for destroy commands.
- [ ] Compile the GLSL produced by `GL::spirvToGlsl()` into program objects and wire `Kld::PipelineBindingLayout` to explicit uniform/sampler bindings.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and clears the default framebuffer.

