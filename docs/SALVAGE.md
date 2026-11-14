# Branches and the salvage from them

`AssetStreamingMkII` (this branch) is a rewrite. Two older trees hold work that is
still worth keeping:

| Tree | Branch | Role |
|---|---|---|
| `/home/legacy/programok/System/GuiTest-legacy` | `master` | The original MhLib monolith: `GL/`, `Renderer/`, `Text/`, `Widget/`, `System/`, `MhLib/`. |
| `/home/legacy/programok/System/GuiTest-legacy2` | `NewBeginning` | A partial rewrite with `GFX/`, `GUI/`, `MhLib/Text/`, `JS/`. |

This file is the decision log for what was carried over. The rule applied
throughout: **salvage concepts, not implementations.** Where an implementation did
not transfer cleanly, the header documents what the replacement must do and why the
old code could not be reused; where only data transferred (tables, DSP kernels), it
was copied verbatim.

## Deliberately not salvaged

| Area | Reason |
|---|---|
| JavaScript runtime (`JS/*`) | Out of scope for the library. |
| Object-oriented OpenGL wrappers (`GL/*`, `GFX/GL/*`, `NativeGfxApi/*`) | Kaldi replaces them with a data-oriented HAL. Backends are deferred. |
| Native GPU API backends (`Kaldi*` implementations) | Deferred; only the Kaldi contract and facade are used. |
| Software renderer (`Renderer/Software/*`) | Superseded; its algorithms are not needed by the GUI/text work. |
| FreeType font pipeline (`Text/Font.cpp` rasterization, `FontRepository`) | Lotte uses preprocessed FontPacker faces (`Lotte/Text/LteFontFace.hpp`) and does not link FreeType. |
| Texture atlas (`MhHardwareAcceleratedFont`) | No atlas exists yet; `Lotte/Text/LteFontAtlas.hpp` records the contract it must satisfy. |
| `System/*` (application/game systems, SDL audio output, FPS counter) | Application layer, not library. |
| Third-party `GUI/sigslot.h` | Replaced by `Elvavena/Util/ElvSignal.hpp` (see below). |

## Salvaged into `Haruka`

| Old source | New location | Notes |
|---|---|---|
| `MhLib/Media/AdvancedAudio/MhAudioEffect.hpp` | `Haruka/FX/HrkAudioEffect.hpp` | `Effect` contract. One virtual call per block; the layout requirement is a query (`getRequiredInterleaving()`) instead of a bare boolean. |
| `MhLib/Media/AdvancedAudio/FX/SampleLevelEffect.*` | `Haruka/FX/HrkSampleLevelEffect.hpp` | Was one **virtual call per sample**; now a template over an inlinable functor. |
| `MhLib/Media/AdvancedAudio/FX/MhClampEffect.*` | `Haruka/FX/HrkClampEffect.hpp` | Algorithm unchanged; the `maximumLevel = 0` division-by-zero default is fixed. |
| `MhLib/Media/AdvancedAudio/FX/MhCrushEffect.*` | `Haruka/FX/HrkCrushEffect.hpp` | Algorithm unchanged; `maxLevel` renamed to `steps` because that is what it is. |
| `MhLib/Media/AdvancedAudio/MhAudioEffectSlot.*` | `Haruka/FX/HrkEffectSlot.hpp` / `.cpp` | Rewritten as a `ProxyPlayable`. `std::deque` → contiguous `std::pmr::vector`; cached layout requirement; cascade dry/wet semantics; no infinite pull loop; both interleaved and planar output. |
| `MhLib/Media/AdvancedAudio/FX/MhConvolver.*` + `fftconvolver::IrBuffer` | `Haruka/FX/HrkConvolver.hpp` / `.cpp` + `HrkImpulseResponse.hpp` | Uses Euphemy's `FFTConvolver` wrapper; the IR type is Haruka's own. Missing IRs now raise instead of dereferencing null. |
| `MhLib/Media/AdvancedAudio/FX/MhImpulseGenerator.*` | `Haruka/FX/HrkImpulseGenerator.hpp` / `.cpp` | Same Blackman-windowed-sinc design; `M_PI` → `std::numbers::pi`; degenerate arguments are rejected instead of producing zero-length kernels. |

## Salvaged into `Lotte/Text`

