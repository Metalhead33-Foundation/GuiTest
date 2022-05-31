# **MML** - *Metalhead33's Markup Language*

**MML** - short for **Metalhead33's Markup Language** - is a hybrid of [BBCode](https://en.wikipedia.org/wiki/BBCode) and [HTML](https://en.wikipedia.org/wiki/HTML), or more specifically, a union of a subset of each, with a few unique features.

* Both the HTML/XML-style `<tag>` and BBCode-style `[tag]` are equally valid and accepted, being interchangeable in MML.
    * In practice, `<tag]` and `[tag>` are also valid, but this usage is discouraged. The adoption is a consistent scheme is encouraged instead. Nevertheless, `<` is interchangeable with `[` and`>` is interchangeable with `]`.
* Any and all code within tags - including the name of the tag itself - is case-insensitive, meaning that `tag`, `TAG`, `tAg`, etc. are all equally valid and interchangeable. It is up to the parser to convert all non-lowercase alphabetic characters to their lowercase equivalents before further processing the tag.
    * This also applies to the arguments of tags, e.g. font names and colour names. Thus, care must be taken that all fonts within a font repository are mapped to fully-lowercase names.
* During the parsing of tags, all whitespace is removed.
* Tags that have parameters must delimit the tag name and the list parameters with an equality sign `=`. For multiple-parameter tags, each parameter must be separated by comma `,`.
* [HTML escapes](https://en.wikipedia.org/wiki/List_of_XML_and_HTML_character_entity_references) are both supported and encouraged. In fact, they are mandatory for certain special characters, namely `<`, `>`, `[`, `]` and `&`. They are also encouraged for most non-ASCII characters.
* [The full palette of both basic and extended web colours](https://en.wikipedia.org/wiki/Web_colors#HTML_color_names) are supported, as well as hexadecimal RGB(A) colour codes and normalized *(0.00 - 1.00)* decimal RGB(A).
    * Due to the case-insensitive nature of the parser, care must be taken in implementations to map the web colours to fully-lowercase names, as well as support the parsing of hexadecimal numbers with lowercase letters.

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

Currently, CSS is unsupported, and there are no intentions to support it in the foreseeable future.
