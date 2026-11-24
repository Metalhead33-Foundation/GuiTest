# Documentation {#doc_documentation}

*The Comment Policy in the Headers, the Module Pages, and the Note Tree*

Status: policy note (revision 1 — 2026-11-24). Every header outside `extern/` and
`build/` is covered by §4–§7; the Markdown note tree is part of the generated site with
five index pages (`docs/`, `graphics/`, `text/`, `networking/`, `modules/`), stable page
ids, and §11 carries the rule that **References is always the last section**. Revision
history lives in git.
Audience: anyone adding or editing a header, a note or a module README, or reviewing a
change that touches one.

The headers are not a private implementation detail that happens to be visible. They are
the API reference: the Kaldi contract, the backend seam, the device ownership rules, the
lifetime and threading rules all live in a comment above the declaration they govern, not
in a separate prose document. A comment a reader cannot *find* is a comment that will be
duplicated, contradicted or simply lost. This note is why the headers carry Doxygen
markup, what that markup has to look like, and how the Markdown notes are nested into one
tree. It contains no engine behaviour.

## 1. Document map

| Section | Question it answers |
|---|---|
| §2 | What documentation lives where, and which layer owns which fact? |
| §3 | What shape does a module README have, and what do the status words mean? |
| §4 | Which comment forms are documentation, and which are not? |
| §5 | How is a function, a template, a class or an alias documented? |
| §6 | What does every header start with? |
| §7 | Which prose breaks a Doxygen run, and how is it written instead? |
| §8 | How do I check? |
| §9 | Why is this necessary rather than nice to have? |
| §10 | What is deliberately left undone? |
| §11 | How are the pages arranged into a tree instead of a flat list? |
| §12 | What is the workflow for a change? |
| §13 | References |

## 2. Where documentation lives

| Location | Role | Owns |
|---|---|---|
| `README.md` | The main page: overview, documentation map, repository layout, status table, build and test recipes | The one-paragraph answer to "what is this project" |
| `<Module>/README.md` | A module page: purpose, build/consumption facts, implemented surface, TODOs and known gaps | **What exists and what is missing** in that module |
| Header files | The API and design reference, in Doxygen comments | The contract of every declaration |
| `docs/*.md` | Cross-cutting notes: policy, guides, studies, history | The reasoning a header comment cannot carry |
| `docs/<folder>/index.md` | A node in the note tree | The list of its children (§11) |
| `docs/modules/index.md` | The module node | One `\subpage` row per module page |
| `Doxyfile` | The single configuration that renders the headers and the Markdown notes | Nothing else may render them |
| `<Module>/<file>.md` | A note that must live next to the code it describes (for example `Lotte/Text/MML.md`) | The same role as `docs/*.md`; it is listed by a `docs/` index |

`extern/` is a submodule and is never documented by this project: reformatting it would
make the next submodule update a merge. `build/` is generated output. `AGENTS.md` is
contributor tooling, deliberately outside `INPUT`, so notes name it as inline code
instead of linking it (§7).

There are three layers and they do not restate each other. The **headers** say what a
declaration promises. The **module page** says what exists and what is a TODO. The
**notes** say why the thing is shaped the way it is, and which alternatives were refused.
A feature change is not complete until the first two agree; a design change is not
complete until the third says so.

## 3. Module READMEs and the status vocabulary

Every module has a README with the same shape:

1. One `\page mod_<module> <Title>` line, then a one-paragraph purpose. The page line is
   what makes the file a Doxygen page at all (§11 explains why it cannot be omitted).
2. A **Status** line using the vocabulary below.
3. Build and consumption facts: qmake target, export macro, dependency list, and the
   consumer `.pri` hook.
4. An **Implemented** section, usually a table mapping headers to what they contain.
5. A **TODOs and known gaps** section, as Markdown checkboxes, with enough context that
   a future contributor can start without re-reading the whole module.

The status words are fixed, so a reader can compare two modules at a glance:

| Status | Meaning |
|---|---|
| **Implemented** | The described surface exists, compiles and is the intended implementation. Tests are mentioned separately. |
| **Partial** | The public surface and some behaviour exist, but the module does not yet deliver its full advertised purpose. |
| **Stub** | The project builds and exports its symbols, but the behaviour is a placeholder (for example a backend whose `Device` cannot execute commands). |
| **Planned** | Not started; only the folder/project placeholder exists. |

