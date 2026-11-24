# **MML** - *Metalhead33's Markup Language* {#doc_mml}

> Salvaged verbatim from `Text/MML.md` on the `NewBeginning` branch. This is the
> language specification; `LteMml.hpp` is the parser, `LteHtmlEntities.hpp` the
> character reference table, and `LteMarkupColour.hpp` the colour grammar.

**MML** - short for **Metalhead33's Markup Language** - is a hybrid of [BBCode](https://en.wikipedia.org/wiki/BBCode) and [HTML](https://en.wikipedia.org/wiki/HTML), or more specifically, a union of a subset of each, with a few unique features.

* Both the HTML/XML-style `<tag>` and BBCode-style `[tag]` are equally valid and accepted, being interchangeable in MML.
    * In practice, `<tag]` and `[tag>` are also valid, but this usage is discouraged. The adoption of a consistent scheme is encouraged instead. Nevertheless, `<` is interchangeable with `[` and`>` is interchangeable with `]`.
* Any and all code within tags - including the name of the tag itself - is case-insensitive, meaning that `tag`, `TAG`, `tAg`, etc. are all equally valid and interchangeable. It is up to the parser to convert all non-lowercase alphabetic characters to their lowercase equivalents before further processing the tag.
    * This also applies to the arguments of tags, e.g. font names and colour names. Thus, care must be taken that all fonts within a font repository are mapped to fully-lowercase names.
