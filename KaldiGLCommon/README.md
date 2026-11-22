# KaldiGLCommon

Shared implementation details of the [KaldiGL](../KaldiGL/README.md) and
[KaldiGLES](../KaldiGLES/README.md) backends. **No library is built here** -
the sources are compiled into both backends, because a translation unit can
only target one GLSL dialect at a time.

| File | Purpose |
|---|---|
| `GlBackend.hpp` | Dialect selector: maps `GL_IMPLEMENTATION` / `GLES_IMPLEMENTATION` onto `KALDIGL_NS` (`GL` / `GLES`) and `KALDIGL_IS_GLES` |
| `GlContext.hpp` / `GlContext.cpp` | Abstract `Context`, `ContextConfig`, the `createContext()` factory, and glad entry-point loading (`loadGlFunctions` / `unloadGlFunctions` / `applyDefaultState`) |
| `GlContextConfig.hpp` / `GlContextConfig.cpp` | `Kld::ConfigProvider` implementation for `ContextConfig`: field descriptors with ranges/defaults, per-purpose common defaults, and the clamping `apply()` |
| `EglDisplay.hpp` / `EglDisplay.cpp` | Reference-counted `EGLDisplay` owner shared by every EGL context, plus `eglErrorName()` |
| `EglContext.hpp` / `EglContext.cpp` | EGL implementation of `Context`: window surface, `ContextConfig`-driven config/context attributes, error reporting |
| `GlUtil.hpp` / `GlUtil.cpp` | Euphemy `Format` -> GL format translation, `DecodeTarget`/`ImageView` uploads, `Kld::VertexDescriptor` -> vertex attribute state (classic and explicit-binding overloads), `Kld::BufferMapAccess` -> `GL_MAP_*` bits |
| `GlSpirvShader.hpp` / `GlSpirvShader.cpp` | SPIR-V (`Kld::ShaderBinaryDescriptor`) -> GLSL / GLSL ES text through SPIRV-Cross, with optional external cache hooks |
| `GlDeviceConfig.hpp` / `GlDeviceConfig.cpp` | `DeviceConfig` (context + executor policy) and its `Kld::ConfigProvider`: UT-inspired keys (`bGlDebugLabels`, `bGlForceNearestFiltering`, `bGlForceTrilinear`, `fGlTextureLodBias`, `fGlMaxAnisotropy`, `iGlMaxTextureSize`) composed with the context fields |
| `GlCommandExecutor.hpp` / `GlCommandExecutor.cpp` | The `Kld::Device::processCommandQueue()` implementation and its dispatch stream, handle registries, tracked state, frame/present path, clears, render passes and debug labels |
| `GlExecutorResources.cpp` / `GlExecutorPipelines.cpp` / `GlExecutorTransfer.cpp` | Resource creation/updates, SPIR-V pipelines + binding + draw handlers, and copies/readbacks/queries/fences |
| `GlExecutorMapping.hpp` | Inline Kaldi -> GL enum tables (blend, compare, stencil, filters, wrapping, index type, barriers, formats) shared by the executor translation units |
| `KaldiGLCommon.pri` | Adds the include path and exposes `$$KALDIGL_COMMON_SOURCES` / `$$KALDIGL_COMMON_HEADERS` |

`GlxContext.hpp` / `GlxContext.cpp` live in [KaldiGL](../KaldiGL/README.md)
instead, because GLX is desktop-only.

## Context layer

Both backends obtain their context through the same entry point, so neither of
them hard-codes a window system:

```
SDL_GetWindowWMInfo(&info)
        │
        ▼
Kld::nativeWindowHandlesFrom(info)   Kaldi/KldNativeWindow.hpp
        │  system + display/window (+ unsupportedReason)
        ▼
createContext(handles, config)   GlContext.cpp
        ├── ContextApi::EGL  -> createEglContext()   EglContext.cpp   (both backends)
        ├── ContextApi::GLX  -> createGlxContext()   GlxContext.cpp   (KaldiGL, X11)
        └── ContextApi::WGL  -> createWglContext()   WglContext.cpp   (KaldiGL, Win32)
        │
        ▼
makeCurrent() -> loadGlFunctions() -> applyDefaultState()
```

| Implementation | Header | Dialect | Window systems |
|---|---|---|---|
| `EglContext` | `EglContext.hpp` | OpenGL 4.3 and OpenGL ES 3.2 | X11, Wayland, Windows (ANGLE), Android, embedded |
| `GlxContext` | `../KaldiGL/GlxContext.hpp` | desktop OpenGL only | X11 |
| `WglContext` | `../KaldiGL/WglContext.hpp` | desktop OpenGL only | Win32 |

