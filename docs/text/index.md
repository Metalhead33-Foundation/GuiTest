# Text and markup {#docs_text_index}

*The MML language specification and its CSS integration study*

Status: index (revision 1 — 2026-11-24). Lotte owns the rich-text pipeline; this
note is the language-level document it is written against. The parser, layout and
renderer are described in the [`Lotte`](../modules/index.md) module page and in the
headers.

| Note | What it covers |
|---|---|
| \subpage doc_mml | **MML — Metalhead33's Markup Language**: the language specification (tag spellings, case rules, parameters, numeric weights) that `Lotte::Text::MmlParser` implements, plus the CSS integration study: what libcss can and cannot do for MML, the resolved attributes, the layout boundary, the libcss 0.9.2 landmines and the optional build switch. |

The CSS study follows the tag vocabulary in the same page, so the two cannot drift
apart.

## References

- [`../index.md`](../index.md) — the documentation tree this folder is a child of.
- [`../../Lotte/Text/MML.md`](../../Lotte/Text/MML.md) — the specification page itself
  (it lives next to the parser, not here, so the two cannot drift apart).
- [`../../Lotte/Text/LteMml.hpp`](../../Lotte/Text/LteMml.hpp) — the parser and its tag
  tables.
- [`../../Lotte/Text/LteCssStyle.hpp`](../../Lotte/Text/LteCssStyle.hpp) — the optional
  libcss integration.
