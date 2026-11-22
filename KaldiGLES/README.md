# KaldiGLES

OpenGL ES 3.2 backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: rendering (default framebuffer path).** The library builds and exports the
backend C ABI, `GLES::Device` owns an EGL context and a command executor, and
`processCommandQueue()` executes the Kaldi command vocabulary: buffers, textures,
images, samplers, framebuffers, SPIR-V graphics/compute pipelines, bindings, every
draw and indirect form, render state, barriers, transfers/readbacks, queries, debug
labels and the `BeginFrame`/`Present` path. Destroy commands delete GL names
immediately, which is the HAL's deferred retirement for OpenGL ES. Explicit swapchain
objects are the one deliberate gap (`UnsupportedBaseline`). The executor has been
validated with an offscreen triangle plus per-pixel readback on an ES 3.2 context
(NVIDIA).

Requires [SPIRV-Cross](https://github.com/khronosgroup/spirv-cross) to be built with `cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DSPIRV_CROSS_SHARED=ON`.
Otherwise, the compiler will complain about linking static SPIRV-Cross
libraries (`libspirv-cross-core.a`) into a shared library (`.so`) or a
Position-Independent Executable (PIE), when the static libraries were
compiled without Position-Independent Code (`-fPIC`)

## Build

| Item | Value |
|---|---|
| qmake target | `KaldiGLES` (`TEMPLATE = lib`, shared, C++20, no Qt) |
| Export macro | `MH_KALDI_IMPL_EXPORT` while building, `MH_KALDI_IMPL_API` for consumers |
| Dialect macro | `GLES_IMPLEMENTATION` (selects namespace `GLES`, `<glad/gles2.h>`) |
| Files | `KaldiGLESDevice.hpp` / `KaldiGLESDevice.cpp` / `KldImplLib.h`, plus the shared sources from `KaldiGLCommon` |
| Depends on | Kaldi headers via `include(../Kaldi.pri)`; links `spirv-cross-core` and `spirv-cross-glsl` for SPIR-V to GLSL ES cross-compilation (a backend concern, not a Kaldi or Euphemy dependency) |

## What exists today

| File | State |
|---|---|
| `KldImplLib.h` | Declares the backend C ABI: `kld_create_rendering_device`, `kld_destroy_rendering_device`, `kld_get_renderer_name`, `kld_get_version`, `kld_get_last_error_message` |
| `KaldiGLESDevice.hpp` | `GLES::Device : Kld::Device`; owns the EGL `Context` and a `CommandExecutor`, implements `capabilities()` and `processCommandQueue()`, exposes `context()`, `executor()`, `apiName()`, `swapBuffers()` |
| `KaldiGLESDevice.cpp` | EGL context creation/make-current/GL loading, the capability probe, executor construction and teardown; the C ABI creates/destroys a real device and keeps a thread-local error message |
| `../KaldiGLCommon/GlContext.*`, `EglDisplay.*`, `EglContext.*` | Shared context contract (`Context`, `ContextConfig`, `createContext()`), EGL display refcounting and the EGL implementation |
| `../KaldiGLCommon/GlContextConfig.*` | `Kld::ConfigProvider` for `ContextConfig` (GLES defaults: 3.2, no profile bits) |
| `../KaldiGLCommon/GlCapabilities.*` | `GLES::probeCapabilities()`: `GL_MAX_*` limits, ES version gates for compute/indirect draws/storage resources, and `GL_KHR_debug`/S3TC extension detection |
| `../KaldiGLCommon/GlDeviceConfig.*` | `DeviceConfig` + `DeviceConfigProvider`: context fields composed with the executor policy keys |
| `../KaldiGLCommon/GlCommandExecutor.*` | The `processCommandQueue()` implementation shared with `KaldiGL`, including the ES-specific `UnsupportedBaseline` answers (1D textures, base-instance draws, polygon mode, depth clamp, sampler LOD bias, timestamp queries without `EXT_disjoint_timer_query`) |
| `../KaldiGLCommon/GlExecutorResources.*` / `GlExecutorPipelines.*` / `GlExecutorTransfer.*` | Resource creation/update, SPIR-V pipelines + bindings + draws, and copies/readbacks/queries/fences |
| `../KaldiGLCommon/GlExecutorMapping.*` | Pure Kaldi -> GL enum tables used by the executor |
| `../Kaldi/KldDispatch.hpp` | Backend-side dispatch layer the command queue will use: `KLD_OPCODE_LIST`, `opcodeName()` diagnostics, and the structure-shifting macros (computed goto on GCC/Clang, `switch` fallback) that keep one handler body per opcode |

## ES-specific limitations

`GlUtil` reports these as "unsupported" instead of uploading silently wrong data:

- `GL_BGR`/`GL_BGRA` client formats and 16-bit `UNORM`/`SNORM` colour formats are not
  part of the ES 3.2 core header set we ship (no `EXT_texture_format_BGRA8888`).
- 64-bit (`double`) vertex attributes cannot be represented; `applyVertexDescriptor()`
  returns `false` for them.

Context creation is EGL-only (there is no GLX path in this backend):
`GLES::createContext()` requests `EGL_OPENGL_ES3_BIT` and a 3.2 context, keeping the
rest of `GLES::ContextConfig` identical to the desktop backend.

A CPU-side conversion in Euphemy is the intended fix.

## TODO

- [x] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with probed limits.
- [x] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set (explicit swapchain objects report `UnsupportedBaseline`).
- [x] Create and own an OpenGL ES 3.2 context from the native window handle (EGL, via `GLES::createContext`).
- [ ] Implement the backend-owned `DefaultFramebuffer` and the `BeginFrame`/`EndFrame`/`Present` path, plus explicit swapchains. (The default path works; explicit swapchain objects are still `UnsupportedBaseline`.)
- [ ] Implement deferred resource retirement for destroy commands. (Immediate `glDelete*` is already deferred by the driver; document or replace with an explicit queue.)
- [x] Compile the GLSL ES produced by `GLES::spirvToGlsl()` into program objects.
- [ ] Return real status/error strings instead of `nullptr`. (Devices and executors report messages; `kld_get_last_error_message` is only `nullptr` before the first failure.)
- [x] Add a smoke test that creates a device, uploads a texture and clears the default framebuffer. (An offscreen triangle + readback covers the pipeline; a window-present smoke test is still useful.)
