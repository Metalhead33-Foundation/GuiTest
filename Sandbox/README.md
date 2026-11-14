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
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka ./Sandbox/Sandbox
```

The sibling shared libraries are not found through an rpath, so the `LD_LIBRARY_PATH` prefix is
required. `main()` calls the scenarios in `sandbox.cpp` one after another; edit it freely.

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
