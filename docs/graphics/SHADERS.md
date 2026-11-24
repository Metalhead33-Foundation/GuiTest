# Shader pipeline: HLSL → SPIR-V → every backend {#doc_shaders}

This page records the shading-language policy: what the engine authors shaders in,
which compiler turns them into SPIR-V, how each backend consumes that SPIR-V, and
the state of the `glslang` → `DXC` question. It is documentation only; no build
step changes because of it.

## The pipeline

| Stage | Where | What happens |
|---|---|---|
| Authoring | game / engine code | HLSL written with **Shader Model 4** (Direct3D 10) and **Shader Model 5 / 5.1** (Direct3D 11 / 12) semantics in mind |
| Compilation | `Lotte::Gfx::compileShaderProgram()` at run time, `UnitTests/UnitTests.pro` at build time | HLSL → **SPIR-V 1.0** targeting Vulkan 1.0 (`EShClientVulkan`, `EShTargetVulkan_1_0`, `EShTargetSpv_1_0`) |
| Vulkan | `KaldiVK` | consumes the SPIR-V words as-is (`Kld::ShaderBinaryDescriptor`), no cross-compiler |
| OpenGL / OpenGL ES | `KaldiGLCommon/GlSpirvShader` | SPIR-V → GLSL through SPIRV-Cross |
| Direct3D 11 / 12 | `KaldiD3DCommon/D3DSPirvShader` | SPIR-V → HLSL through SPIRV-Cross (Shader Model 5.0 / 5.1) |

The consequences that matter when authoring a shader:

- One blob per stage serves every backend, so a shader is never written twice.
- **SPIR-V 1.0 / Vulkan 1.0 is the floor.** Newer SPIR-V versions, Vulkan 1.1+
  features or SPIR-V extensions are outside the contract even if a driver would
  accept them.
- The SPIR-V must stay lowerable by SPIRV-Cross to GLSL 4.x / ES 3.2 *and* to
  Shader Model 5.0/5.1. That rules out Shader Model 6 constructs — bindless and
  descriptor-indexing resources, wave intrinsics, 16-bit types, dynamic resources
  — regardless of which compiler produced the blob.
- Resources are bound explicitly, `[[vk::binding(slot, set)]]`, with one
  descriptor set per Kaldi binding kind. The mapping is documented in
  \ref mod_kaldivk.

## Compiler status

### glslang — what the repository uses today