**"Implemented" does not imply "tested".** Coverage is recorded in the module's own TODO
list and in the `UnitTests` page, never inferred from the status word. The root
`README.md` carries a compact table that summarises these pages; it must stay consistent
with them, and `docs/modules/index.md` is the same list in Doxygen form.

## 4. The header rule

**Every comment in a `.h`/`.hpp` file is a Doxygen comment**, with the three deliberate
exceptions listed below. The rule covers the public headers, the backends, the shared
implementation halves and the test harness — the whole tree except `extern/` (a
submodule) and `build/` (generated).

The corollary matters as much as the rule: a comment sits **where the thing it describes
is declared**. Prose about `Device` goes above `class Device`, not above the config
struct that happens to precede it; a paragraph about resource retirement goes above the
proxy that owns the handle, not in a file-level preamble.

This tree writes Doxygen's Javadoc and Qt forms side by side, and both are enabled by the
same configuration:

| Form | Use |
|---|---|
| `/** … */` | The dominant form: a doc block before a declaration |
| `/// text` | A single-line doc block before a declaration |
| `//! text` | A Qt-style doc block before a declaration |
| `//!< text` | A Qt-style doc block *after* a declaration — enumerators, struct fields |
| `/*! … */`, `/*!< … */` | The Qt-style multi-line forms, used where they read better |
| `@brief`, `@param`, `@return`, `@tparam`, `@throws`, `@note`, `@warning`, `@sa`, `@par`, `@section`, `@ref` | Inside a block; the backslash spelling is accepted for every one of them |

Keep `@brief` explicit and one sentence long, on the first line of the block: it is what
every index shows. `QT_AUTOBRIEF = YES` is set so a first sentence still becomes the brief
if the command is ever omitted, but the tree does not rely on that.

Three kinds of comment are **not** documentation. Doxygen never renders them, and writing
a marker there does active harm — `#endif //! GUARD` attaches the text to whatever was
documented last:

| Not documentation | Why it stays plain |
|---|---|
| `#endif // ELVTRAP_HPP`, `#if … // NOLINT` | An annotation on a preprocessor line |
| `} // namespace Elv`, `} // extern "C"` | An annotation on a closing brace |
| `bool /*async*/ handle` | A name-suppression idiom *inside* a declaration |

`.cpp` files keep their `//` comments. A file block (`@file`) at the top of a
translation unit is welcome and several already have one, but the rule and its checks are
about the surface a reader is handed, which is the header.


## 5. Function and template documentation

A function block carries `@brief`, exactly one `@param` per declared parameter, and a
`@return` when the function returns a value. Anything templated carries exactly one
`@tparam` per template parameter — a function template, a class template, an alias
template, a variable template and a template member alike.

```cpp
/**
 * @brief Uploads every frame of a decode target as a mip chain of a 2D texture.
 * @tparam Pixel Pixel type of the destination texel.
 * @param texture Name of an existing texture object (the caller need not bind it).
 * @param source Decoded image whose frames become mip levels 0..N-1.
 * @param wantedMipmaps Mip levels the caller wants; missing ones are generated.
 * @return The levels that were uploaded, and whether mipmaps were generated.
 * @throws std::runtime_error On unsupported formats or inconsistent frame data.
 */
template <class Pixel>
TextureUploadInfo uploadTexture2D(GLuint texture, const Image2D<Pixel>& source,
                                  uint8_t wantedMipmaps = 0);
```

The commands have rules, and each of them is a warning Doxygen emits rather than a matter
of taste:

| Command | Rule |
|---|---|
| `@brief` | One sentence, on the first line of the block; it is what every index shows |
| `@param name` | **Exactly one per declared parameter, spelled exactly as declared**, in declaration order. Never document a parameter that does not exist, and never write two `@param` sections for one name |
| `@return` | Only when the function returns a value. Never on `void`, a constructor, a destructor or an overloaded operator that returns nothing |
| `@tparam name` | Exactly one per template parameter, spelled exactly as declared — for functions, classes, aliases and variable templates alike |
| `@throws` | The exception types the function can propagate; needs whitespace after the command |
| `@note`, `@warning` | A real constraint a caller must honour, not a restatement of the signature |

