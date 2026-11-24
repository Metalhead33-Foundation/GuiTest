# Graphics {#docs_graphics_index}

*The Kaldi walkthrough and the shader pipeline*

Status: index (revision 1 — 2026-11-24). Two notes cover the graphics stack: one is a
hands-on guide to the HAL, the other is the shading-language policy. The API itself is
in the headers and in the [`Kaldi`](../modules/index.md) module pages; these notes hold
the reasoning a header comment cannot carry.

| Note | What it covers |
|---|---|
| \subpage doc_kaldi_tutorial | The raw `GfxOp` command stream and the object-oriented facade side by side, a complete textured triangle, the retirement/lifetime/threading contracts and the per-backend differences. **Start here** before writing a renderer. |
| \subpage doc_shaders | HLSL authored at Shader Model 4/5 level, compiled to SPIR-V 1.0 for Vulkan 1.0 and cross-compiled by SPIRV-Cross for the other backends; the `glslang` deprecation and the deferred DXC migration. |

The module pages these notes describe:

| Module | Role |
|---|---|
| \ref mod_kaldi | The HAL contract: opcode vocabulary, capabilities, status codes, recording facade |
| \ref mod_kaldigl, \ref mod_kaldigles | The OpenGL/OpenGL ES backends and their shared executor |
| \ref mod_kaldivk | The Vulkan backend |
| \ref mod_kaldid3d11, \ref mod_kaldid3d12 | The Direct3D backends (device ownership; no executor yet) |
| \ref mod_lotte | The renderer, text pipeline and GUI built on the HAL |

## References

- [`../index.md`](../index.md) — the documentation tree this folder is a child of.
- [`../../Kaldi/KldCommandBuffer.hpp`](../../Kaldi/KldCommandBuffer.hpp) — the facade,
  documented in place; the tutorial is its walkthrough.
- [`../../Lotte/GFX/LtePipeline.cpp`](../../Lotte/GFX/LtePipeline.cpp) — the only place
  in the tree that compiles shaders.
- [`../../UnitTests/tests/KaldiCommandBufferTests.cpp`](../../UnitTests/tests/KaldiCommandBufferTests.cpp)
  — the facade's behaviour as executable spec.
