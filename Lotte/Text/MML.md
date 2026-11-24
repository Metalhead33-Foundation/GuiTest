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
|    **Bold**   |    b, strong   |                              N/A                              |
|    _Italic_   |      i, em     |                              N/A                              |
|   <ins>Underscore</ins>  |        u       |                              N/A                              |
| ~~Strikethrough~~ | s, del, strike |                              N/A                              |
|   <span style="font-family:Arial">Font name   |      font      |                          A font name</span>                          |
|  <span style="color:blue">Font colour</span>  |  color, colour | A colour name, hexadecimal colour or normalized colour values |
|   <span style="font-size:18px">Font size</span>   |      size      |                          A font size                          |

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
(see [`docs/text/MML_CSS.md`](../../docs/text/MML_CSS.md)); without one they are ignored and
the built-in tag behaviour applies.

### CSS

CSS support is **optional and partial**, and the honest scope is documented in
[`docs/text/MML_CSS.md`](../../docs/text/MML_CSS.md). In short: a stylesheet can set `color`,
`font-family`, `font-size`, `font-weight`, `font-style` and `text-decoration` using
element, class, id and tree selectors; it cannot do layout, spacing, backgrounds or
floats, because Lotte has no layout engine and MML runs have no boxes. When a
stylesheet is attached, the tags below keep their familiar meaning through a
built-in user-agent stylesheet, so `b { font-weight: normal }` really does undo
`<b>`.

## References

- [`../../Lotte/Text/LteMml.hpp`](../../Lotte/Text/LteMml.hpp) — the parser that
  implements this grammar, including its tag tables.
- [`../../Lotte/Text/LteHtmlEntities.hpp`](../../Lotte/Text/LteHtmlEntities.hpp) — the
  character-reference table.
- [`../../Lotte/Text/LteMarkupColour.hpp`](../../Lotte/Text/LteMarkupColour.hpp) — the
  colour grammar.
- \ref doc_mml_css — the stylesheet study and the optional libcss module.
- \ref mod_lotte — the module page for the rich-text pipeline as a whole.

