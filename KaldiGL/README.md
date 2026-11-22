# KaldiGL

OpenGL 4.3 core backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, and `GL::Device` now
implements `capabilities()` by probing the context it creates, but `processCommandQueue()`
is still missing, so the class is abstract and nothing can be submitted to it yet. What
*does* work is the window-system and capability half: `GL::createContext()` turns an SDL
window into a current OpenGL 4.3 core context (EGL or GLX), loads the GL entry points, and
`GL::probeCapabilities()` reports the portable limits (`GL_MAX_*`, version gates, S3TC and
`GL_KHR_debug` detection). The translation helpers the implementation needs already exist
in [`KaldiGLCommon`](../KaldiGLCommon/README.md).

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
| Files | `KaldiGLDevice.hpp` / `KaldiGLDevice.cpp` / `GlxContext.hpp` / `GlxContext.cpp` / `KldImplLib.h` (+ `WglContext.*` on Win32), plus the shared sources from `KaldiGLCommon` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; links `spirv-cross-core` and `spirv-cross-glsl` for SPIR-V to GLSL cross-compilation (a backend concern, not a Kaldi or Euphemy dependency) |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiGLDevice.hpp` | `GL::Device : Kld::Device`; owns the `Context` (EGL or GLX), implements `capabilities()` from a probed snapshot, exposes `context()`, `apiName()`, `swapBuffers()`; still abstract because the command-queue half is unimplemented |
| `KaldiGLDevice.cpp` | Context creation/make-current/GL loading and the capability probe in the `Device` constructor, teardown in the destructor; C exports still return `nullptr` until `processCommandQueue()` exists |
| `WglContext.hpp` / `WglContext.cpp` | `GL::WglContext` - WGL implementation of `GL::Context` (Win32, desktop only) and `createWglContext()`; built only on Windows (`win32` scope), so unverified here |
| `GlxContext.hpp` / `GlxContext.cpp` | `GL::GlxContext` - GLX implementation of `GL::Context` (X11, desktop only), plus `createGlxContext()` which the shared `createContext()` dispatcher calls |
| `../KaldiGLCommon/GlContext.*`, `EglDisplay.*`, `EglContext.*` | Shared context contract, EGL display refcounting and the EGL implementation |
| `../KaldiGLCommon/GlContextConfig.*` | `Kld::ConfigProvider` for `ContextConfig`: describes every field (key, range, default), recommends per-purpose defaults and clamps what a `.ini` file says |
| `../KaldiGLCommon/GlUtil.*` | Euphemy `Format` -> GL format translation, `DecodeTarget`/`ImageView` texture uploads, `Kld::VertexDescriptor` -> vertex attribute state, `Kld::BufferMapAccess` -> `GL_MAP_*` |
| `../KaldiGLCommon/GlSpirvShader.*` | SPIR-V (`Kld::ShaderBinaryDescriptor`) -> GLSL 4.30 text, with optional external shader-cache hooks |
| `../KaldiGLCommon/GlCapabilities.*` | `GL::probeCapabilities()`: `GL_MAX_*` limits, actual-context version gates for compute/indirect draws/storage resources, S3TC and `GL_KHR_debug` detection |
| `../Kaldi/KldDispatch.hpp` | Backend-side dispatch layer the command queue will use: `KLD_OPCODE_LIST`, `opcodeName()` diagnostics, and the structure-shifting macros (computed goto on GCC/Clang, `switch` fallback) that keep one handler body per opcode |

## TODO

- [x] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with probed limits (the C ABI still waits for `processCommandQueue()`).
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [x] Create and own an OpenGL 4.3 core context from the `SDL_SysWMinfo` passed to `kld_create_rendering_device` (EGL or GLX, via `GL::createContext`).
- [ ] Implement the backend-owned `DefaultFramebuffer` and the `BeginFrame`/`EndFrame`/`Present` path, plus explicit swapchains.
- [ ] Implement deferred resource retirement for destroy commands.
- [ ] Compile the GLSL produced by `GL::spirvToGlsl()` into program objects and wire `Kld::PipelineBindingLayout` to explicit uniform/sampler bindings.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and clears the default framebuffer.
- [ ] Decide whether `GlxContext` needs a share-context variant (`glXCreateContextAttribsARB` with a shared context) once multi-threaded recording lands.