Three failure modes were found in this tree when the rule was written down, and all three
are fixed; they are listed because they are what a generated or copy-pasted block tends to
do:

- `@return` on a `void` handler, a constructor or a destructor (Doxygen reports
  "found documented return type … that does not return anything");
- `@param` names guessed from the prose, or a parameter left out because the signature
  wrapped onto a second line;
- a doc block inserted **inside** a macro body or a multi-line expression, which documents
  a token-paste helper instead of the declaration it was meant for.

`WARN_NO_PARAMDOC = YES` in the `Doxyfile` states the intent, and `WARN_IF_DOC_ERROR = YES`
catches a `@return` on a `void` function or a `@param` that names no parameter in every
run. What the default run does **not** measure is a documented function with a missing
`@param`, because `EXTRACT_ALL = YES` switches that warning off; §8's strict pass is the
command that measures it.

## 6. Every header starts with a file block

Each header opens with a block that says what the file is, because the generated "Files"
page is the front door of the reference:

```cpp
/**
 * @file ElvIoDevice.hpp
 * @brief The device abstraction shared by every I/O path.
 *
 * This header is the seam between the protocol-level code in Elvavena and the
 * concrete file, socket and memory devices in Euphemy: it defines `Device`,
 * `DeviceLike`, the buffer proxies, and the `pDevice`/`sDevice`/`uDevice`
 * ownership aliases. Nothing here opens an OS handle.
 */
```

