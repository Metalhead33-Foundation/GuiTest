# Text and markup {#docs_text_index}

*The MML language specification and the CSS study*

Status: index (revision 1 — 2026-11-24). Lotte owns the rich-text pipeline; these two
notes are the language-level documents it is written against. The parser, layout and
renderer are described in the [`Lotte`](../modules/index.md) module page and in the
headers.

| Note | What it covers |
|---|---|
| \subpage doc_mml | **MML — Metalhead33's Markup Language**: the full language specification (tag spellings, case rules, parameters) that `Lotte::Text::MmlParser` implements. Salvaged verbatim from the older tree; it is a specification, not a status report. |
| \subpage doc_mml_css | The `libcss` study: what CSS can and cannot do for MML, the resolved attributes, the layout boundary, the libcss 0.9.2 landmines and the optional build switch. |

Reading order: the specification first, then the stylesheet study — the study assumes
the tag vocabulary the specification defines.

## References

- [`../index.md`](../index.md) — the documentation tree this folder is a child of.
- [`../../Lotte/Text/MML.md`](../../Lotte/Text/MML.md) — the specification page itself
  (it lives next to the parser, not here, so the two cannot drift apart).
- [`../../Lotte/Text/LteMml.hpp`](../../Lotte/Text/LteMml.hpp) — the parser and its tag
  tables.
- [`../../Lotte/Text/LteCssStyle.hpp`](../../Lotte/Text/LteCssStyle.hpp) — the optional
  libcss integration.