| Old source | New location | Notes |
|---|---|---|
| `Text/MML.md` | `Lotte/Text/MML.md` | Verbatim specification. |
| `Text/MmlParser.*` (parser and tag tables) | `Lotte/Text/LteMml.hpp` / `.cpp` | Same tag tables and aliases. Stray `>` and `]` are no longer swallowed; unknown references are preserved. |
| `Text/MmlParser.cpp` (`NamedEscapes` table) | `Lotte/Text/LteHtmlEntities.cpp` | 2,124 entries copied verbatim into a `std::map<std::string_view, std::string_view>`. `#x` now means hexadecimal, `#` means decimal. |
| `MhLib/Util/ColourHelper.*` | `Lotte/Text/LteMarkupColour.cpp` | Hex/component/named colour parsing; the 300-entry colour table is replaced by Euphemy's `getStandardColour`. |
| `Text/RichTextProcessor.*` | `Lotte/Text/LteRichText.hpp` / `.cpp` | Same mutators, manipulators and `operator<<` chain. No font repository: runs carry a family name and size. Fixed: every attribute change flushes, `ChangeFont` no longer captures by reference. |
| `Text/RtProcessorStack.*` | `Lotte/Text/LteMarkupStack.hpp` / `.cpp` | The `pop*` depth logic was inverted in the original and is fixed. |
| `Text/Font.cpp` `Font::renderText` / `renderTextBlocks` | `Lotte/Text/LteTextLayout.hpp` / `.cpp` | The layout algorithm (bearings, measured line height, italic shear, decorations) without atlas or draw calls; `y` grows downwards, coordinates are pixels, and the pen sits on the baseline. Glyphs are placed from the *metric* box (`metricWidth`/`metricHeight`/`horiBearing*`), because FontPacker stretches that box onto the square SDF and leaves the raw bitmap fields empty. Kerning is applied per pair (units confirmed against FontPacker: pixels at the face's reference size). |
| `Text/Font.hpp` `TextRenderState`, `TextBlockUtf*` | `Lotte/Text/LteTextLayout.hpp`, `Lotte/Text/LteTextAttributes.hpp` | Cursor state split from per-run attributes; `reciprocalSize` and the per-block `Font*` dropped. |
| `std::wstring_convert`/`codecvt` usage in `RichTextProcessor` | `Lotte/Text/LteUtf.hpp` | Replaced by [utfcpp](https://github.com/nemtrif/utfcpp) (header-only system dependency) plus a sanitizing wrapper: undecodable bytes become U+FFFD instead of throwing. |
| `Renderer/Shared/GuiVertexTypes.hpp`, `GFX/Advanced/*GuiRenderer.hpp` (vertex structs) | `Lotte/Gui/LteGuiVertex.hpp` | Descriptive member names, function-local static Kaldi descriptors (header-only). |
| `GFX/Advanced/MhHardwareAcceleratedFont.*` (concept) | `Lotte/Text/LteFontAtlas.hpp` | Atlas packing and UV mapping documented; `GlyphAtlasMapping` plus `appendGlyphQuads()` are implemented. |

## Salvaged into `Lotte` (graphics)

| Old source | New location | Notes |
|---|---|---|
| `MhLib/Media/GFX/MhMesh.hpp` | `Lotte/GFX/LteMesh.hpp` | `MeshView` (handles + layout + counts + ranges) replaces the abstract mesh classes; `MeshBuilder` owns the buffers and records uploads. |
| `GFX/Advanced/MhHardwareAcceleratedRenderCache.hpp` | `Lotte/Gui/LteGuiRenderCache.hpp` | Four cache variants collapsed into one `GuiGeometryBatch`; owned `Lotte::Buffer` proxies instead of raw handles; explicit `Range` instead of a single `primitiveCount`. |
| `GUI/MhGuiRenderer.hpp`, `GFX/Advanced/MhHardwareAcceleratedGuiRenderer.*` | `Lotte/Gui/LteGuiRenderer.hpp` | Primitive vocabulary and batching policy kept; the implementation is deliberately absent and documented as a gap. `uintptr_t texturePtr` → `GuiTextureBinding`. |
| `MhLib/Media/GFX/MhPipeline.hpp` | *not salvaged* | Too low-level: Kaldi already has `Pipeline`, binding layouts and `OpBind*` opcodes. Recorded as a non-goal instead of adding a wrapper that would duplicate the HAL. |
| `Renderer/Shared/GuiRenderer.hpp` (`GuiTest-legacy`) | *not salvaged* | Cruder than its `NewBeginning` equivalent, which is itself only partly reusable. |

## Salvaged into `Elvavena`

| Old source | New location | Notes |
|---|---|---|
| `GUI/sigslot.h` (third-party) | `Elvavena/Util/ElvSignal.hpp` | `Signal<Args...>`, `Connection`, `ScopedConnection`. Same two ideas (fan-out plus lifetime-scoped connections) without the third-party header; emission walks a snapshot, so a slot may disconnect during emission. |

## Consequences worth remembering

- Haruka now links Euphemy and Elvavena (`Haruka.pro`), because it exposes Euphemy's
  resampler, sound-file and convolution wrappers. `Project.pro` mirrors that in
  `Haruka.depends`.
- The Test app links Haruka and exercises the new code paths: `testAudioEffects`,
  `testTextPipeline`, `testSignals` and `testGeometryBatches`.
- Anything marked `@warning` in a header is a contract without a backend: the Kaldi
  draw paths in `LteMesh.hpp` and `LteGuiRenderCache.hpp` are recorded but cannot be
  validated until a backend exists.
