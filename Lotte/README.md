# Lotte

Lotte is the higher-level graphics and GUI library of MhLib. It sits on top of Kaldi
(the graphics HAL) and Euphemy (assets, images, text-adjacent I/O), and is where
renderer-facing integration belongs so that Euphemy can stay independent of Kaldi.

**Status: partial / early.** Lotte is no longer an empty placeholder. It currently owns
the Kaldi bridge types, the CPU-to-GPU asset upload scheduler, and the FontPacker-compatible
font/vector-image formats. There is no renderer or GUI drawing code yet.

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Lotte` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_LOTTE_EXPORT` while building, `MH_LOTTE_API` for consumers |
| Consumer hook | `include(Lotte/Lotte.pri)` adds the include path and links Lotte/Euphemy/Elvavena |
| Dependencies | Elvavena, Euphemy, Kaldi headers, GLM |

Only `Asset/LteGpuAssetStreamer.cpp` is compiled. Everything else is header-only.

Kaldi is consumed header-only through `Kaldi.pri`. Do not add `-lKaldi` or `libKaldi`
to Lotte; use the top-level include path.

## Implemented

| Header | Contents |
|---|---|
| `LotteLib.hpp` | `MH_LOTTE_API` export macro |
| `LteKld.hpp` | Lotte aliases over the Kaldi facade: `CommandBuffer`, `Buffer`, `Framebuffer`, `Fence`, `Image`, `Pipeline`, `ComputePipeline`, `QueryPool`, `Sampler`, `Texture`, plus `OpAllocator`/`HandleAllocator` |
| `Asset/LteGpuAssetStreamer.hpp` / `.cpp` | `GpuAssetStreamer` - bridges Euphemy's CPU `ResourceRegistry` to Kaldi command buffers. Queues buffer/texture uploads and evictions, accounts GPU bytes against a budget, supports priority/pinning/strong references, and records work into move-only `GpuCommandBatch` objects that keep payload memory alive until commit or fail |
| `Text/LteFontFace.hpp` | FontPacker-compatible `WODF` font-face format: SDF/MSDF/MSDFA channel types, distance metrics, glyph metrics, kerning tables, absolute-offset TOC, and both importer (`operator>>`) and exporter (`operator<<`) over `Elv::Io::DataStream` |
| `VectorGraphic/LteStoredVectorImage.hpp` | FontPacker-compatible `WODI` stored-vector-image format with mip levels, encoding flags and stream import/export |

### Typical asset flow

1. Euphemy's `AssetCatalog`/`ResourceRegistry`/`StreamingScheduler` load and decode an
   asset into a CPU `ResidentPayload`.
2. `Lotte::Asset::GpuAssetStreamer::requestBuffer` / `requestTexture` queue it.
3. `recordUploads` records Kaldi create commands into a `GpuCommandBatch`.
4. The caller submits the batch to a `Kld::Device`.
5. `commitUploads` publishes the Kaldi handle, or `failUploads` records the backend
   rejection. Eviction follows the same record/commit/fail cycle.

## TODOs and known gaps

- [ ] No renderer. There is no render graph, command-list builder, material/shader
      management, mesh handling, camera, or frame loop in Lotte yet.
- [ ] No GUI system. No widgets, layout, input routing, text layout/shaping, or font
      atlas generation/upload. `Text/LteFontFace.hpp` only parses and writes the font
      format.
- [ ] No 3D rendering features (scene graph, lighting, shadows, post-processing).
- [ ] No Kaldi `Swapchain`/`RenderPass` RAII proxies in `LteKld.hpp`; those commands are
      available only through the raw Kaldi opcode vocabulary.
- [ ] No tests beyond `testFontParser()` in the Test app; the GPU asset streamer is
      exercised by `testGpuAssetStreamer()` but no renderer exists to consume it.
- [ ] Font/vector-image formats are read/written through `Elv::Io::DataStream`; there is
      no standalone file loader/saver wrapper in Lotte.

## Build notes

This is a qmake project. Makefiles are generated artifacts and are intentionally ignored
by the repository. Regenerate them from the existing shadow build directory, for example:

```sh
cd build/Desktop-Debug
/usr/bin/qmake6 ../../Project.pro
```

Do not edit source-tree Makefiles to fix Lotte build behavior; update the `.pro` or
`.pri` files and regenerate. Do not create Makefiles in this folder.
