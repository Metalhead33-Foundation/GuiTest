# Lotte

Lotte is the higher-level graphics and GUI library of MhLib. It sits on top of Kaldi
(the graphics HAL) and Euphemy (assets, images, text-adjacent I/O), and is where
renderer-facing integration belongs so that Euphemy can stay independent of Kaldi.

**Status: partial.** Lotte owns the Kaldi bridge types, the CPU-to-GPU asset upload
scheduler, the FontPacker-compatible font/vector-image formats, the rich-text
pipeline (MML parsing, text layout, glyph quads) and the mesh/GUI geometry batches.
There is no renderer and no GUI widget system yet; see the TODO list.

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Lotte` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_LOTTE_EXPORT` while building, `MH_LOTTE_API` for consumers |
| Consumer hook | `include(Lotte/Lotte.pri)` adds the include path and links Lotte/Euphemy/Elvavena |
| Dependencies | Elvavena, Euphemy, Kaldi headers, GLM, utfcpp (header-only, `#include <utf8cpp/utf8.h>`) |

Compiled units: `Asset/LteGpuAssetStreamer.cpp` and the six `Text/*.cpp` files. The
`Gui/` and `GFX/` headers are header-only (templates plus inline recording helpers).

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

### `Text/` - rich text pipeline

| Header | Contents |
|---|---|
| `LteTextAttributes.hpp` | `Colour`, `TextAttributes` (colour, family, size, bold/italic/underline/strikethrough) and `TextRun` (one `std::u32string` plus its attributes) |
| `LteUtf.hpp` | UTF-8 ↔ UTF-32 conversion over [utfcpp](https://github.com/nemtrif/utfcpp) (replaces the deprecated `std::codecvt` usage); invalid input decodes to U+FFFD instead of throwing |
| `LteRichText.hpp` / `.cpp` | `RichTextProcessor` - buffers UTF-8, emits `TextRun`s on attribute changes, merges adjacent runs with equal attributes, and keeps the original `enable*`/`disable*`/`Manipulator`/`operator<<` surface |
| `LteMarkupStack.hpp` / `.cpp` | `MarkupStack` - per-attribute nesting depths (colour, family, size, bold, italic, underline, strikethrough) driving a processor |
| `LteMml.hpp` / `.cpp` | `MmlParser` - the MML state machine and tag tables (`b`, `strong`, `i`, `em`, `u`, `s`, `del`, `strike`, `br`, `p`, `font=`, `size=`, `color=`/`colour=`) |
| `LteHtmlEntities.hpp` / `.cpp` | Named/decimal/hexadecimal HTML character references; the 2,124-entry table is carried over verbatim from the old tree |
| `LteMarkupColour.hpp` / `.cpp` | `parseMarkupColour()` - web colour names, `#RGB(A)` / `#RRGGBB(AA)` hex, and normalized or byte component lists |
| `LteTextLayout.hpp` / `.cpp` | `layoutRun()` / `layoutRuns()` - glyph placement from a `PreprocessedFontFace` (bearings, measured line height, newline handling, italic shear, underline/strikethrough segments) producing `PlacedGlyph`/`DecorationLine`/`TextStyle` |
| `LteFontAtlas.hpp` | The glyph atlas contract that does not exist yet (`GlyphAtlasMapping`), plus `appendGlyphQuads()` which turns a layout into textured quads |
| `MML.md` | The MML language specification |

### `Gui/` and `GFX/` - geometry and draw vocabulary

| Header | Contents |
|---|---|
| `Gui/LteGuiVertex.hpp` | `ColouredGuiVertex`, `TexturedGuiVertex`, `ColouredTexturedGuiVertex` with Kaldi `VertexDescriptor`s, and `appendGlyphVertices()` |
| `Gui/LteGuiRenderCache.hpp` | `GuiGeometryBatch<Vertex>` - CPU vertex/index cache that records create/resize/upload, bind and indexed draw commands, with an explicit sub-range |
| `Gui/LteGuiRenderer.hpp` | GUI primitive vocabulary (`ColouredQuad`, `GradientQuad`, `TexturedQuad`, ...) and the abstract `GuiRenderer` with its bulk-submission helpers |
| `GFX/LteMesh.hpp` | `MeshView` (buffer handles + layout + counts + index width + `MeshDrawRange` groups) with inline `recordBind`/`recordDraw`/`recordDrawInstanced`, and `MeshBuilder<Vertex>` that owns the buffers and records uploads |

### FontPacker binary formats

`Text/LteFontFace.hpp` implements the FontPacker `WODF` preprocessed-font container and
`VectorGraphic/LteStoredVectorImage.hpp` implements the standalone `WODI` vector-image
container. Both use `Elv::Io::DataStream` with big-endian byte order, including the
null-inclusive UTF-8 string encoding and `uint32`-sized containers, and follow the
upstream specification:

<https://github.com/Metalhead33-Foundation/FontPacker/blob/master/BINARY_FORMAT.md>

The CBOR map variants described by that specification are not implemented.

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
      management, camera or frame loop. `Gui/LteGuiRenderer.hpp` defines the interface a
      GUI renderer must implement but deliberately ships no implementation.
- [ ] No GUI widget system. No widgets, layout, input routing or focus handling. The old
      `Widget`/`RootNode`/`TickboxWidget` classes were not carried over; `ElvSignal.hpp`
      provides the signals they used.
- [ ] No glyph atlas. `Text/LteFontAtlas.hpp` documents what is missing (decode
      `StoredCharacter::sdf`, pack into a texture with `bitmapPadding`, expose UV
      rectangles through `GlyphAtlasMapping`) and implements the quad emission, but no
      texture, packer or Kaldi upload path exists.
- [ ] No text shaping beyond glyph placement: kerning is parsed from the font and stored
      in `PreprocessedFontFace::kerning`, but the unit of its offsets has to be confirmed
      against FontPacker before `layoutRun()` can apply it. No bidi, no line breaking by
      width, no ligatures.
- [ ] `TextAttribute::bold` is only a request: there is no bold/italic face selection
      policy yet, because the atlas/font registry that would resolve it does not exist.
- [ ] No 3D rendering features (scene graph, lighting, shadows, post-processing). `GFX/LteMesh.hpp`
      only covers vertex/index storage and draw recording.
- [ ] `MhPipeline`-style wrappers are intentionally absent: Kaldi's `Pipeline`, binding
      layouts and `OpBind*` opcodes already cover that level, and a second wrapper would
      duplicate the HAL.
- [ ] No Kaldi `Swapchain`/`RenderPass` RAII proxies in `LteKld.hpp`; those commands are
      available only through the raw Kaldi opcode vocabulary.
- [ ] The GPU command sequences recorded by `MeshBuilder` and `GuiGeometryBatch` are
      unverified: there is no working Kaldi backend yet.
- [ ] Text and geometry are covered by `testTextPipeline()` and `testGeometryBatches()` in
      the Test app; the font and vector-image formats by `testFontParser()` and
      `testFontRoundTrip()`. Nothing exercises a renderer, because none exists.
- [ ] The CBOR map variants of the FontPacker specification are not implemented.
- [ ] `KerningMap` and the glyph map are `std::unordered_map`s, so exported font files
      are not byte-deterministic. Switch to ordered maps (or sort before writing) if
      reproducible binaries are required.
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
