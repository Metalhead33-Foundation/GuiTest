# Documentation policy

This file describes how MhLib is documented, what "done" means for a documentation
change, and how the rendered reference is produced. It is the policy behind `Doxyfile`
and the module READMEs.

## 1. Purpose

Documentation has two jobs in this repository:

1. **Tell a reader what the code is**: the headers are the API reference.
2. **Tell a reader what exists and what is missing**: the module READMEs are the status
   report, with explicit implemented/TODO lists.

A change that adds or removes a feature is not complete until both layers agree.

## 2. Where documentation lives

| Location | Role |
|---|---|
| `README.md` | Project overview, module status table, build and documentation entry points |
| `<Module>/README.md` | Purpose, build/consumption facts, implemented surface, TODO list, known gaps |
| Header files | API and design reference, in Qt-style Doxygen comments |
| `docs/*.md` | Cross-cutting policy and design notes (this file is the first one) |
| `Doxyfile` | The single configuration that renders headers and Markdown notes |

`extern/` is a submodule and is never documented by this project. Generated output goes
to `build/doxygen/`, never into the source tree.

## 3. Header comments

Headers are written in Doxygen's Qt dialect. The tree uses `/** ... */` and `///` for
most blocks and the Qt-style `//!` / `/*!` forms where they read better; both are
accepted by the same configuration (`QT_AUTOBRIEF = YES`).
- Start every file with a `@file` block: file name, one-sentence `@brief`, and a short
  paragraph explaining why the file exists and what contract it defines.
- Document a declaration with `@brief`, then `@param` for every parameter, `@tparam`
  for every template parameter, and `@return` when the function returns a value.
- Use `@ref`, `@section`, `@name`/`///@{` grouping, and `@note`/`@warning` where they
  add information. Do not restate the signature in prose.
- Document lifetime, ownership, threading and error semantics explicitly. For Kaldi in
  particular, pointer lifetime and deferred destruction are part of the contract, not an
  implementation detail.

### 3.1 Documenting existing and undocumented declarations

The tree predates a strict "every declaration is documented" policy. `Doxyfile` therefore
sets:

- `EXTRACT_ALL = YES` and `HIDE_UNDOC_MEMBERS = NO`, so every declaration still appears
  in the reference;
- `WARN_IF_UNDOCUMENTED = NO`, so a missing block is not a build warning;
- `WARN_NO_PARAMDOC = YES`, so a block that *does* document a function must document all
  of its parameters.

That split is deliberate. Pay down missing comments incrementally when touching a file;
do not mass-generate empty `@brief` lines just to silence a counter. When you add a new
public declaration, document it immediately.

## 4. Module READMEs

Every module has a README with the same shape:

1. One paragraph of purpose.
2. A **Status** line using the vocabulary in section 5.
3. Build and consumption facts: qmake target, export macro, dependency list, consumer
   `.pri` hook.
4. An **Implemented** section, usually a table mapping files/headers to what they
   contain.
5. A **TODOs and known gaps** section, as markdown checkboxes, with enough context that
   a future contributor can start without re-reading the whole module.

The root `README.md` carries a compact status table that links to each module. It must
stay consistent with the module READMEs.

## 5. Status vocabulary

Use these words consistently:

| Status | Meaning |
|---|---|
| **Implemented** | The described surface exists, compiles and is the intended implementation. Tests are mentioned separately. |
| **Partial** | The public surface and some behavior exist, but the module does not yet deliver its full advertised purpose. |
| **Stub** | The project builds and exports its symbols, but the behavior is a placeholder (for example a backend whose `Device` cannot execute commands). |
| **Planned** | Not started; only the folder/project placeholder exists. |

"Implemented" does not imply "tested". The Tests column/paragraph is where that is
recorded.

## 6. Rendering and checking the reference

```sh
mkdir -p build
doxygen Doxyfile 2>&1 | tee build/doxygen.log
```

The output is `build/doxygen/html/index.html`. Useful checks:

- `doxygen` must exit successfully and produce `build/doxygen/html/index.html`.
- The log should be reviewed. `Doxyfile` sets `WARN_AS_ERROR = NO`, so warnings are a
  work list rather than a gate; CI turns each warning into a run annotation.
- The important warning classes are broken commands, unmatched `@param` lists and
  malformed blocks. A missing block is not a warning (section 3.1).

The GitHub Actions workflow (`.github/workflows/doxygen.yml`) renders the same site and
publishes it; it requires the `extern/doxygen-awesome-css` submodule.

## 7. Generated output and the shadow build tree

Everything generated stays under `build/`:

- Doxygen: `build/doxygen/` (HTML, no LaTeX/XML/man output).
- qmake object files, Makefiles and libraries: `build/Desktop-*/...`.
- Logs, temporary notes and review artifacts: `build/`.

Never commit generated documentation, Makefiles, object files or libraries to the source
tree. `.gitignore` covers the current paths, but the rule is the reason, not the file.

## 8. Maintenance workflow

When changing a module:

1. Update the header comments for the API you touched.
2. Update that module's `## Implemented` and `## TODOs and known gaps` lists.
3. If the module-level status changed, update the table in the root `README.md`.
4. Run `doxygen Doxyfile` and check the log for new warnings.
5. If the change affects build instructions, update `README.md`/module README and
   `AGENTS.md` together.

## 9. Note pages and Doxygen navigation

Markdown files listed in `Doxyfile`'s `INPUT` become pages. Doxygen nests a page only
when another page names it with `\subpage`. Keep the note tree shallow:

- `README.md` is the main page (`USE_MDFILE_AS_MAINPAGE = README.md`).
- Add cross-cutting notes under `docs/` and link them from this file or the root README.
- Prefer a short paragraph plus a table over a deeply nested hierarchy of pages.
