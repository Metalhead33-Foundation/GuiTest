# Procedural textures, noise and themes {#doc_procedural}

*Why the noise, the Voronoi, the GUI forge and the theme schema are shaped the way they are*

Status: policy note (revision 1 — 2026-11-26). It owns the JSON rule, the "enums over
strings" rule, what was salvaged from the old trees and what was deliberately dropped.
The API reference lives in the headers ([`EuphNoise.hpp`](../Euphemy/Util/EuphNoise.hpp),
`Lotte/Procedural/*`,
[`LteThemeDefinition.hpp`](../Lotte/Gui/LteThemeDefinition.hpp)); the module pages list
what exists and what is still a TODO.

## 1. The two hard rules

**JSON is an application concern.** The libraries describe data as C++ structs and
enums. Only the Sandbox and the UnitTests link `nlohmann_json`, and only their loaders
(`Sandbox/ThemeJson.cpp`) see a parsed document. Consequences:

- no library of ours gains a parser dependency, so a future swap of `nlohmann_json` for
  something else (or for a binary format) touches two files, not twenty;
- a theme can be produced by a generator, a test, an editor or a hand-written file with
  no JSON in the picture at all;
- the same rule keeps the algorithm layer pure: `buildGuiAtlas()` takes settings, not a
  document.

**Enums where the set of values is fixed.** An element is a `ThemeElement`, a text role
a `ThemeTextRole`, an atlas source a `ThemeAtlasSource`, an atlas rectangle a
`GuiAtlasPart`. A string exists only at the file boundary, and each enum owns one name
table (`themeElementName`/`parseThemeElement`, `guiAtlasPartName`/`parseGuiAtlasPart`,
...). A loader calls the table; it never contains a `std::map<std::string, ...>` of its
own vocabulary. The loader accepts the colour strings MML already accepts
(`Lotte::Text::parseMarkupColour`), so a theme and a text block speak the same colour
language.

## 2. What was salvaged

| From | Kept | Dropped, and why |
|---|---|---|
| `GuiTest-legacy2/MhLib/Util/MhNoise.{hpp,cpp}` | white, value, Perlin, bilinear Perlin, simplex, Voronoi, sum-of-sines | the `srand()`/`rand()` overloads (one process-wide seed made a "seeded" result unreproducible), the `glm` lattice type (a pair of integers does not need a geometry library) |
| `ProceduralGenerationTest/Voronoi.{hpp,cpp}` | the tile traversal with border wrapping, both distance metrics - folded into `EuphNoise.hpp` because a Voronoi distance field and a Voronoi tile map are the same algorithm | `std::pair<int,int>` and the misspelled `Distance::Eucledian`; the global `rand()` default assigner |
| `ui_forge.cpp` (Claude) | the SDF painter, the height-to-normal lighting pass, the material set, the element recipes | the built-in PNG writer (Euphemy encodes), the on-disk element dump, the hard-coded `atlas.json` layout |
| `gemini.md` | the material-mask idea, the per-state widget recipes | its own atlas writer and its fixed 1024x768 layout |
| `grok.md` | the flat bevel vocabulary used by the simple recipes | the per-element hard-coded rectangles |

Nothing here writes a file. A generated atlas is a `Lotte::Gfx::Rgba8Image`;
`Lotte::Procedural::toDecodeTarget()` hands it to any Euphemy encoder (PNG, TGA, DDS),
and `WriteableTexture2D::fromRgba8()` uploads the same bytes without a round-trip.

## 3. The pipeline

```text
GuiAtlasSettings (seed, sizes)
  -> element recipes        paint(shape, material) per layer       (SurfaceCanvas)
  -> shade()                height -> normal -> diffuse/AO/spec/shadow
  -> shelf packing          image + enum-keyed rect manifest       (GuiAtlas)
  -> ThemeDefinition        atlas alias + pixel rect per element/state
  -> DefinitionTheme        skin lookup by element + state mask      (Theme)
  -> GuiScene + ScriptButton  theme skin -> textured quad every frame
```

Each arrow is a separate type, which is what lets the pieces be tested without a GPU:
the UnitTests build an atlas, check the manifest, and resolve skins through a resolver
that returns a stand-in handle.

### The element inventory

The four reference documents were cross-checked against the builder, and the union of
their types is what `GuiAtlasPart` now names. The panel/button/slot/slider/checkbox/
parchment core came from `ui_forge.cpp` first; these were the gaps and are now generated
too:

