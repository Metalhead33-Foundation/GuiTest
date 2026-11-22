# Lotte

Lotte is the higher-level graphics and GUI library of MhLib. It sits on top of Kaldi
(the graphics HAL) and Euphemy (assets, images, text-adjacent I/O), and is where
renderer-facing integration belongs so that Euphemy can stay independent of Kaldi.

**Status: partial.** Lotte owns the Kaldi bridge types, the CPU-to-GPU asset upload
scheduler, the FontPacker-compatible font/vector-image formats, the rich-text
pipeline (MML parsing, text layout, glyph quads), the mesh/GUI geometry batches, a
run-time-loaded Kaldi rendering context with a resource factory, a dirty-rectangle
writable texture and a working GUI renderer with anchored (2D and 3D) cursors.
There is no GUI widget system yet; see the TODO list.

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Lotte` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_LOTTE_EXPORT` while building, `MH_LOTTE_API` for consumers |
| Consumer hook | `include(Lotte/Lotte.pri)` adds the include path and links Lotte/Euphemy/Elvavena |
| Dependencies | Elvavena, Euphemy, Kaldi headers, GLM, utfcpp (header-only, `#include <utf8cpp/utf8.h>`), glslang + glslang-default-resource-limits + SPIRV (shader compilation) |

Compiled units: `Asset/LteGpuAssetStreamer.cpp`, `Conf/LteBackendConfig.cpp`,
`GFX/LteKaldiContext.cpp`, `GFX/LtePipeline.cpp`, `GFX/LteResourceFactory.cpp`,
`GFX/LteWriteableTexture2D.cpp`, `Gui/LteCursor.cpp`, `Gui/LteKaldiGuiRenderer.cpp` and
the six `Text/*.cpp` files. The remaining `Gui/` and `GFX/` headers are header-only
(templates plus inline recording helpers).

Kaldi is consumed header-only through `Kaldi.pri`. Do not add `-lKaldi` or `libKaldi`
to Lotte; use the top-level include path.

## Shader compilation

`GFX/LtePipeline.cpp` is the only place in the tree that compiles shaders, and it
uses the **HLSL front-end of glslang** (linked as `-lglslang -lSPIRV
-lglslang-default-resource-limits`). HLSL is the engine's authoring language; the
compiler always emits *Vulkan* SPIR-V 1.0 (`EShClientVulkan`,
`EShTargetVulkan_1_0`, `EShTargetSpv_1_0`) so a single blob serves every backend:
Vulkan consumes it as-is, while the GL/GLES and D3D11/D3D12 backends cross-compile
it with SPIRV-Cross.