* During the parsing of tags, all whitespace is removed.
* Tags that have parameters must delimit the tag name and the list parameters with an equality sign `=`. For multiple-parameter tags, each parameter must be separated by comma `,`.
* [HTML escapes](https://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references) are both supported and encouraged. In fact, they are mandatory for certain special characters, namely `<`, `>`, `[`, `]` and `&`. They are also encouraged for most non-ASCII characters.
* [The full palette of both basic and extended web colours](https://en.wikipedia.org/wiki/Web_colors#HTML_color_names) are supported, as well as hexadecimal RGB(A) colour codes and normalized *(0.00 - 1.00)* decimal RGB(A).
    * Due to the case-insensitive nature of the parser, care must be taken in implementations to map the web colours to fully-lowercase names, as well as support the parsing of hexadecimal numbers with lowercase letters.
    * An example to each usage would be `[color=blue]`, `[color=#00FF0000]` and `[color=1.0, 0.332, 0.75, 1.0]` respecitvely. For the latter two examples, the alpha-component is entirely optional, and defauls to fully opaque, if absent.

### Currently supported tags:

|               |    **Tags**    |                         **Parameters**                        |
|:-------------:|:--------------:|:-------------------------------------------------------------:|
|   Line break  |      br, p     |                              N/A                              |
|    **Bold**   |    b, strong   |                    N/A (weight 700)                           |
|    _Italic_   |      i, em     |                              N/A                              |
|   <ins>Underscore</ins>  |        u       |                              N/A                              |
| ~~Strikethrough~~ | s, del, strike |                              N/A                              |
|   <span style="font-family:Arial">Font name   |      font      |                          A font name</span>                          |
|  <span style="color:blue">Font colour</span>  |  color, colour | A colour name, hexadecimal colour or normalized colour values |
|   <span style="font-size:18px">Font size</span>   |      size      |                          A font size                          |

### Font weights

WODF has no font-weight metadata, so a face's weight comes from its
`FontRepository` registration. An unannotated face is treated as **normal (400)**;
`b`/`strong` are the conventional alias for **bold (700)**. Of 245 regular/bold
font pairs on the development host, 243 used exactly 400 and 700 (mean difference
299.18), which is why those values are the defaults.

MML's boolean `bold` field remains for compatibility, but the effective weight is
the numeric value used by layout and rendering. CSS `font-weight` values are
preserved numerically (100-900), with `bolder` and `lighter` resolved relative to
the parent. The SDF/MSDF/MSDFA shader shifts its sampling threshold by the
difference between the requested weight and the registered face weight; a
regular face rendered at normal weight therefore uses the same threshold as
before.

### Provisionally: `class=` and `id=` (CSS)

A tag may carry `class=` and `id=` parameters, so a stylesheet can target it. Both
spellings are accepted, because MML otherwise removes all whitespace inside a tag
and `<b class=warn>` would collapse into `bclass=warn`:

```
<b class=warning>careful</b>
<b=class=warning,id=loud>loud and careful</b>
[span class=note]
```

Class and id names are lowercased like everything else inside a tag, so write them
lowercase in the stylesheet too. They have no effect unless a stylesheet is attached
(see the CSS integration study below); without one they are ignored and
the built-in tag behaviour applies.

### CSS

CSS support is **optional and partial**; the complete study follows below. In short: a
stylesheet can set `color`, `font-family`, `font-size`, `font-weight`, `font-style` and
`text-decoration` using element, class, id and tree selectors; it cannot do layout,
spacing, backgrounds or floats, because Lotte has no layout engine and MML runs have no
boxes. `font-weight` retains its numeric value (100-900); `bolder`/`lighter` are resolved
against the parent weight. When a stylesheet is attached, the tags below keep their
familiar meaning through a built-in user-agent stylesheet, so `b { font-weight: normal }`
really does undo `<b>`.

## CSS integration study

> Status: **feasible, implemented as an optional module, deliberately narrow.** This
> section records what NetSurf's `libcss` can and cannot do for MML, what the integration
> cost, and what we may and may not promise to document authors. The code lives in
> `Lotte/Text/LteCssStyle.{hpp,cpp}` and is only compiled when
> `pkg-config --exists libcss` succeeds.

### The question

MML (this page) styles text with attribute tags: `<b>`, `<i>`, `<u>`,
`<s>`, `<font=...>`, `<size=...>`, `<color=...>`. That vocabulary maps onto
`Lotte::Text::TextAttributes` - colour, family, size, bold/weight, italic,
underline, strikethrough - and nothing else, because Lotte has no layout engine.

Could `libcss` (`dev-libs/libcss`, NetSurf's CSS parser, version 0.9.2 here) give us
real CSS without over-promising? The short answer: **yes for parsing, the cascade,
selectors and inheritance; no for layout.** Everything that would make CSS feel like
CSS - boxes, margins, alignment, backgrounds, floats - is out of reach until a
layout engine exists. What remains is genuinely useful: a stylesheet can recolour,
resize and emphasise MML documents, and it can do it with real selectors
(`.warning`, `#title`, `p b`) instead of per-tag attributes.

### What libcss actually is

| Layer | Provided by libcss? |
|---|---|
| CSS tokenizer and parser (properties, values, at-rules) | yes |
| Selector matching (type/class/id/attribute, combinators, tree pseudo-classes) | yes |
| Cascade: origin, specificity, `!important`, `@media` filtering | yes |
| Inheritance (`css_computed_style_compose()`) | yes, if the caller supplies the parent chain |
| Unit resolution (`em`, `pt` -> px) | partly - see the units table |
| DOM, boxes, layout, line breaking, painting | **no** |
| Font loading (`@font-face` consumption) | **no** - it only parses the descriptors |

So libcss is a *style resolver*: give it a node tree and it gives you one computed
style per node. Everything else stays ours.

### Verified capabilities

Measured with a probe program and then locked in by
`UnitTests/tests/CssStyleTests.cpp` (12 test cases):

#### Selectors that work

| Selector | Note |
|---|---|
| `b`, `p`, `font` | MML tags become element names |
| `.warning` | needs the provisional `class=` parameter (below) |
| `#title` | needs `id=` |
| `p b`, `p > b`, `b + i`, `b ~ i` | the element stack provides parents and siblings |
| `:first-child`, `:nth-child()`, `:not()`, `:root` | tree information is available |
| `:hover`, `:focus`, `:active`, `:visited`, `:link` | always false - rich text has no interaction or history |

#### Properties that reach the text

| Property | Effect on `TextAttributes` |
|---|---|
| `color` | colour, 8-bit precision (libcss stores `css_color`) |
| `font-weight` | numeric @ref Lotte::Text::FontWeight (100-900); `bold` = 700, `bolder`/`lighter` step relative to the parent, `normal` = 400 |
| `font-style` | `italic` and `oblique` set `italic`, `normal` clears it |
| `text-decoration` | underline and line-through **add** to ancestors (see below) |
| `font-size` | size in pixels, after unit conversion |
| `font-family` | first *named* family, lowercased (MML font repositories use lowercase) |

#### Properties that are parsed and then ignored

`margin`, `padding`, `border`, `background`, `width`, `height`, `position`,
`display`, `float`, `clear`, `overflow`, `line-height`, `text-align`,
`text-indent`, `letter-spacing`, `word-spacing`, `white-space`, `text-transform`,
`vertical-align`, `opacity`, `visibility`, `columns`, `flex`, `grid`, transforms.
This is the honest boundary: they need layout, MML runs have no boxes, and libcss
never computes them into anything we could use. **Do not promise them.**

#### Units

| Unit | Behaviour |
|---|---|
| `px` | as written |
| `em` | resolved by libcss against the composed parent style |
| `pt`, `pc`, `in`, `cm`, `mm`, `q` | converted by our adapter at `CssMetrics::deviceDpi` |
| `%` | relative to the parent size |
| `ex`, `ch` | approximated as `0.5em` (no font metrics available) |
| `rem`, `lh`, `vw`, `vh`, `vmin`, `vmax` | mapped onto the metrics |
| `xx-small` ... `xx-large`, `larger`, `smaller` | resolved by libcss with *its* table (`large` = 1.125 x `medium`, not 1.2) |

`font-size` is the only property where units matter to us; everything else is
length-based and ignored.

### What the integration cost

The plumbing, in the order it was needed:

1. **An element stream from the parser.** `MmlParser` gained an optional
   `MmlElementObserver` (`onElementOpen(name, classes, id)`, `onElementClose(name)`).
   With an observer attached, MML's built-in attribute handlers step aside
   (`ownsElementStyling()`), because a stylesheet must be able to *undo* `<b>`
   (`b { font-weight: normal }`). `br` stays purely structural; `p` is both an
   element and a line break.
2. **A provisional tag grammar.** MML removes all whitespace inside a tag, which
   would turn `<b class=warn>` into `bclass=warn`. The compactor now keeps one space
   when the following token starts with `class=`/`id=`, so both spellings work:
   `<b class=warn>`, `<b=class=warn,id=loud>`, `[span class=note]`. Positional
   arguments (`<font=Arial>`, `<color=1.0,0.332,0.75>`) are untouched.
3. **A node tree.** Each open tag becomes a `CssNode` with interned name, class
   array and id, plus parent/children links so libcss can answer `parent_node`,
   `sibling_node`, `node_count_siblings`, `node_is_root` and friends.
4. **The user-agent stylesheet.** `<b>`/`<i>`/`<u>`/`<s>`/`<code>`/headings keep
   their familiar meaning through a built-in sheet registered as `CSS_ORIGIN_UA`,
   so author rules outrank them. `CssStylesheet::markupDefaults()` exposes it.
5. **Inheritance.** `css_select_style()` returns *specified* styles full of
   `INHERIT` markers; `css_computed_style_compose(parent, child, ...)` turns them
   into resolved values and is also what makes `em` come out in pixels.
6. **The conversion.** `composeAttributes()` reads the six properties above and
   produces a `TextAttributes`; `RichTextProcessor::setCurrentAttributes()` applies
   it atomically, so one element is one run boundary.
7. **Decoration propagation.** `text-decoration` is *not* inherited in CSS, but
   browsers paint an ancestor's line through descendant boxes. The adapter unions
   the decorations along the ancestor chain, so `<u>a<b>b</b></u>` underlines `b`
   too.

#### libcss 0.9.2 landmines (all worked around, all worth remembering)

These cost most of the debugging time and are the reason this module is isolated in
one translation unit:

| Problem | Workaround |
|---|---|
| `computed.h` uses the C `restrict` keyword, so the headers are not C++-clean | `#define restrict __restrict__` around the includes in `LteCssStyle.cpp` only |
| `css_unit_ctx` has a `const` function-pointer member | aggregate-initialise it once; it cannot be assigned |
| `css_stylesheet_create()` answers `CSS_BADPARM` when `params.resolve` is null, even for an in-memory sheet | a pass-through URL resolver that refs the relative URL |
| A partially filled `css_select_handler` segfaults inside `css_select_style()` | all 36 callbacks are implemented |
| `ua_default_for_property()` returning `CSS_PROPERTY_NOT_SET` for pointer-valued properties (`font-family`, `quotes`) segfaults: the `initial_*` helpers feed the hint straight into the property setter | return a generic family / "no quotes" hint instead |
| `node_name`/`node_id`/`node_classes` are *consumed*: libcss may release the reference it is handed | hand out a fresh reference per call (`lwc_string_ref`) and keep the node's own |
| Absolute units are not converted; keyword font sizes use libcss's own scale table | convert in the adapter; mirror libcss's table for the unresolved fallback |
| No `node_data` caching, because we pass null through `set/get_libcss_node_data` | correct but re-selects ancestors for every element; fine for documents |

### What we may promise

**Do** promise:

> MML documents can be styled with a stylesheet: element, class and id selectors,
> descendant/child/sibling combinators, `:first-child`-style tree pseudo-classes,
> `!important`, `@media screen`, and the properties `color`, `font-family`,
> `font-size`, `font-weight`, `font-style` and `text-decoration`. Inheritance and
> `em` sizing behave like CSS. `font-weight` remains numeric; the compatibility
> `bold` flag is true for weights 600 and above.

**Do not** promise:

> Layout, alignment, spacing, backgrounds, borders, floats, inline styles
> (`style="..."`), `@import` of additional sheets, `@font-face` font loading or
> interactive pseudo-classes. They parse (or are ignored) and have no effect.

### Recommendation

Keep it **optional and narrow**:

* optional at build time (`pkg-config --exists libcss`), so no host gains a hard
  dependency and the library still builds on a bare system;
* narrow in properties, because every additional property needs a `TextAttributes`
  field *and* a layout stage that consumes it;
* explicit about the grammar extension, which is the only user-visible MML change.

The natural next steps, in order of value per unit of work:

1. **`text-align` and `line-height` into `TextLayout`.** Both already exist as
   concepts (`Lotte::Text::TextLayout`), so this is the cheapest way to make CSS
   feel real for rich text.
2. **`white-space` and `text-transform`** - cheap, no layout needed; they only
   affect the text buffer before shaping.
3. **An inline `style="..."` MML parameter**, using
   `css_select_style(..., inline_style, ...)`, which libcss already supports.
4. **`@font-face` into the font registry** - libcss parses the descriptors; loading
   is ours, and there is no atlas/registry yet.
5. Revisit `@import` only if documents become multi-file.

If MML ever grows real block layout, this module is the part that survives
unchanged: the cascade and the selector engine are already there, and only the
"convert a computed style to attributes" step would need replacing.

### Trying it

```cpp
#include <Lotte/Text/LteCssStyle.hpp>

const Lotte::Text::CssStylesheet sheet(
	"p { font-size: 14px; color: #223344 }\n"
	".warning { color: #cc3300; font-weight: bold }\n",
	Lotte::Text::CssMetrics{ /* viewport, default size, dpi */ });

const Lotte::Text::TextRunList runs = Lotte::Text::styleMarkup(
	"<p>plain <b class=warning>careful</b></p>", sheet);
```

`styleMarkup()` is the one-call form. For a streaming pipeline, construct a
`CssStyleResolver` and attach it with `MmlParser::setElementObserver()`; the class
documentation has the four-line version.

### Build

`Lotte.pro` enables the module when `pkg-config --exists libcss` succeeds:

```pro
packagesExist(libcss) {
    CONFIG += link_pkgconfig
    PKGCONFIG += libcss
    DEFINES += LTE_WITH_LIBCSS
}
```

Without libcss the header is inert, `LteCssStyle.cpp` compiles to an empty
translation unit and `UnitTests/tests/CssStyleTests.cpp` disappears from the suite -
verified by configuring with `PKG_CONFIG_LIBDIR=/nonexistent`.

## References

- [`../../Lotte/Text/LteMml.hpp`](../../Lotte/Text/LteMml.hpp) — the parser that
  implements this grammar, including its tag tables.
- [`../../Lotte/Text/LteHtmlEntities.hpp`](../../Lotte/Text/LteHtmlEntities.hpp) — the
  character-reference table.
- [`../../Lotte/Text/LteMarkupColour.hpp`](../../Lotte/Text/LteMarkupColour.hpp) — the
  colour grammar.
- [`LteCssStyle.hpp`](../../Lotte/Text/LteCssStyle.hpp) — the optional libcss integration.
- \ref mod_lotte — the module page for the rich-text pipeline as a whole.