> "The HLSL front-end is deprecated as of April 2026 and will be removed at the
> next major version of glslang. See
> [issue #4210](https://github.com/KhronosGroup/glslang/issues/4210) for rationale
> and migration guidance." — [KhronosGroup/glslang](https://github.com/KhronosGroup/glslang)

The deprecation covers only the **HLSL** front-end; the GLSL front-end and the
SPIR-V generator are unaffected. This repository currently uses it for both
halves of shader work:

- run time: `Lotte/GFX/LtePipeline.cpp` (`compileShaderProgram()`);
- build time: `UnitTests/UnitTests.pro`, which compiles `UnitTests/shaders/` to
  `.spv` with the `GLSLANG=` knob (default `/usr/bin/glslang`).

It emits SPIR-V 1.0 for Vulkan 1.0, which is exactly the contract above.

### DXC — the intended replacement

[DXC](https://github.com/microsoft/DirectXShaderCompiler) is the go-to option.
Its one notable flaw for this engine is that it targets **Shader Model 6.x**,
while the engine deliberately authors SM 4/5-level HLSL. That is a
compiler-profile mismatch rather than a language mismatch: `-HV 2016` keeps the
SM 5-era language rules, and `-T ps_6_0` is merely the lowest profile DXC accepts.

| Flag | Why it is there |
|---|---|
| `-spirv` | emit SPIR-V instead of DXIL |
| `-fspv-target-env=vulkan1.0` | SPIR-V 1.0 / Vulkan 1.0, matching the `glslang` target |
| `-T ps_6_0` | profile (`vs_6_0` for vertex, `cs_6_0` for compute, ...); the *lowest* SM 6 profile, not an authoring level |
| `-E main` | entry-point name, which `compileShaderProgram()` already requires |
| `-HV 2016` | HLSL language version: SM 5-era parsing (`in`/`out`, `cbuffer`, `Texture2D`/`SamplerState`, no SM 6 template resources) |
| `-flegacy-macro-expansion` | pre-SM 6 macro expansion |
| `-flegacy-resource-reservation` | reserve slots for declared-but-unused resources, keeping register numbering stable between stages |
| `-fvk-auto-shift-bindings` | shift bindings per resource class so `t0` and `s0` do not both land on binding 0; only *register-derived* bindings are shifted, an explicit `[[vk::binding(slot, set)]]` is honoured verbatim (see the set table in \ref mod_kaldivk) |
| `-fvk-use-gl-layout` | GL-style layout rules for constant/structured buffers, matching what SPIRV-Cross and the GL backends expect |

Reference invocation, one stage at a time (repeat with the matching `-T`):

```sh
dxc -spirv -fspv-target-env=vulkan1.0 -T ps_6_0 -E main \
    -HV 2016 -flegacy-macro-expansion -flegacy-resource-reservation \
    -fvk-auto-shift-bindings -fvk-use-gl-layout \
    shader.hlsl -Fo shader.spv
```

### Slang — worth examining

[Slang](https://github.com/shader-slang/slang) is worth examining as a third
option (it has an HLSL-compatible front-end and direct SPIR-V output), but no
evaluation has been done and no dependency is planned.

SPIRV-Cross, the other half of the pipeline, is unaffected by this choice: it
consumes SPIR-V, so it does not care which compiler emitted it.

## Migration status: deferred indefinitely

The `glslang` → `DXC` migration is **indefinitely deferred**. The compiler is a
system dependency of Lotte (`-lglslang`) and of the test build (`/usr/bin/glslang`),
not a vendored copy, so no build graph changes. The trigger to migrate is a system
`glslang` upgrade that lands *past* the front-end removal: at that point HLSL
compilation in `Lotte::Gfx::compileShaderProgram()` and the shader-asset rule break
together. Until then, pinning the current `glslang` (or simply not upgrading past
the removal) keeps everything working.

When the migration happens, the work is mechanical and bounded:

1. Switch the compiler in `Lotte::Gfx::compileShaderProgram()` (run time) and in
   the `UnitTests.pro` rule (build time), keeping the `GLSLANG=`-style override so
   a machine can point at either tool.
2. Use `-HV 2016` plus the legacy flags above so the accepted language stays at
   SM 4/5 semantics; the authoring level must not drift to Shader Model 6 merely
   because the compiler only speaks SM 6 profiles.
3. Keep `[[vk::binding(slot, set)]]` in the shader assets, which keeps the
   Kaldi → Vulkan descriptor mapping unchanged.
4. Re-run `UnitTests "[shader]"` (SPIR-V magic, stage entry points, binding
   survival, GLSL cross-compilation) and the Vulkan triangle smoke (per-pixel
   readback) as the acceptance gate.

Nothing else in the backend matrix moves: the Vulkan backend consumes the blob
as-is, and the GL/GLES/D3D backends only ever see SPIR-V through SPIRV-Cross.

## References

- [`../index.md`](../index.md) — the graphics folder this note belongs to.
- [`../../Lotte/GFX/LtePipeline.cpp`](../../Lotte/GFX/LtePipeline.cpp) — the runtime
  compiler this note's policy is implemented in.
- [`../../UnitTests/UnitTests.pro`](../../UnitTests/UnitTests.pro) — the build-time
  HLSL -> SPIR-V rule and its `GLSLANG=` override.
- \ref mod_lotte — the module that owns the compiler.
- \ref mod_kaldivk — the backend that consumes SPIR-V directly.
- \ref mod_kaldiglcommon — the GL family's SPIR-V -> GLSL step.
- \ref mod_kaldid3dcommon — the D3D family's SPIR-V -> HLSL step.
- [KhronosGroup/glslang issue 4210](https://github.com/KhronosGroup/glslang/issues/4210)
  — the HLSL front-end deprecation this note plans around.