| Type | Parts | Where the reference draws it |
|---|---|---|
| Radio glyph and its selected state | `radio_empty`, `radio_checked` | `ui_forge.cpp` (`radio_small_on`, `radio_tiny_on`) |
| Scroll/spin arrows + dropdown triangle | `arrow_up`, `arrow_down`, `tri_down` | `ui_forge.cpp`; all three notes ("arrows from polygon paths") |
| Vertical scroll bar | `scroll_track`, `scroll_thumb` | `ui_forge.cpp` (`scroll_panel`, `pill_*`); `grok.md` scrollbars |
| Tabs | `tab_active`, `tab_inactive` | `ui_forge.cpp` (`arrow_tab`) |
| Wide text/portrait frame | `frame_text` | `ui_forge.cpp` (`frame_text`); `gemini.md` inset frame |
| Inventory grid | `slot_group` | `ui_forge.cpp` (`panel_*slot*`, `grid_8x2`); `gemini.md` `drawSlotGroup`; `grok.md` |
| Window edge | `border_strip` | `ui_forge.cpp` (`border_strip`) |
| Gem icon tile | `gem_tile` | `ui_forge.cpp` (`tile_star`, `tile_gem`) |

Deliberate non-additions: the reference sheets draw ten grey button faces and five pill
shades, which are *instances* of one recipe - the atlas ships the four button states and
one thumb, and a theme tints them. Orientation (vertical slider, reversed arrow) is a
rotation of the existing part, not a part of its own, and the nine-slice margins a theme
already stores are what stretch a tab or a border.

## 4. The two Voronoi outputs

A tile Voronoi has two useful answers per pixel, and the API returns both from one
traversal (`VoronoiSample`):

- **`firstDistance`/`secondDistance`** - the classic Worley field. `F1` is the familiar
  cellular ramp; `F2 - F1` is what draws cracks and cell borders (the stone material
  uses it through `GuiField`).
- **`cell`** - the index of the nearest point. That is the answer a *tile map* needs:
  index a palette, an atlas or a per-cell random value with it and the cells become
  discrete tiles instead of a height field.

Both honour the border wrap, so a field tiles seamlessly. Tie-breaking follows the
search order; a caller that needs a canonical winner must break ties itself (the tests
document this by accepting any nearest cell).

## 5. Noise: one provider, gradient first

Every generator a recipe needs lives in `EuphNoise.hpp`; the procedural stack composes
them and never rolls its own. That rule is not tidiness - it was a bug report.

**The parchment hatched.** A diagonal grating appeared across the sheet. There were two
independent causes, and both are worth remembering:

1. **The SDF, not the noise.** `roundedBox` clamped the sideways offset before the
   interior term, so every point inside the box reported exactly `-radius` instead of its
   real (negative) distance. `outline(box, 7)` therefore evaluated negative over the whole
   canvas, and `SurfaceCanvas::paint` blended the frame's rope material - a
   `sin((x + y) * 0.82)` weave - over the face at full coverage. The fix keeps the offset
   unclamped; the interior now reports a real distance, which also restores the bevel ramp
   the `paint` depth term was silently flattening. The tests pin both halves: the exact
   interior distance, and that `outline()` covers a frame, not 100% of the canvas.
2. **Gratings, not texture.** The fibre layer summed a horizontal and a vertical
   anisotropic `fbm` at ~2 px period. Two axis-aligned gratings near the sampling limit
   interfere, and the beat reads as a 45 degree comb. An anisotropic *fbm* is worse than
   one grating because each octave lands at its own angle. The recipe now uses one
   single-octave stretched lattice for the fibre, plus an isotropic tooth.

**Gradient noise is the default.** `latticeNoise2D` blends corner *values*, so its
iso-contours run along the cell diagonals, each cell has a flat plateau, and a coarse
lattice reads as diamonds - fine for a cheap field, wrong for a surface. `latticeGradient2D`
blends corner *gradients*: no plateau, no preferred direction. Its fbm
(`latticeGradientFbm2D`) **rotates every octave**, because translate-only octaves keep all
lattices parallel and their ridges line up into hatching. `latticeFbm2D` keeps its
historical translate-only walk at `rotation = 0` and offers the same escape hatch.

