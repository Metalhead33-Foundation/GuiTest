# Modules {#docs_modules_index}

*One page per module: what it implements, what is still a TODO*

Status: index (revision 1 — 2026-11-24). Each module has exactly one document —
`<Module>/README.md` — and that file is also its Doxygen page. It is the source of
truth for **what exists and what is missing**; the headers are the API reference and
[`../index.md`](../index.md) is where the design reasoning lives.

The status words are fixed ([`../DOCUMENTATION.md`](../DOCUMENTATION.md) §3):
**Implemented**, **Partial**, **Stub**, **Planned**. "Implemented" does not imply
"tested" — coverage is recorded on the [`UnitTests`](../../UnitTests/README.md) page and
in each module's own TODO list.

## 1. The libraries

| Module | Status | What it owns |
|---|---|---|
| \subpage mod_elvavena | Implemented | The foundational library: the device abstraction, containers/allocators, serialization primitives, scope guards, dynamic libraries, the event loop and the transport-agnostic networking protocol. |
| \subpage mod_euphemy | Implemented, small gaps | The media and platform layer: concrete devices, image/audio codecs, compression and crypto proxies, the INI configuration format, optional PMR arenas and the CPU asset runtime. |
| \subpage mod_haruka | Partial | The pull-based audio graph: mixers, panners, resamplers, streamers and the dry/wet effect chain. No output device yet. |
| \subpage mod_kaldi | Implemented (contract + facade) | The graphics HAL/RHI: the 98-opcode command vocabulary, HAL 1.0–1.2, the header-only recording facade, and the C ABI the backends implement. |
| \subpage mod_lotte | Partial | The graphics/GUI layer above Kaldi: shader compilation, GPU asset streaming, the FontPacker font/vector formats, the rich-text pipeline, geometry batches and the GUI base hierarchy. |

## 2. The graphics backends

| Module | Status | What it owns |
|---|---|---|
| \subpage mod_kaldigl | Implemented | The OpenGL 4.3 core backend: EGL/GLX context, capability probe and the complete command executor. |
| \subpage mod_kaldigles | Implemented | The OpenGL ES 3.2 backend: EGL context, capability probe and the ES-specific answers of the shared executor. |
| \subpage mod_kaldivk | Implemented | The Vulkan backend: frames in flight, descriptor/staging rings, explicit swapchains and VMA-backed memory. |
| \subpage mod_kaldid3d11 | Stub | The Direct3D 11 backend: the C ABI and device/context ownership exist; `processCommandQueue()` does not. |
| \subpage mod_kaldid3d12 | Stub | The Direct3D 12 backend: the C ABI and device/context ownership exist; `processCommandQueue()` does not. |
| \subpage mod_kaldiglcommon | Implemented (shared sources) | Shared implementation of the GL family: context layer, format tables, SPIR-V → GLSL, capability probe and the command executor. No library. |
| \subpage mod_kaldid3dcommon | Implemented (shared sources) | Shared implementation of the D3D family: the DXGI format table and SPIR-V → HLSL. No library. |

## 3. The harnesses

| Module | Status | What it owns |
|---|---|---|
| \subpage mod_unittests | Implemented | The Catch2 suite: round-trips, format tables, netcode, the HAL facade, the text pipeline, and the documentation-tree checks. The automated gate. |
| \subpage mod_sandbox | Implemented (scratch) | The manual playground: hardcoded paths and work-in-progress scenarios. Not part of the gate. |

## 4. How these pages are built

Doxygen 1.18 renders a subdirectory `README.md` as *directory documentation*, which
cannot be the target of a `\subpage`. Each module README therefore opens with one
`\page mod_<module> <Title>` line, which makes the same file a page with a stable id —
exactly once, with no duplicated content. The rule and its rationale are in
[`../DOCUMENTATION.md`](../DOCUMENTATION.md) §11.

**Adding a module** takes three steps: create `<Module>/README.md` in the shape of
[`../DOCUMENTATION.md`](../DOCUMENTATION.md) §3, put the `\page` line at its top, and
add one row to the tables above. The `[docs]` test fails until all three are done.

## 5. References

- [`../index.md`](../index.md) — the documentation tree this folder is a child of.
- [`../DOCUMENTATION.md`](../DOCUMENTATION.md) — the module-README shape, the status
  vocabulary and the `\page` rule.
- [`../../README.md`](../../README.md) — the project overview and the compact status
  table.
- [`../../Project.pro`](../../Project.pro) — the module list as the build sees it.