The `@brief` is one line in the noun-phrase register the rest of the tree uses ("The
device abstraction shared by every I/O path.", not "This file contains…"). A constraint
that applies to the whole file belongs in this block as a `@note`; a paragraph written
above the first declaration instead becomes that declaration's brief, which is how a
header ends up describing `class Buffer` as "this file is part of the public API".


## 7. Prose that breaks a Doxygen run

Running Doxygen is the only honest way to find these. The list below is what real runs on
this tree reported, with the rewrite that fixed each one.

| Written | What Doxygen does | Written instead |
|---|---|---|
| A code span whose last character is an asterisk, for example a wildcard prefix | Opens the teletype tag and never closes it ("found `</tt>` tag without matching `<tt>`") | End the span before the asterisk and leave the asterisk in prose |
| A period inside a code span in the **first sentence** | `QT_AUTOBRIEF` cuts the brief at that period, inside the span | Make the first sentence end on its own, then start a new sentence with the span |
| A bare `#` in prose, for example `#prefix` or a `docs/NOTE.md#anchor` link | `#name` is a link request ("explicit link request to 'live' could not be resolved") | Keep it in a code span, or escape the hash with a backslash |
| An unescaped `<` that a reader means literally | Read as markup ("unsupported xml/html tag") | Escalate it with a backslash when it is not inside a code span |
| `@ref name()` — a call signature as the reference target | The target is the literal `name()`, which no declaration is called, so the link cannot resolve | Write `@ref name` without the parentheses, or the whole signature as inline code |
| An unqualified `@ref` in a **file block** to a symbol in a namespace | File blocks sit at global scope, so the name is not found | Qualify it, or make it inline code |
| A `/* … */` block with the body flush left | Renders, but the project style is an indented block | `/**` … two-space body … `*/` |

The next rows only bite once Markdown files are rendered (they were found when the notes
joined the site), and each one is silent in a Markdown preview — which is why they are
listed rather than left to intuition:

| Written | What Doxygen does | Written instead |
|---|---|---|
| A **code span split across two lines**, or any line with an odd number of backticks | Opens a verbatim block and searches for a closing backtick to the end of the file ("Reached end of file while still searching closing backtick of a verbatim block"), swallowing the rest of the page | Keep each span on one line, and keep the backtick count on a line even. `docs/graphics/KALDI_TUTORIAL.md` ended with a stray fence and lost its last section to exactly this |
| A member-style name in plain prose | The hash starts a link request, exactly as in a header comment | Keep it inside a code span |
| A bare angle-bracket placeholder in prose, even inside a code span | "Unsupported xml/html tag found" | Escape the angle bracket with a backslash |
| A **relative link to a file outside the `Doxyfile`'s `INPUT`** | "unable to resolve reference … for `\ref` command": the target is not a page | Name it as inline code (`AGENTS.md`) instead of linking it |
| An anchor link whose heading carries no explicit label | The anchor is not a Doxygen label, so the reference cannot be resolved | Put a label on the heading (`{#environment-notes}`) and link to that |
| A link to `<Module>/README.md` | It resolves to the *directory* reference, not to the module's page, because Doxygen uses a subdirectory README as directory documentation (§11) | Link the page id instead: `@ref mod_<module>`, or list it as a `\subpage` row in `docs/modules/index.md` |
| A `\subpage` row read in a Markdown preview | GitHub shows the command literally; only the generated site turns it into a link | Accept it — this is the price of a nested tree, exactly as WasmJit pays for its `{#id}` anchors |
| An image referenced from Markdown, for example a mascot next to the module README | The `<img>` tag is emitted with the path as written and **nothing is copied** unless Doxygen can find the file through `IMAGE_PATH`; the site then shows a broken image and the run stays warning-free | `IMAGE_PATH = .` in the `Doxyfile` (already set). Doxygen copies the files flat by base name, so keep image names unique |

**Rule of thumb:** the generated site is the arbiter, not the Markdown preview.
`doxygen Doxyfile` must report nothing; a warning is a doc bug with a rewrite, which is
why the render is the first of the checks in §8.


## 8. How to check

Four commands, all cheap:

```sh
# 1. Render the reference. Any output on stderr is a doc bug: the tree is clean.
doxygen Doxyfile
$BROWSER build/doxygen/html/index.html

# 2. Enforce the note tree and the module pages (part of the default suite).
cd build/Desktop-Debug
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests "[docs]"

# 3. The strict pass: measures the two things the default render cannot, because
#    EXTRACT_ALL switches them off — a declaration with no block at all, and a
#    documented function whose parameters are not all documented (§5).
sed -e 's/^EXTRACT_ALL .*/EXTRACT_ALL = NO/' \
    -e 's/^WARN_IF_UNDOCUMENTED .*/WARN_IF_UNDOCUMENTED = YES/' \
    -e 's|^OUTPUT_DIRECTORY .*|OUTPUT_DIRECTORY = build/doxygen-strict|' \
    Doxyfile | doxygen -

# 4. Read a header the way a newcomer will: the top of the file, then the brief of
#    its first declaration.
```

The output goes to `build/doxygen/` (and `build/doxygen-strict/` for the strict pass) —
never into the source tree, like every other artefact (`AGENTS.md` requires it). The
strict pass is a *work list*, not a gate: at revision 1 (2026-11-24) it reports **3,293
warnings**, of which 2,492 are declarations with no block at all and 756 are partial
`@param` gaps in a documented declaration; §10 classifies them. The **default** render is
the gate, and it is warning-free.

The HTML theme is read from `extern/doxygen-awesome-css`, a submodule of this tree: the
`Doxyfile` names its stylesheets and scripts by path, so a checkout that skipped
`git submodule update --init --recursive` renders unstyled and reports that those files
cannot be read.

The GitHub Actions workflow (`.github/workflows/doxygen.yml`) runs the same render into
`build/doxygen/html`, uploads it as an artifact and publishes it to GitHub Pages. It turns
each warning into a run annotation instead of a red build, because `WARN_AS_ERROR = NO` is
the committed policy — the render is expected to be clean, and a warning that appears is
visible on the run rather than silently ignored.

## 9. Why this is necessary

1. **The headers are the API reference.** There is no hand-written reference manual and
   there should not be one: it would be a second copy of the same facts, updated by
   whoever remembers. `docs/` carries the *design reasoning*; the headers carry the
   *contract*. If the contract is not rendered, a consumer's only option is to read the
   source, and a review can only check what a reviewer happens to open.
2. **A prose-only rule drifts, and this tree has the receipts.** `Kaldi/KldCommandBuffer.hpp`
   carries the lifetime and deferred-destruction contract in comments because a header is
   where a caller looks; the tutorial in `docs/graphics/KALDI_TUTORIAL.md` is the prose
   half. When those two drifted apart, nothing failed — comments cannot fail a build. The
   `[docs]` test is the same lesson applied *before* the drift, and the render gate is the
   other half of it.
3. **It makes the missing documentation visible.** With `EXTRACT_ALL` and a clean run,
   the reference shows every declaration, documented or not. The gaps are then a
   deliberate list (§10) instead of an invisible part of the tree.
4. **It is what the tools already assume.** The backend C ABI, the Kaldi command
   vocabulary and the plugin-facing headers are read by people and by tooling; a header
   that renders is a header a backend author can navigate.


## 10. What is deliberately not done

- **Not every declaration has a block yet.** The default render is clean, but the strict
  pass (§8, step 3) reports **2,492 declarations with no block** across the tree at
  revision 1 (2026-11-24), plus 756 partial `@param` gaps in declarations that do have
  one. Both are work lists, not gates: pay them down when touching a file, and document
  every new public declaration immediately. Do not mass-generate empty `@brief` lines to
  drive the counter down.
- **A doc block must say something.** A block of the form "`@brief Performs the
  operation. @param x Input value.`" is *worse than no block*: it renders, it counts as
  documented, and it states nothing (or, on a `void` handler, states something false).
  Such blocks were removed where they existed; do not reintroduce them.
- **Uniform family tails are acceptable.** The small accessor families (`swapBuffers()`,
  `apiName()`, the `x()`/`y()`/`width()` groups) may carry one short sentence repeated
  across the family, because the declaration already names the thing and the sentence is
  true of every member. Per-member prose is not required where there is nothing distinct
  to say.
- **`extern/` is untouched.** It is a submodule; reformatting it would make the next
  submodule update a merge.
- **`.cpp` files are untouched by the rule.** A file block at the top of one is welcome,
  but the enforced surface is the headers.
- **The `[docs]` test checks structure, not prose quality.** It cannot tell whether a
  sentence is true or useful; that remains a review question.

## 11. The note tree

The site renders the Markdown notes as pages. Doxygen nests a page only when another page
names it with `\subpage`; without that every note is a *sibling* at the top level, which is
what a flat tree is. The hierarchy is therefore built by five index pages:

| Index page | Id | Children |
|---|---|---|
| [`index.md`](index.md) | `docs_index` | The three notes in `docs/` (`DOCUMENTATION.md`, `PLATFORM.md`, `SALVAGE.md`) plus the four folder nodes |
| [`graphics/index.md`](graphics/index.md) | `docs_graphics_index` | The Kaldi tutorial and the shader pipeline note |
| [`text/index.md`](text/index.md) | `docs_text_index` | The combined MML specification and CSS integration study (which lives in `Lotte/Text/`) |
| [`networking/index.md`](networking/index.md) | `docs_networking_index` | The netcode note, the HTTP note, and the platform I/O note (which lives in `docs/`) |
| [`modules/index.md`](modules/index.md) | `docs_modules_index` | One page per module: the fourteen `<Module>/README.md` files |

A note declares its own id on the title line, and its folder's index lists that id in a
`\subpage` row:

```md
# Using Kaldi {#doc_kaldi_tutorial}                    <- docs/graphics/KALDI_TUTORIAL.md
| \subpage doc_kaldi_tutorial | ... |                  <- docs/graphics/index.md
```

which produces:

```text
MhLib                     (README.md, the main page)
├── Documentation map, Repository layout, …   (the README's own sections)
└── docs                   (docs/index.md)
    ├── Documentation, Platform I/O, Salvage …  (notes in docs/)
    ├── graphics           (docs/graphics/index.md)
    │   ├── Using Kaldi
    │   └── Shader pipeline
    ├── modules            (docs/modules/index.md)
    │   ├── Elvavena, Euphemy, Haruka, Kaldi, …
    │   └── UnitTests, Sandbox
    ├── networking         (docs/networking/index.md)
    │   ├── Netcode foundation
    │   ├── Minimal HTTP
    │   └── Platform I/O wrappers
    └── text               (docs/text/index.md)
        └── MML (specification + CSS study)
```

**Adding a note** takes two steps: create the file with `{#doc_what_it_is}` appended to
its `# Title` line, and add one row to its folder's index. **Adding a folder** takes one
more: `<folder>/index.md` with its own `{#docs_<folder>_index}` id, a `\subpage` row in
`docs/index.md`, and a row in the `[docs]` test's list of index pages.


### 11.1 Not every page can be a README.md

Doxygen 1.18 renders a subdirectory `README.md` as *directory documentation*, not as a
Markdown page, even when the file is listed explicitly in `INPUT`. A `\subpage` target in
it therefore cannot resolve, and a relative link to it lands on the (contentless)
directory reference. `SHOW_DIRECTORIES`, `\dir` documentation and renaming the directory
do not change that.

A module README stays where it is, and pays for its page with one line at the top:

```md
\page mod_kaldi Kaldi

# Kaldi
```

That line makes the same file a page with a stable id, renders it exactly once (the
directory page becomes empty), and lets `docs/modules/index.md` nest it normally. The
line is visible in a Markdown preview, exactly as a `{#id}` anchor is; the generated site
is the arbiter. The `[docs]` test requires the line, requires the id to match the folder
name, and requires the page to be listed in `docs/modules/index.md` — so a new module
cannot be added without becoming part of the tree.

Because of the directory-reference resolution, a **link to a module page is `@ref
mod_<module>`, not a link to its README file**. In `docs/` notes that is the rule; inside
a module README, a link to a sibling module's file is a GitHub convenience, and the
Doxygen site reaches the same page through the `modules` node.

### 11.2 House rules for a note

- **Keep the title short.** The `# Title` is what the sidebar and the page tree show, so
  it stays a name, not a paragraph. Put the qualifying detail on the italic subtitle line
  right below it. A title longer than roughly sixty characters is a table-of-contents
  defect.
- **Front matter keeps one revision.** The `Status:` line names the current revision and
  the current state; earlier revision notes are git history, not a growing preface. Do
  not append a `Revision N — …` chain to the next edit: replace the previous summary.
- **References is always the last section.** In every note, in every folder. A reference
  list that is not last is a navigation defect, because a reader who reaches the final
  heading and still has no links has to scroll back to find them. An appendix or a
  post-mortem goes **before** it — never after — and the note's §1 map says so.
- **A note owns one question.** If two notes answer the same question for different
  slices, the older slice is *history* and belongs in a historical record, not in a new
  file. The MML page is the deliberate exception: its CSS integration study is part of
  the same language contract, not a second claim about a different question.
- **The folder index is part of the note.** A note that is not listed in its folder's
  index is invisible in the tree; that is why the check below exists.

### 11.3 Enforced, not remembered

`UnitTests "[docs]"` fails when:

- a note in `docs/`, `docs/graphics/`, `docs/text/` or `docs/networking/` has no `{#id}`
  on its title line;
- a note is missing from its folder's index — the silent failure, because it would still
  build, as a sibling of the whole sub-tree;
- an index names an id that no page declares (a typo Doxygen would drop without
  complaint);
- a module `README.md` has no `\page mod_<folder> …` line, or its id does not match its
  folder name, or the page is not listed in `docs/modules/index.md`;
- two pages declare the same id;
- a note under `docs/` puts any `##` section after its `## References`.

The test reads the files, so it also documents the conventions: its source is the shortest
complete description of the tree.


## 12. Maintenance workflow

When changing a module:

1. Update the header comments for the API you touched, including every `@param` and
   `@tparam` (§5).
2. Update that module's `## Implemented` and `## TODOs and known gaps` lists.
3. If the module-level status changed, update `docs/modules/index.md` and the compact
   table in the root `README.md`.
4. If the change alters a contract that a note explains, update the note in the same
   change — a contract that exists only in the next commit is a contract nobody can read.
5. If the change affects build instructions, update `README.md`, the module README and
   `AGENTS.md` together.
6. Run `doxygen Doxyfile` and check the log for new warnings, then run
   `UnitTests "[docs]"`. Both are cheap; neither is optional.

When adding a note, follow §11: create the file with its `{#id}`, add the row to its
folder index, and keep `## References` last. When adding a module, follow §11.1: the
`\page` line, the module page in the shape of §3, and the row in
`docs/modules/index.md`.

## 13. References

- `Doxyfile` — the renderer's configuration: `QT_AUTOBRIEF`, `WARN_NO_PARAMDOC`, the
  `build/doxygen` output directory and the `INPUT` list that decides which Markdown file
  is a page.
- [`index.md`](index.md) — the note tree this policy describes.
- [`modules/index.md`](modules/index.md) — the module pages and the `\page` convention.
- `UnitTests/tests/DocumentationTests.cpp` — the `[docs]` check, and therefore the
  executable description of the tree.
- `.github/workflows/doxygen.yml` — the CI render and the Pages deployment.
- `AGENTS.md` — the build rules (absolute `make`/`qmake6`, artefacts under `build/`) that
  the render command follows too.