Practical rules that follow from the above:

- pick the generator by what the field *is*: gradient noise for a face, value noise when
  the diamonds are the effect, Voronoi when cells are the effect;
- keep the finest octave at or above ~2 px per cycle; anything faster is aliasing, not
  detail;
- sample a stretched field in a rotated frame, and only ever with **one** octave.

## 6. What the theme schema owns

`ThemeDefinition` is the data form of `LteTheme.hpp`'s interface: atlases (file or
procedural), per-element skins as `(stateMask, skin)` variants, metrics, and one text
style per role. A skin stores a **pixel rectangle** because that is what an artist
edits; `DefinitionTheme` converts it to normalized UVs using the atlas size the
resolver reports. State variants are resolved by "most specific mask fully contained in
the requested state", so a theme that ignores hover costs one entry, not eight.

A generated atlas is packed by the builder, so its rectangles are only known after it
ran. A theme therefore names the **part** (`"part": "button_hover"`) instead of a
rectangle: the loader resolves the name through `parseGuiAtlasPart`, and the atlas
answers "where did this part land?" through `ThemeAtlasBinding::partRect`, which the
scene builds from the manifest it kept when it uploaded the atlas. A hand-authored
atlas keeps working with an explicit `rect`, so the same schema covers both.

The scene is what makes this live: `GuiScene::addAtlas()` uploads a generated image
and remembers its manifest, `GuiScene::resolveAtlas()` is the resolver the theme is
constructed with, and `ScriptButton` queries `Theme::skin()` for its *current* state
every frame. A theme installed after the script created its widgets still dresses
them, and clearing it puts them back on their own colours.

### CSS is the theme's second language

A theme document is JSON, but JSON has no vocabulary for typography: a font stack, a
size in `pt` or `em`, a colour, a padding and a border width are exactly what CSS already
names, resolves and cascade-resolves. So the theme borrows them (`Lotte/Gui/LteThemeCss`):

| CSS | Becomes |
|---|---|
| `font-family`, `font-size`, `font-weight`, `font-style` | `ThemeTextStyleDefinition` |
| `color`, `text-decoration` | text colour, underline, strikethrough |
| `padding` | `ThemeMetrics::padding` (the horizontal/vertical averages) |
| `border-width` | `ThemeMetrics::borderWidth` **and** the atlas bevel |
| `background-color` | the procedural atlas face tint |

The last two rows are the point: the same block that styles a widget's text also styles
the *generated skin* the widget wears, because `GuiAtlasSettings` takes a `bevelWidth` and
a `faceTint` that `GuiAtlas` applies while painting. Change the CSS, and the atlas is
carved differently on the next build.

Two rules make that safe to use:

- **The document's explicit values are a CSS rule of their own.** The block is prefixed
  with a `* { ... }` rule generated from the JSON fields, so a block that mentions only
  `color` cannot reset the 32 px / 700 weight the document asked for (libcss's initial
  values would otherwise win, which is exactly the bug this design avoids).
- **Resolve once, never per frame.** `DefinitionTheme` resolves every block in its
  constructor and stores the computed style; a widget asking for its text style gets a
  plain struct, and a broken block is reported by `validate()` instead of throwing at
  draw time. Without `LTE_WITH_LIBCSS` the block is reported and ignored, so a theme
  document stays usable in a build that dropped the dependency.

## 7. The GPU plan

Every algorithmic piece is written as "per-pixel pure function of position": SDFs,
materials, the lighting pass. Those are exactly what a compute shader evaluates, so the
CPU implementations are the reference images the shaders will be checked against, and
the tests are the fixture. What does *not* move to the GPU is the packing and the theme
resolution: those are layout decisions that run once per theme.

## 8. References

- [`Euphemy/Util/EuphNoise.hpp`](../Euphemy/Util/EuphNoise.hpp) - the noise API and the
  degenerate-input rules.
- [`Lotte/Procedural/LteSurfaceCanvas.hpp`](../Lotte/Procedural/LteSurfaceCanvas.hpp) -
  the painter and the lighting pass.
- [`Lotte/Gui/LteThemeDefinition.hpp`](../Lotte/Gui/LteThemeDefinition.hpp) - the theme
  data form and `DefinitionTheme`.
- [`DOCUMENTATION.md`](DOCUMENTATION.md) - the comment and note-tree policy this note
  follows.
