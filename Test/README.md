# Test

`Test` is the project's runnable smoke-test application (`TARGET = MyGameApp`). It is a
plain console program, not a unit-test framework: assertions are checked by
`requireCondition()`, which throws `std::runtime_error` on failure.

**Status: working, but partial.** The harness runs a subset of the available test
functions. Several tests declared in `test.hpp` are never called from `main()`.

## Build and run

The app links Elvavena, Euphemy, Lotte and Haruka:

```sh
cd build/Desktop-Debug
/usr/bin/qmake6 ../../Project.pro
env MAKE=/usr/bin/make /usr/bin/make -j16 QMAKE=/usr/bin/qmake6
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:Test ./Test/MyGameApp
```

The sibling shared libraries are not found through an rpath, so the `LD_LIBRARY_PATH`
prefix is required. Several tests use absolute `/tmp` paths.

## What `main()` currently runs

| Test | Covers |
|---|---|
| `testAssetRuntime()` | Euphemy `AssetCatalog`, `DependencyGraph`, `ResourceRegistry`, residency transitions, generation-checked handles, placeholders |
| `testGpuAssetStreamer()` | Lotte `GpuAssetStreamer`: request/record/commit/fail, handle visibility, budget accounting, eviction and refcounts |
| `testAsyncIo()` | Elvavena `Async<ThreadPool>`: read/write/seek/size/flush, `*Then` callbacks, cancellation, error propagation |
| `testTGA()` | Euphemy TGA encoder: renders a 256x256 test image to `/tmp/testImg.tga` |
| `testPNG()` | Euphemy PNG encoder: renders a 256x256 test image to `/tmp/testImg.png` |
| `testMemoryArenas()` | Euphemy `MemoryManager`: default-resource fallback, explicit initialization, idempotence, over-aligned allocation and a PMR vector round-trip |
| `testFontRoundTrip()` | Lotte FontPacker `WODF` and `WODI` containers: write/read through `Elv::Io::DataStream`, including TOC, kerning, glyph payload and mipmaps |
| `testAudioEffects()` | Haruka effect chain: clamp math, dry/wet blending, mono/stereo planar routing, planar output requests, impulse-generation kernels, FFT convolution and the "no IR" error path |
| `testTextPipeline()` | Lotte text pipeline: HTML entities, markup colours, MML tags into `TextRun`s, nesting depth, UTF-8 escapes, glyph placement, italic shear, underline emission and glyph quads/vertices |
| `testSignals()` | Elvavena `Signal`/`Connection`/`ScopedConnection`, including disconnect-during-emission |
| `testGeometryBatches()` | Lotte `GuiGeometryBatch` and `Gfx::MeshBuilder` recording into a Kaldi `CommandBuffer`, including group ranges and vertex descriptors |
| `testFontParser()` | Lotte `FontDataStream >> FontFace`; reads the hardcoded path `/home/legacy/vinque.bin`, which must be a FontPacker `WODF` file |

`testTGA()` and `testPNG()` only write files; they do not decode and compare them.
`testFontParser()` is machine-specific because of its hardcoded input path, and it
aborts with `Unsupported PreprocessedFontFace magic` if the file is missing or is not a
`WODF` font.

## Tests that exist but are not wired into `main()`

`testCustomAllocator`, `testMapDatastream`, `randomDeviceTest`, `testMemoryAllocator`,
`testIntegralIterable`, `testZstdWithMaps`, `testZstdWithLargeData`,
`testEncryptionWithMaps`, `testEncryptionWithLargeData`,
`testCompressionAndEncryptionWithMaps`, `testCompressionAndEncryptionWithLargeData`,
`testMemoryMapped`, `testMemoryMappedTemp`, `testMemoryMappedTempDlopenNoClose`,
`testMemoryMappedTempDlopenClose`, `testConfigFile`, `testConfiguration`,
`testInterpolationFloat`, `testInterpolationFixed`.

## TODOs and known gaps

- [ ] Replace the ad-hoc `requireCondition()` harness with a real test framework (or at
      least a runner that executes every declared test and reports pass/fail).
- [ ] Call the currently orphaned tests from `main()` or a test registry.
- [ ] Remove hardcoded paths (`/home/legacy/vinque.bin`, `/tmp/testImg.*`); take inputs
      from an argument, environment variable, or fixture directory. Provide a checked-in
      `WODF` fixture: the file currently on the development machine is rejected with
      `Unsupported PreprocessedFontFace magic`.
- [ ] Add tests for Haruka, the Kaldi command buffer/facade, the Kaldi backends and the
      Euphemy image decoders.
- [ ] Turn failures into a clean non-zero exit with a diagnostic. Today the exception
      escapes `main()` and the process terminates via `std::terminate` (SIGABRT).
