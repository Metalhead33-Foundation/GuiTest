# KaldiGLES

OpenGL ES 3.2 backend for the Kaldi graphics HAL (see [Kaldi](../Kaldi/README.md)).

**Status: stub.** The library builds and exports the backend C ABI, but no rendering is
implemented. `GLES::Device` declares no overrides for `Kld::Device`'s pure virtual
functions, so it is abstract and nothing can be submitted to it yet. What *does* work is
the window-system half: `GLES::createContext()` turns an SDL window into a current OpenGL
ES 3.2 context through EGL and loads the GL entry points - the remaining work is the
command queue. The translation helpers the implementation needs already exist in
[`KaldiGLCommon`](../KaldiGLCommon/README.md) and are compiled into this library - before
the refactor, `GlUtil`/`GlSpirvShader` only existed inside `KaldiGL`.

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
| `KaldiGLESDevice.hpp` | `GLES::Device : Kld::Device`; owns the EGL `Context`, exposes `context()`, `apiName()`, `swapBuffers()`; still abstract because the command-queue half is unimplemented |
| `KaldiGLESDevice.cpp` | EGL context creation/make-current/GL loading in the `Device` constructor, teardown in the destructor; C exports still return `nullptr` until `capabilities()`/`processCommandQueue()` exist |
| `../KaldiGLCommon/GlContext.*`, `EglDisplay.*`, `EglContext.*` | Shared context contract (`Context`, `ContextConfig`, `createContext()`), EGL display refcounting and the EGL implementation |
| `../KaldiGLCommon/GlContextConfig.*` | `Kld::ConfigProvider` for `ContextConfig` (GLES defaults: 3.2, no profile bits) |

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

- [ ] Implement `Device::capabilities()` and report `KaldiHalVersion1_0` with probed limits (this is what unblocks `kld_create_rendering_device`).
- [ ] Implement `Device::processCommandQueue(std::span<const GfxOp>)` for the Kaldi opcode set.
- [x] Create and own an OpenGL ES 3.2 context from the native window handle (EGL, via `GLES::createContext`).
- [ ] Implement the backend-owned `DefaultFramebuffer` and the `BeginFrame`/`EndFrame`/`Present` path, plus explicit swapchains.
- [ ] Implement deferred resource retirement for destroy commands.
- [ ] Compile the GLSL ES produced by `GLES::spirvToGlsl()` into program objects.
- [ ] Return real status/error strings instead of `nullptr`.
- [ ] Add a smoke test that creates a device, uploads a texture and clears the default framebuffer.
