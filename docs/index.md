# docs {#docs_index}

*The long-form notes, the module pages, and the rules they are written under*

Status: index (revision 1 — 2026-11-24). This page is the root of the documentation
tree: three notes live here, `graphics/`, `text/` and `networking/` hold the topical
notes, and `modules/` holds one page per module. The policy behind all of it is
[`DOCUMENTATION.md`](DOCUMENTATION.md).

The notes are the *design reasoning* — the things that do not fit above a declaration.
The headers are the API reference, and `<Module>/README.md` is the status report for
that module (**what exists, what is a TODO**). A note here never restates a module's
implemented/TODO list; it explains why the code is shaped the way it is.

## 1. Start from the task

Each row is a `\subpage` link, which is what nests the target under this page in the
generated tree instead of flattening it. **Adding a note** takes two steps: give its
`# Title` line an explicit id (`# Title {#doc_what_it_is}`) and add one row to its
folder's index. A note missing from its index builds as a *sibling* and fails
`UnitTests '[docs]'` ([`DOCUMENTATION.md`](DOCUMENTATION.md) §11).

| If you are about to… | Read only | It owns |
|---|---|---|
| draw anything through Kaldi, or choose between the raw and the object-oriented API | \subpage doc_kaldi_tutorial | the command stream, the facade, a complete textured triangle, retirement/lifetime/threading, the per-backend differences |
| author, compile or port a shader | \subpage doc_shaders | HLSL → SPIR-V 1.0, how each backend consumes it, the glslang deprecation and the deferred DXC migration |
| generate textures, GUI atlases or a theme file | \subpage doc_procedural | the JSON and enum policies, the noise/Voronoi API, the SDF/material/lighting pipeline, the theme schema |
| write, parse or style MML text, or ask what CSS may promise | \subpage doc_mml | the MML language specification, the CSS integration study, resolved attributes, landmines and what may not be promised |
| touch the networking layer (session, snapshots, RPC, prediction) | \subpage doc_netcode | the Elvavena/Euphemy split, the wire contract, the milestones |
| change the HTTP parser, client or server | \subpage doc_http | the deliberately minimal HTTP layer and its extension points |
| add a poller, a completion wrapper or a streaming buffer | \subpage doc_platform | readiness vs completion, the wakeup seam, the two ring-buffer contracts |
| add or edit a header, a note or a module README | \subpage doc_documentation | the comment policy, the note tree, the checks |
| find out what a module implements | \subpage docs_modules_index | one page per module: status, implemented surface, TODOs |
| ask where a salvaged concept came from | \subpage doc_salvage | the decision log for the older trees and what was deliberately dropped |
| find a graphics note | \subpage docs_graphics_index | the Kaldi tutorial and the shader pipeline |
| find a text note | \subpage docs_text_index | the combined MML specification and CSS integration study |
| find a networking or platform note | \subpage docs_networking_index | netcode, HTTP and the platform I/O wrappers |

## 2. What each document is allowed to claim

A note is one of five kinds, and the kind decides what a sentence in it means. The
distinction matters because a module README's TODO list, a study's verdict and a
contract all read alike.

| Kind | What a sentence in it means | Where it lives |
|---|---|---|
| **Contract / reference** | a statement about shipped behaviour; changing it needs a decision, not a commit | [`DOCUMENTATION.md`](DOCUMENTATION.md), the headers, the "Implemented" halves of the module pages |
| **Guide** | how to use the shipped thing, with worked examples | \subpage doc_kaldi_tutorial, \subpage doc_shaders |
| **Study** | a question answered with evidence; it opens no work, and its verdict may be "boundary" | the CSS section of \subpage doc_mml |
| **Status report** | a dated snapshot of implemented vs TODO for one module | every `<Module>/README.md` (\subpage docs_modules_index) |
| **History** | what happened and why, not what is true today | \subpage doc_salvage |

Two consequences worth stating, because both are easy to get wrong:

- **"Implemented" does not imply "tested".** The module pages keep the two apart; the
  test suite ([`UnitTests`](modules/index.md)) is where coverage is recorded
  ([`DOCUMENTATION.md`](DOCUMENTATION.md) §3).
- **A TODO on a module page is a gap, not a promise.** It says the described surface
  does not exist yet. If the design deliberately excludes it, the module page says so
  in the same list rather than leaving a checkbox that will never be ticked.

## 3. References

- [`DOCUMENTATION.md`](DOCUMENTATION.md) — the comment policy, the note tree and the
  checks that keep this page complete.
- [`modules/index.md`](modules/index.md) — the module pages this tree points at.
- [`../README.md`](../README.md) — the project overview and status table.
- [`Doxyfile`](../Doxyfile) — the renderer's configuration.
