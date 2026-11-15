# KaldiGLCommon

Shared implementation details of the [KaldiGL](../KaldiGL/README.md) and
[KaldiGLES](../KaldiGLES/README.md) backends. **No library is built here** -
the sources are compiled into both backends, because a translation unit can
only target one GLSL dialect at a time.

| File | Purpose |
|---|---|
| `GlBackend.hpp` | Dialect selector: maps `GL_IMPLEMENTATION` / `GLES_IMPLEMENTATION` onto `KALDIGL_NS` (`GL` / `GLES`) and `KALDIGL_IS_GLES` |
| `GlUtil.hpp` / `GlUtil.cpp` | Euphemy `Format` -> GL format translation, `DecodeTarget`/`ImageView` uploads, `Kld::VertexDescriptor` -> vertex attribute state, `Kld::BufferMapAccess` -> `GL_MAP_*` bits |
| `GlSpirvShader.hpp` / `GlSpirvShader.cpp` | SPIR-V (`Kld::ShaderBinaryDescriptor`) -> GLSL / GLSL ES text through SPIRV-Cross, with optional external cache hooks |
| `KaldiGLCommon.pri` | Adds the include path and exposes `$$KALDIGL_COMMON_SOURCES` / `$$KALDIGL_COMMON_HEADERS` |

## Why a shared directory instead of `glad/`?

`glad/` is vendored third-party loader code and is regenerated from
<https://gen.glad.sh>; hand-written backend code must not be mixed into it.
`KaldiGLCommon/` follows the same pattern the repository already uses for
`Kaldi/Kaldi.pri`: an include path plus a source list that consuming projects
pull in. Nothing here is a subproject, so `Project.pro` does not list it.

## Dialect differences that are handled here

| Topic | Desktop OpenGL 4.3 | OpenGL ES 3.2 |
|---|---|---|
| Source include | `<glad/gl.h>` | `<glad/gles2.h>` |
| Namespace | `GL` | `GLES` |
| `GL_BGR` / `GL_BGRA` client formats | supported | not in the core header set -> reports "unsupported" |
| 16-bit `UNORM`/`SNORM` color formats | supported | not core in ES 3.x -> reports "unsupported" |
| 64-bit (`double`) vertex attributes | `glVertexAttribLPointer` | not representable -> `applyVertexDescriptor()` returns `false` |
| GLSL output | `#version 430`, highp defaults | `#version 320 es`, mediump fragment defaults |

Anything that reports "unsupported" is expected to be handled by CPU-side
conversion (for example a BGRA->RGBA swizzle in Euphemy) before it reaches the
backend.
