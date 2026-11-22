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

## `GameSystem`: audio and a cleared window

`main()` loads `config.ini` (if present, from the working directory), then hands it to a
`GameSystem`, which wires up two independent halves and runs `updateLogic()`/`render()` until the
window is closed or ESC is pressed:

| Half | What it does | Configuration |
|---|---|---|
| Audio | `Haruka_SDL_Driver` opens an SDL audio device, a `Hrk::Mixer` feeds it, and a `Hrk::ModuleStreamer` plays a module file | `[Audio] uSamplerate`, `uChannels`, `uBufferSize`; `[Game] sSongPath`, `fModVolume` |
| Graphics | `KaldiContext` loads a Kaldi backend library, creates its rendering device on the SDL window, and `render()` clears the window to one colour every frame | `[Graphics] sBackendLibrary`, `sClearColor` |

The graphics half is deliberately minimal - one `BeginFrame`/`ClearColorAttachment`/`EndFrame`/
`Present` batch per frame, no resources - but it exercises the whole "backend is a dynamically
loaded library" path: `KaldiContext` `dlopen`s the backend, calls
`kld_create_rendering_device(window, size)`, reinterprets the handle as the `Kld::Device` it wraps,
and destroys the device before unloading the library. Missing backend, missing GPU or a refused
device is not fatal: it logs `Graphics disabled: ...` and the audio half keeps running.

```ini
; build/Desktop-Debug/config.ini
[Graphics]
sBackendLibrary=libKaldiVK.so
sClearColor=0xFF20A030
```

- `sBackendLibrary` is a soname resolved through `LD_LIBRARY_PATH` or an absolute path.
  `libKaldiVK.so`, `libKaldiGL.so` and `libKaldiGLES.so` all work.
- `sClearColor` is a packed `0xAARRGGBB` **string**, not a `u` key: the configuration parses
  numbers in base 10, so a hex literal in a numeric key would silently read as `0`.

Leave `config.ini` out and the defaults apply (`libKaldiVK.so`, `0xFF102030`, 640x480, 48 kHz).

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
