\page mod_sandbox Sandbox

# Sandbox

`Sandbox` is the project's **manual playground**: a plain console application (`TARGET = Sandbox`)
that a human runs and edits while exploring the libraries. It is not an automated test suite - the
automated, regression-checked tests live in [`../UnitTests`](../UnitTests/README.md).

Hardcoded paths (`/home/legacy/vinque.bin`, `/tmp/testImg.png`, ...), debug printing and
work-in-progress experiments are **explicitly allowed here**. Nothing in this module is expected to
be reproducible on another machine, and nothing here gates a change.

**Status: intentional scratch space.** Expect unused or orphaned functions; they are kept around as
starting points for manual experiments (graphics work especially).

## Build and run

```sh
cd build/Desktop-Debug
/usr/bin/qmake6 ../../Project.pro
env MAKE=/usr/bin/make /usr/bin/make -j16 QMAKE=/usr/bin/qmake6
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK:KaldiGL:KaldiGLES ./Sandbox/Sandbox
```

The sibling shared libraries are not found through an rpath, so the `LD_LIBRARY_PATH` prefix is
required - including the Kaldi backend the graphics context loads at run time (`KaldiVK`, and
`KaldiGL`/`KaldiGLES` when those are selected).

## JavaScript

The inclusion of MozJS / SpiderMonkey is **temporary**. It is **not** intended to be embedded into any of the libraries in our project *(or even in the Sandbox for the long term)*. It is in there for demonstration purposes - our intended long-term trajectory is swapping it for a combination of [WasmJIT](https://github.com/Metalhead33-Foundation/Wasmjit/tree/master) and either [quickjs](https://github.com/bellard/quickjs) or [mquickjs](https://github.com/bellard/mquickjs) *(plus, WasmJIT itself is slated for integration into this project - MhLib - in the long term)*.

The included SpiderMonkey code is for demonstration purposes, plus for ease of integration into the windowing system.

## `GameSystem`: audio, a window and a cursor

`main()` loads `config.ini` (if present, from the working directory), then hands it to a
`GameSystem`, which wires up two independent halves and runs `updateLogic()`/`render()` until the
window is closed or ESC is pressed:

| Half | What it does | Configuration |
|---|---|---|
| Audio | `Haruka_SDL_Driver` opens an SDL audio device, a `Hrk::Mixer` feeds it, and a `Hrk::ModuleStreamer` plays a module file | `[Audio] uSamplerate`, `uChannels`, `uBufferSize`; `[Game] sSongPath`, `fModVolume` |
| Graphics | `Lotte::Gfx::KaldiContext` loads a Kaldi backend library, creates its rendering device on the SDL window, and `render()` clears the window, draws the sample SDF text through `Lotte::Text::TextRenderer`, then draws a cursor through `Lotte::Gui::KaldiGuiRenderer` | `[Graphics] sBackendLibrary`, `sClearColor`, `sCursor`, `sFont` |

The graphics half is the project's first end-to-end texture path. `KaldiContext` `dlopen`s the
backend, calls `kld_create_rendering_device(window, size)` and reinterprets the handle as the
`Kld::Device` it wraps; it then owns the `CommandBuffer` and the `ResourceFactory` that record
everything else. Startup compiles two HLSL programs with
`Lotte::Gfx::compileShaderProgram`, creates the pipelines/sampler, decodes both cursor PNGs out of
`cursors/`, converts them to RGBA8 (`WriteableTexture2D::fromDecodeTarget`), queues the initial
texture upload and submits that batch. Each frame is:

```text
prepare text -> FontRepository::addFaceFromFile(iosys, path) / TextRenderer::prepare
                (lazy atlas uploads, submitted before the frame when new glyphs appeared)
beginFrame   -> BeginFrame + ClearColorAttachment
KaldiGuiRenderer::beginFrame -> reset the CPU caches
TextRenderer::beginFrame/render -> per-page glyph runs + decoration quads
Cursor::render -> one tinted textured quad at the mouse position
flush        -> text draws first, then the cursor (viewport/scissor/blend,
                BindPipeline, BindTexture, BindSampler, buffer upload,
                BindVertex/IndexBuffer, DrawIndexed)
endFrame     -> EndFrame + Present, swapBuffers, processCommandQueue
```

The sample text is loaded from `[Graphics] sFont` through the PhysFS
`Elv::Io::System` (mount its directory, or point `sFont` at a virtual path) and laid
out from MML, so it exercises the family lookup, bold/italic/underline/strike/colour
tags and the SDF atlas at once. A missing font logs `Text disabled: ...` and leaves
the cursor path working.

Missing backend, missing GPU or a refused device is not fatal: it logs
`Graphics disabled: ...` and the audio half keeps running. Cursor images are resolved
relative to the working directory; a missing file is logged and skipped, so the SDF text
path still runs when the binary is started from the build directory.

| Input | Effect |
|---|---|
| Mouse motion / click | Moves the cursor; the cursor's origin point (hotspot) is what lands on the pointer. The window system's pointer is hidden while the window is focused, so only the engine cursor is visible |
| `Tab` | Cycles the loaded cursors (`wodmouse.png`: upper-left arrow, `hourglass2.png`: center-intended hourglass) |
| `1`-`9` | Sets the active cursor's anchor in enum order: TopLeft, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight. Each change logs the re-resolved hotspot |
| `Esc` | Quits |

The startup log names the loaded backend and the clip-space y axis it reports, because that
axis is what the pixel transform has to follow:

```text
INFO: Kaldi HAL 1.2.0 via Vulkan Renderer (0.1.0), clip-space y down (Vulkan-style), clearing to 0xFF102030 every frame
INFO: Kaldi HAL 1.2.0 via OpenGL Renderer (0.1.0), clip-space y up (GL/D3D-style), clearing to 0xFF102030 every frame
```

Vulkan's clip space is y-down while OpenGL's and Direct3D's is y-up, so the sign is not
assumed anywhere in the demo: `KaldiGuiRenderer` asks the resource factory for
`conventions()` (which resolves `Kld::HalCapabilities::clipSpaceYAxis` once, when the device
is attached) and applies the matching pixel transform. Assuming either convention
hard-codes a vertically mirrored UI on the other backend.

```ini
; build/Desktop-Debug/config.ini
[Graphics]
sBackendLibrary=libKaldiVK.so
sClearColor=0xFF20A030
sCursor=cursors/wodmouse.png
sFont=vinque.wodf
```

`sFont`, `sCursor` and `sSongPath` are resolved by the Sandbox's PhysFS
`Elv::Io::System`. Mount the physical directories through `[Game] ModPaths` (or put
the files under the base directory) before startup; absolute host paths are not
opened directly by the font loader.

- `sBackendLibrary` is a soname resolved through `LD_LIBRARY_PATH` or an absolute path.
  `libKaldiVK.so`, `libKaldiGL.so` and `libKaldiGLES.so` all work.
- `sClearColor` is a packed `0xAARRGGBB` **string**, not a `u` key: the configuration parses
  numbers in base 10, so a hex literal in a numeric key would silently read as `0`.
- `sCursor` is the first cursor image; `cursors/hourglass2.png` is always loaded second. The
  directory is copied into the build tree by `Sandbox.pro` (`CONFIG += file_copies`), so launching
  from `build/Desktop-Debug/Sandbox` finds it.

Leave `config.ini` out and the defaults apply (`libKaldiVK.so`, `0xFF102030`,
`cursors/wodmouse.png`, 640x480, 48 kHz).

## What `main()` currently runs

| Function | Covers |
|---|---|
| `testAssetRuntime()` | Euphemy `AssetCatalog`, `DependencyGraph`, `ResourceRegistry`, residency transitions, generation-checked handles, placeholders |
| `testGpuAssetStreamer()` | Lotte `GpuAssetStreamer`: request/record/commit/fail, handle visibility, budget accounting, eviction and refcounts |
| `testAsyncIo()` | Elvavena `Async<ThreadPool>`: read/write/seek/size/flush, `*Then` callbacks, cancellation, error propagation |
| `testTGA()` / `testPNG()` | Euphemy encoders: write a 256x256 test image to `/tmp/testImg.{tga,png}` for visual inspection (no verification) |
| `testMemoryArenas()` | Euphemy `MemoryManager` arenas |
| `testFontRoundTrip()` | Lotte FontPacker `WODF`/`WODI` containers through `Elv::Io::DataStream` |
| `testAudioEffects()` | Haruka effect chain: clamp, crush, dry/wet, planar routing, impulse generation, FFT convolution |
| `testTextPipeline()` | Lotte text pipeline: entities, colours, MML, UTF-8, glyph placement, quads |
| `testSignals()` | Elvavena `Signal`/`Connection`/`ScopedConnection` |
| `testGeometryBatches()` | Lotte `GuiGeometryBatch` and `Gfx::MeshBuilder` against a Kaldi `CommandBuffer` |
| `testNetLoopback()` | Elvavena reference replication pipeline: interest-filtered spawn, ack-gated delta, client parse and jitter buffer (no sockets) |
| `testFontParser()` | Lotte `FontFace` through `FontDataStream`; reads the hardcoded `/home/legacy/vinque.bin`, which must be a FontPacker `WODF` file |

`testFontParser()` is machine-specific on purpose: it prints the metadata of whatever font file the
developer has on disk, and aborts with `Unsupported PreprocessedFontFace magic` when that file is
missing or is not a `WODF` font. That abort is a known property of the playground, not a bug to fix
here - the reproducible round-trip coverage lives in `UnitTests`.

## Tests that exist but are not wired into `main()`

`testCustomAllocator`, `testMapDatastream`, `randomDeviceTest`, `testMemoryAllocator`,
`testIntegralIterable`, `testZstdWithMaps`, `testZstdWithLargeData`, `testEncryptionWithMaps`,
`testEncryptionWithLargeData`, `testCompressionAndEncryptionWithMaps`,
`testCompressionAndEncryptionWithLargeData`, `testMemoryMapped`, `testMemoryMappedTemp`,
`testMemoryMappedTempDlopenNoClose`, `testMemoryMappedTempDlopenClose`, `testConfigFile`,
`testConfiguration`, `testInterpolationFloat`, `testInterpolationFixed`.

## Conventions

- Assertions use `requireCondition()` from `sandbox.hpp`, which throws `std::runtime_error`; a
  failure therefore escapes `main()` and terminates the process via `std::terminate` (SIGABRT).
  That is acceptable here.
- Feel free to add scenarios that open windows, play audio or dump images. Nothing in `Sandbox` is
  built by the unit-test job.
- Do **not** add reusable round-trip/regression checks here; put them in `UnitTests` instead.
