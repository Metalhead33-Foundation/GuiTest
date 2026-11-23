# KaldiGL

OpenGL 4.3 core backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: rendering (default framebuffer path).** The library builds and exports the
backend C ABI, `GL::Device` owns a context and a command executor, and
`processCommandQueue()` executes the Kaldi command vocabulary: buffers, textures,
images, samplers, framebuffers, SPIR-V graphics/compute pipelines, bindings, every
draw and indirect form, render state, barriers, transfers/readbacks, queries, debug
labels and the `BeginFrame`/`Present` path. Destroy commands delete GL names
immediately, which is the HAL's deferred retirement for OpenGL. Explicit swapchain
objects (HAL 1.1) are a validated view of that drawable: `AcquireSwapchainImage`
reports `DefaultFramebuffer` with image index 0, `PresentSwapchain` swaps the window,
and a non-null native surface or mailbox pacing answers `UnsupportedBaseline`; GL presentation is the
default framebuffer. The executor has been validated with an offscreen triangle plus
per-pixel readback on both dialects (NVIDIA, EGL). The context and capability half is
described below.

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
| `KaldiGLDevice.hpp` | `GL::Device : Kld::Device`; owns the `Context` (EGL or GLX) and a `CommandExecutor`, implements `capabilities()` and `processCommandQueue()`, exposes `context()`, `executor()`, `apiName()`, `swapBuffers()` |
| `KaldiGLDevice.cpp` | Context creation/make-current/GL loading, the capability probe, executor construction and teardown; the C ABI creates/destroys a real device and keeps a thread-local error message |
| `WglContext.hpp` / `WglContext.cpp` | `GL::WglContext` - WGL implementation of `GL::Context` (Win32, desktop only) and `createWglContext()`; built only on Windows (`win32` scope), so unverified here |
| `GlxContext.hpp` / `GlxContext.cpp` | `GL::GlxContext` - GLX implementation of `GL::Context` (X11, desktop only), plus `createGlxContext()` which the shared `createContext()` dispatcher calls |
| `../KaldiGLCommon/GlContext.*`, `EglDisplay.*`, `EglContext.*` | Shared context contract, EGL display refcounting and the EGL implementation |
| `../KaldiGLCommon/GlContextConfig.*` | `Kld::ConfigProvider` for `ContextConfig`: describes every field (key, range, default), recommends per-purpose defaults and clamps what a `.ini` file says |
| `../KaldiGLCommon/GlUtil.*` | Euphemy `Format` -> GL format translation, `DecodeTarget`/`ImageView` texture uploads, `Kld::VertexDescriptor` -> vertex attribute state, `Kld::BufferMapAccess` -> `GL_MAP_*` |
| `../KaldiGLCommon/GlSpirvShader.*` | SPIR-V (`Kld::ShaderBinaryDescriptor`) -> GLSL 4.30 text, with optional external shader-cache hooks |
| `../KaldiGLCommon/GlCapabilities.*` | `GL::probeCapabilities()`: `GL_MAX_*` limits, actual-context version gates for compute/indirect draws/storage resources, S3TC and `GL_KHR_debug` detection |
| `../KaldiGLCommon/GlDeviceConfig.*` | `DeviceConfig` + `DeviceConfigProvider`: context fields composed with the executor policy (`bGlDebugLabels`, `bGlForceNearestFiltering`, `bGlForceTrilinear`, `fGlTextureLodBias`, `fGlMaxAnisotropy`, `iGlMaxTextureSize`) |
| `../KaldiGLCommon/GlCommandExecutor.*` | The `processCommandQueue()` implementation: handle registries, tracked state, frame/present path, render passes, debug labels and the `KLD_DISPATCH_*` stream |
| `../KaldiGLCommon/GlExecutorResources.*` / `GlExecutorPipelines.*` / `GlExecutorTransfer.*` | Resource creation/update, SPIR-V pipelines + bindings + draws, and copies/readbacks/queries/fences |
| `../KaldiGLCommon/GlExecutorMapping.*` | Pure Kaldi -> GL enum tables used by the executor (blend, compare, filters, barriers, formats) |
| `../Kaldi/KldDispatch.hpp` | Backend-side dispatch layer the command queue will use: `KLD_OPCODE_LIST`, `opcodeName()` diagnostics, and the structure-shifting macros (computed goto on GCC/Clang, `switch` fallback) that keep one handler body per opcode |

## TODO

- [x] Implement `Device::capabilities()` and report `KaldiHalVersion1_2` with probed limits.
- [x] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set, including explicit swapchain objects (a validated view of the default drawable).
- [x] Create and own an OpenGL 4.3 core context from the `SDL_SysWMinfo` passed to `kld_create_rendering_device` (EGL or GLX, via `GL::createContext`).
- [x] Implement the backend-owned `DefaultFramebuffer` and the `BeginFrame`/`EndFrame`/`Present` path, plus explicit swapchain objects. (A GL swapchain is a validated view of the context drawable: one backend-owned image at index 0, and `PresentSwapchain` swaps the window buffers.)
- [x] Implement deferred resource retirement for destroy commands. GL's own deletion rule *is* the deferral: `glDelete*` flags the object and the driver frees it once no queued command, binding, attachment or vertex array can reference it, so a CPU-side queue would only delay names past their destroy command and let later spans re-bind them. The executor additionally clears the state the driver cannot: destroyed buffers leave the vertex/index binding trackers, a destroyed framebuffer resets to `DefaultFramebuffer`, a destroyed pipeline unbinds the program. See the "Resource lifetime and retirement" section of `GlCommandExecutor.hpp`.
- [x] Compile the GLSL produced by `GL::spirvToGlsl()` into program objects and wire `Kld::PipelineBindingLayout` to explicit uniform/sampler bindings.
- [x] Return real status/error strings instead of `nullptr`: `processCommandQueue()` failures are recorded in the thread-local slot `kld_get_last_error_message()` reads, next to the creation failures that were already recorded, and a successful call leaves the last message intact.
- [x] Add a smoke test that creates a device, uploads a texture and clears the default framebuffer. (An offscreen triangle + readback covers creation, pipelines, draws and transfer; a window-present smoke test is still useful.)
- [x] Decide whether `GlxContext` needs a share-context variant (`glXCreateContextAttribsARB` with a shared context) once multi-threaded recording lands. **Decision: no.** Kaldi funnels every command into one command queue: commands may be *recorded* from multiple threads, but `process()` executes them on a single thread with one current context, so a second context would only alias the GL object namespace (names are shared per share group) without buying parallelism. Multi-threaded execution is a property of APIs that support it explicitly (Vulkan command buffers and queues, D3D12 command lists), which is exactly why only the Vulkan backend records into per-slot command buffers. A share group would be a queue-splitting decision, so it belongs with a future multi-queue executor rather than with a per-window `GlxContext`; see the "Share group" section of `GlxContext.hpp`.