**glslang's HLSL front-end is deprecated** (as of April 2026, removed at the next
major version; see [issue #4210](https://github.com/KhronosGroup/glslang/issues/4210)).
The intended replacement is [DXC](https://github.com/microsoft/DirectXShaderCompiler),
whose only notable flaw here is that it targets Shader Model 6.x while this engine
authors HLSL at Shader Model 4/5 level. The migration is **deferred indefinitely**;
[`docs/SHADERS.md`](../docs/SHADERS.md) records the reference `dxc` invocation,
what each flag does, and the acceptance gate for the switch.

## Implemented

| Header | Contents |
|---|---|
| `LotteLib.hpp` | `MH_LOTTE_API` export macro |
| `LteKld.hpp` | Lotte aliases over the Kaldi facade: `CommandBuffer`, `Buffer`, `Framebuffer`, `Fence`, `Image`, `Pipeline`, `ComputePipeline`, `QueryPool`, `Sampler`, `Texture`, plus `OpAllocator`/`HandleAllocator` |
| `Asset/LteGpuAssetStreamer.hpp` / `.cpp` | `GpuAssetStreamer` - bridges Euphemy's CPU `ResourceRegistry` to Kaldi command buffers. Queues buffer/texture uploads and evictions, accounts GPU bytes against a budget, supports priority/pinning/strong references, and records work into move-only `GpuCommandBatch` objects that keep payload memory alive until commit or fail |
| `Text/LteFontFace.hpp` | FontPacker-compatible `WODF` font-face format: SDF/MSDF/MSDFA channel types, distance metrics, glyph metrics, kerning tables, absolute-offset TOC, and both importer (`operator>>`) and exporter (`operator<<`) over `Elv::Io::DataStream` |
| `VectorGraphic/LteStoredVectorImage.hpp` | FontPacker-compatible `WODI` stored-vector-image format with mip levels, encoding flags and stream import/export |
| `Conf/LteBackendConfig.hpp` / `.cpp` | Bridge between `Kld::ConfigProvider` and `Euph::Conf::Configuration`: writes a backend's field defaults into an `.ini`, serializes/parses the text, loads/saves through `Elv::Io::System`, and applies the file back through a `Kld::ConfigReader` (which clamps every value) |
| `GFX/LtePipeline.hpp` / `.cpp` | `compileShaderProgram()` (glslang's HLSL front-end, see [`docs/SHADERS.md`](../docs/SHADERS.md); HLSL or GLSL input, always emits **Vulkan** SPIR-V 1.0 so one blob serves every backend, pluggable `ShaderIncludeResolver`), the owning `ShaderBinary` (SPIR-V words + stage, convertible to `Kld::ShaderBinaryDescriptor`) and `Pipeline` (owns stages + vertex attributes and exposes Kaldi views) |

### `GFX/` and `Gui/` - context, resources, cursor

| Header | Contents |
|---|---|
| `GFX/LteKaldiContext.hpp` / `.cpp` | `KaldiContext` - the promoted `Sandbox/KaldiContext`: `dlopen`s a Kaldi backend, creates its device, owns one `CommandBuffer` plus a `ResourceFactory`, and provides the `beginFrame()` (BeginFrame + clear) / `endFrame()` (EndFrame + Present + swap + submit) protocol |
| `GFX/LteResourceFactory.hpp` / `.cpp` | `ResourceFactory` - the one place Lotte allocates Kaldi handles through `Kld::oop_detail`: `createTexture2D` (raw pixels, `DecodeTarget`, or a shared `DecodeTarget` the factory retains), `createBuffer`, `createSampler`, `createPipeline`, `createImage2D`, `createFramebuffer`, plus `retain()`/`submit()` for the payload lifetime Kaldi requires |
| `GFX/LteWriteableTexture2D.hpp` / `.cpp` | `WriteableTexture2D` - CPU RGBA8 surface mirrored by a Kaldi texture with a dirty rectangle: `setPixel`, `fill`, `blitRgba8` (atlas stamping), `resize`, and `recordUpdate()` which records one `UpdateTexture2D` per dirty row (one for a full-width rectangle) |
| `GFX/LteImageConvert.hpp` | Header-only Euphemy → GUI conversion: `Rgba8Image`, `toRgba8()` for the 8-bit `RGBA/BGRA/RGB/BGR/RG/R` layouts, and `isRgba8Convertible()` |
| `Gui/LteKaldiGuiRenderer.hpp` / `.cpp` | The first concrete `GuiRenderer`: two runtime-compiled HLSL pipelines (coloured `ColouredGuiVertex`, textured `ColouredTexturedGuiVertex`), CPU pixel→clip-space transform, `GuiGeometryBatch` per pipeline, texture-change flush and straight-alpha blending. `beginFrame()` resets the caches, `flush()` records uploads + draws |
| `Gui/LteCursor.hpp` / `.cpp` | `AlphaCoverage` (alpha mask) + `CursorMetadata` (anchor, crawl mode, threshold, explicit hotspot, scale, visibility) + `Cursor`. Nine anchors (four corners, four edge midpoints, center) and five crawl modes (`NoCrawl`, `Diagonal`, `Vertical`, `Horizontal`, `Radial`); a 2D image visual (with atlas UVs) or a 3D model visual with a model-space origin and `modelPlacement()` |

The cursor origin rules, in one place:

| Anchor | Default crawl | What it does |
|---|---|---|
| `TopLeft`, `TopRight`, `BottomLeft`, `BottomRight` | `Diagonal` | walks the anchored diagonal towards the opposite corner until an opaque pixel shows up |
| `Top`, `Bottom` | `Vertical` | walks the middle column inwards |
| `Left`, `Right` | `Horizontal` | walks the middle row inwards |
| `Center` | `Radial` | nearest opaque pixel by Chebyshev ring; an opaque center keeps the exact center point |

A crawled origin is the center of the pixel it found (`x + 0.5`, `y + 0.5`), which is
what makes an arrow tip or a reticle visually stable. `CursorMetadata` is the aggregate
a JSON descriptor will deserialize into later; no JSON is parsed today.

```cpp
// Sandbox/GameSystem.cpp, abbreviated
Lotte::Gfx::KaldiContext context(windowInfo, width, height, settings);
Lotte::Gui::KaldiGuiRenderer gui(context.resources(), { width, height });
Lotte::Gfx::WriteableTexture2D texture =
    Lotte::Gfx::WriteableTexture2D::fromDecodeTarget(context.resources(), decodedPng);
Lotte::Gui::CursorMetadata metadata;
metadata.anchor = Lotte::Gui::CursorAnchor::TopLeft;
metadata.alphaCrawl = true;
const Lotte::Gui::AlphaCoverage coverage =
    Lotte::Gui::AlphaCoverage::fromDecodeTarget(decodedPng);
Lotte::Gui::Cursor cursor({ texture.handle(), { texture.width(), texture.height() } },
                          &coverage, metadata);

context.beginFrame();
gui.beginFrame();
cursor.render(gui, mousePosition);
gui.flush();
context.endFrame();
```

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

- [x] A concrete GUI renderer exists (`Gui/LteKaldiGuiRenderer`) for the 2D primitive
      vocabulary: coloured/textured/gradient quads, triangles and lines, batched per
      pipeline and flushed on texture change.
- [ ] No render graph, material/shader management, camera, 3D scene or post-processing.
      The renderer converts pixels to clip space itself and owns no camera; a 3D
      pipeline still has to be added on top of `KaldiContext`.
- [ ] No GUI widget system. No widgets, layout, input routing or focus handling. The old
      `Widget`/`RootNode`/`TickboxWidget` classes were not carried over; `ElvSignal.hpp`
      provides the signals they used.
- [ ] Cursor metadata is a C++ aggregate (`Lotte::Gui::CursorMetadata`); there is no JSON
      loader yet, and only the 2D image visual is drawn. The 3D model visual produces a
      placement matrix (`Cursor::modelPlacement`) that a 3D renderer must consume.
- [ ] No glyph atlas. `Text/LteFontAtlas.hpp` documents what is missing (decode
      `StoredCharacter::sdf`, pack it into a texture, expose UV rectangles through
      `GlyphAtlasMapping`) and implements the quad emission. `GFX/LteWriteableTexture2D`
      now provides the atlas page (CPU pixels + dirty-rectangle upload) and
      `CursorImageVisual::uv` shows the sub-rectangle convention, but the glyph packer
      itself does not exist. Confirmed constraints from FontPacker: every payload is a
      square `bitmapSize` × `bitmapSize` image because each glyph's metric box is
      stretched onto a square before downsampling, the padding is already baked into the
      payload (`bitmapPadding * bitmapSize / bitmapLogicalSize`), and MSDF payloads are
      stored as RGB888.
- [ ] No text shaping beyond glyph placement: kerning is applied per pair within a run
      (offsets are pixels at the face's reference size, as FontPacker stores them), but
      pairs that straddle a run boundary are not kerned, and there is no bidi, no line
      breaking by width, and no ligature substitution.
- [ ] `TextAttribute::bold` is only a request: there is no bold/italic face selection
      policy yet, because the atlas/font registry that would resolve it does not exist.
- [ ] No 3D rendering features (scene graph, lighting, shadows, post-processing). `GFX/LteMesh.hpp`
      only covers vertex/index storage and draw recording.
- [ ] `MhPipeline`-style wrappers are intentionally absent: Kaldi's `Pipeline`, binding
      layouts and `OpBind*` opcodes already cover that level, and a second wrapper would
      duplicate the HAL.
- [ ] No Kaldi `Swapchain`/`RenderPass` RAII proxies in `LteKld.hpp`; those commands are
      available only through the raw Kaldi opcode vocabulary.
- [ ] The GPU command sequences recorded by `MeshBuilder`, `GuiGeometryBatch` and
      `KaldiGuiRenderer` are only smoke-tested: the Sandbox renders a cursor every frame
      through the Vulkan backend, but nothing asserts the resulting pixels yet.
- [ ] Text and geometry are covered by `testTextPipeline()` and `testGeometryBatches()` in
      `Sandbox`; the font and vector-image formats by `testFontParser()` and
      `testFontRoundTrip()`. The CPU half of the new cursor/texture code is covered by
      `CursorTests` and `WriteableTexture2DTests` in `UnitTests`; the renderer itself has
      no automated frame-level test.
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