Behaviour that the legacy code got wrong and that is now defined:

- **One `EGLDisplay`, one `eglInitialize`.** `EglDisplay::acquire()` refcounts the
  display per native handle, so a second window (or a share context) does not
  re-initialize or, worse, terminate a display another context still uses.
- **Configuration instead of a hard-coded attribute blob.** The old
  `EGL_BUFFER_SIZE 16` request was replaced by `ContextConfig`-driven sizes; the
  GLX side now frees its `GLXFBConfig` array with `XFree` and falls back from the
  detailed visual request to a minimal one before giving up.
- **Errors name the failing call** (`eglChooseConfig`, `eglCreateWindowSurface`,
  `glXCreateContextAttribsARB`, ...) together with `eglErrorName()`, instead of a
  bare `std::runtime_error("Unable to get display!")`.
- **Explicit release path.** `makeCurrent()` / `releaseCurrent()` / `swapBuffers()`
  are part of the abstract `Context`, so the device does not have to know which
  binding created it.

### `ContextConfig`

| Field | Desktop GL default | GLES default | Notes |
|---|---|---|---|
| `api` | `Auto` | `Auto` | Resolves to `suggestedApi`; GLES always ends up on EGL |
| `versionMajor` / `versionMinor` | 4 / 3 | 3 / 2 | EGL uses `MAJOR`/`MINOR` when the display has EGL 1.5 or `EGL_KHR_create_context`, otherwise `EGL_CONTEXT_CLIENT_VERSION` |
| `coreProfile` / `forwardCompatible` | `true` / `true` | `false` / `false` | ES has no profile bits |
| `debugContext` | `false` | `false` | Requests a debug context when the driver offers one |
| `doubleBuffered` | `true` | `true` | GLX attribute; EGL window surfaces are always back-buffered |
| `red/green/blue/alphaBits` | 8 each | 8 each | `0` omits the attribute |
| `depthBits` / `stencilBits` | 24 / 8 | 24 / 8 | `0` omits the attribute |
| `sampleCount` | 0 | 0 | `> 1` adds `EGL_SAMPLE_BUFFERS`/`EGL_SAMPLES` |
| `screen` | -1 | -1 | X11 screen for GLX; -1 means `DefaultScreen()` |

### Window systems

`Kld::nativeWindowHandlesFrom()` (in `Kaldi/KldNativeWindow.hpp`) is the only place that
knows about SDL's subsystems; this layer just maps the result onto a context API:

| SDL subsystem | display | window | Suggested API |
|---|---|---|---|
| `SDL_SYSWM_X11` | `Display*` | `Window` | `GLX` (EGL works too) |
| `SDL_SYSWM_WAYLAND` | `wl_display*` | `wl_egl_window*` (from SDL 2.0.16+) | `EGL` |
| `SDL_SYSWM_WINDOWS` | default | `HWND` | `EGL` (WGL is not implemented yet) |
| `SDL_SYSWM_ANDROID` | default | `ANativeWindow*` | `EGL` |
| anything else | - | - | `unsupportedReason` is set and `createContext()` throws |

## Planned: backend configuration callbacks

`ContextConfig` is intentionally a plain aggregate, because it is meant to become
the persisted surface of a two-way conversation:

1. the **engine** asks a backend what it would like to use for a purpose
   ("main window", "offscreen", "debug view");
2. the **backend** answers with the defaults and the accepted range per field;
3. the host stores the merged result as an `.ini` file through
   `Euph::Conf::Configuration` (`[graphics]` section, typed `key=value` lines such
   as `iGlVersionMajor=4`);
4. on the next start the file is read back, clamped to the ranges the backend
   reported, and passed to `createContext()`.

The proposed shape - matching Kaldi's POD/function-pointer style rather than
`std::function` - is:

```cpp
// Not implemented yet; sketched here so the ContextConfig fields stay stable.
enum class ContextPurpose : uint8_t { MainWindow, Offscreen, DebugView };

struct ContextFieldDescriptor {
    const char* key;      // "iGlVersionMajor" -> Euphemy's typed key
    int minimum;
    int maximum;
    const char* help;
};

class ContextConfigProvider {
public:
    virtual ~ContextConfigProvider() = default;
    // Defaults the backend recommends for this purpose, already clamped.
    virtual ContextConfig describe(ContextPurpose purpose) const = 0;
    // The fields the backend exposes for editing, with their valid ranges.
    virtual std::span<const ContextFieldDescriptor> fields() const = 0;
};
```

Until that exists, callers construct `ContextConfig` directly - the struct is
already the exact shape a `.ini` round-trip needs.


## Configuration

The configuration contract lives in `Kaldi/KldConfig.hpp`; this directory provides the GL
implementation of it. The conversation has two directions:

1. the **engine** asks the backend what it recommends for a purpose
   (`Kld::ContextPurpose`: main window, offscreen, debug view);
2. the **backend** answers with backend-neutral defaults (`Kld::DeviceConfig`: resolution,
   frames in flight, present mode, vsync, sRGB, anisotropy, mipmaps) plus the backend
   specific fields it exposes (`Kld::ConfigField`: key, type, range, default, choices,
   help);
3. the host persists the result as an `.ini` file through `Euph::Conf::Configuration` -
   `Lotte/Conf/LteBackendConfig.hpp` writes the defaults, serializes the file, parses it
   back and hands a `Kld::ConfigReader` to `apply()`;
4. `apply()` clamps every value to the advertised range and stores it in the
   `ContextConfig` that `createContext()` will use.

```cpp
GL::ContextConfig config = GL::defaultContextConfig();
GL::ContextConfigProvider provider(config);      // implements Kld::ConfigProvider

Euph::Conf::Configuration ini;                   // Lotte::Conf owns the .ini bridge
Lotte::Conf::writeBackendDefaults(ini, provider, Kld::ContextPurpose::MainWindow);
Lotte::Conf::saveConfiguration(iosys, "/config/kaldi.ini", ini);

Euph::Conf::Configuration loaded = Lotte::Conf::loadConfiguration(iosys, "/config/kaldi.ini");
Lotte::Conf::applyBackendConfig(loaded, provider);   // clamps, then writes into config
auto context = GL::createContext(handles, config);
```

Keys written by the GL provider (all Euphemy-typed, so the file is self-describing):

| Key | Range | Default (desktop) | Meaning |
|---|---|---|---|
| `sGlApi` | `auto`, `egl`, `glx`, `wgl` | `auto` | Window-system binding |
| `iGlVersionMajor` | 1-5 | 4 (3 on GLES) | Requested major version |
| `iGlVersionMinor` | 0-9 | 3 (2 on GLES) | Requested minor version |
| `bGlCoreProfile` | bool | true (false on GLES) | Core profile |
| `bGlForwardCompatible` | bool | true (false on GLES) | Forward-compatible context |
| `bGlDebugContext` | bool | false | Debug context when available |
| `iGlSamples` | 0-16 | 0 | MSAA samples |
| `iGlDepthBits` | 0-32 | 24 | Depth buffer size |
| `iGlStencilBits` | 0-8 | 8 | Stencil buffer size |
| `iGlScreen` | -1-15 | -1 | X11 screen for GLX |

`iWidth`, `iHeight`, `iFramesInFlight`, `sPresentMode`, `bVSync`, `bSrgbFramebuffer`,
`fMaxAnisotropy` and `bGenerateMipmaps` are shared with the other backends through
`Kld::commonConfigFields()`, and `UnitTests` covers the round-trip (defaults written,
values clamped, text re-parsed).

## Why a shared directory instead of `glad/`?

`glad/` is vendored third-party loader code and is regenerated from
<https://gen.glad.sh>; hand-written backend code must not be mixed into it.
`KaldiGLCommon/` follows the same pattern the repository already uses for
`Kaldi/Kaldi.pri`: an include path plus a source list that consuming projects
pull in. Nothing here is a subproject, so `Project.pro` does not list it.

## Dialect differences that are handled here

| Topic | Desktop OpenGL 4.3 | OpenGL ES 3.2 |
|---|---|---|
| Source include | `<glad/gl.h>` | `<glad/gles2.h>` |
| Namespace | `GL` | `GLES` |
| Context creation | EGL or GLX | EGL only |
| Renderable-type attribute | `EGL_OPENGL_BIT` | `EGL_OPENGL_ES3_BIT` |
| `GL_BGR` / `GL_BGRA` client formats | supported | not in the core header set -> reports "unsupported" |
| 16-bit `UNORM`/`SNORM` color formats | supported | not core in ES 3.x -> reports "unsupported" |
| 64-bit (`double`) vertex attributes | `glVertexAttribLPointer` | not representable -> `applyVertexDescriptor()` returns `false` |
| GLSL output | `#version 430`, highp defaults | `#version 320 es`, mediump fragment defaults |

Anything that reports "unsupported" is expected to be handled by CPU-side
conversion (for example a BGRA->RGBA swizzle in Euphemy) before it reaches the
backend.
