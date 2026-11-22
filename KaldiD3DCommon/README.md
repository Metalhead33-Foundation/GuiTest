# KaldiD3DCommon

Shared implementation details of the [KaldiD3D11](../KaldiD3D11/README.md) and
[KaldiD3D12](../KaldiD3D12/README.md) backends. **No library is built here** - like
[`KaldiGLCommon`](../KaldiGLCommon/README.md), the sources are compiled into the consuming
backend.

| File | Purpose |
|---|---|
| `D3DUtil.hpp` / `D3DUtil.cpp` | Euphemy `Format` and `Kld::PrimitiveType` -> `DXGI_FORMAT`, depth/compression queries, `alignUp()`, `mipLevelCount()` |
| `D3DSPirvShader.hpp` / `D3DSPirvShader.cpp` | SPIR-V (`Kld::ShaderBinaryDescriptor`) -> HLSL source through SPIRV-Cross, parameterised by Shader Model |
| `KaldiD3DCommon.pri` | Include path plus `$$KALDID3D_COMMON_SOURCES` / `$$KALDID3D_COMMON_WIN32_SOURCES` / `$$KALDID3D_COMMON_HEADERS` |

`D3DSPirvShader.cpp` deliberately touches no Direct3D API, so it compiles (and is tested)
on every host. `D3DUtil.cpp` needs `<dxgiformat.h>` and is therefore only built inside a
`win32` scope. `D3D11Util`/`D3D12Util` live in their own backends because
`D3D11_INPUT_ELEMENT_DESC` and `D3D12_INPUT_ELEMENT_DESC` are different types.

The input is HLSL authored at Shader Model 4/5 level and compiled to SPIR-V 1.0 by
Lotte; this cross-compiler's Shader Model 5.0/5.1 output is what bounds which
shader constructs the engine may use (no Shader Model 6 features). The compiler
choice and its `glslang` deprecation are documented in
[`docs/SHADERS.md`](../docs/SHADERS.md).

## Header provenance

The two Direct3D backends do **not** get their headers from the same place, and the
difference matters.

### `extern/DirectX-Headers` (Direct3D 12) - authoritative

Official Microsoft repository (<https://github.com/microsoft/DirectX-Headers>), MIT
licensed, included as `include/directx/d3d12.h` and `include/directx/dxgiformat.h`. This
is the header set Microsoft itself ships for D3D12, including the WSL adapter used to build
D3D12 code on non-Windows hosts. Keeping it as a pinned submodule is the right call. Do not
mix it with `extern/DXSDK/Include` on the same include path - both define `DXGI_FORMAT` and
the D3D12 types.

### `extern/DXSDK` (Direct3D 11 and older) - unofficial mirror

`extern/DXSDK` is <https://github.com/apitrace/dxsdk.git>, whose own README says it
"combines headers of several DXSDK, to facilitate Windows builds of Apitrace". What that
means concretely:

- **It is a community merge, not a Microsoft release.** The 90 headers come from several
  DirectX SDK and Windows SDK generations (`d3d11_1.h` ... `d3d11_4.h` including
  `ID3D11Device5` are present, so it reaches beyond the June 2010 DirectX SDK).
- **There is no license file.** The files carry Microsoft's copyright notice, so
  redistribution is governed by the DirectX SDK/Windows SDK EULA, not by an open-source
  license. Treat the tree as a local build fallback and do not ship it in release
  artifacts; if in doubt, ask a lawyer, not a README.
- **It cannot be compiled on Linux/macOS.** The headers need `windows.h`, WRL, SAL and
  `winapifamily.h` from the Windows SDK, which is why `D3D11Util.cpp` is `win32`-gated and
  why nothing here can be syntax-checked in this repository's CI.

How much should we trust it? It depends on what we take from it:

| Usage | Risk | Verdict |
|---|---|---|
| Plain enums/structs we use today (`DXGI_FORMAT`, `D3D11_INPUT_ELEMENT_DESC`, `D3D11_MAP`, `D3D11_SUBRESOURCE_DATA`, `D3D12_INPUT_ELEMENT_DESC`, `D3D12_SUBRESOURCE_DATA`) | Value/layout is specified by Microsoft's public documentation and has been stable for a decade | Low. Our format tables can be cross-checked against both header sets (they are, see `D3DUtil.cpp`) |
| COM interface declarations (`ID3D11Device`, `ID3D12Device`, ...) used for real device creation | A stale or mis-merged vtable declaration silently jumps to the wrong slot in the OS DLL; this is memory corruption, not a compile error | High. Use the Windows SDK (`d3d11_4.h`, `d3d12.h`) from a real Windows toolchain for this |
| Deprecated APIs (D3D9/D3D10/`d3dx*`) | These are no longer in the current Windows SDK at all | Accept the risk, or drop the APIs |

Practical policy, therefore:

1. Direct3D 12 builds against `extern/DirectX-Headers` (already the case).
2. Direct3D 11 builds against the **Windows SDK** on a Windows host; `extern/DXSDK` only
   exists so that the include paths resolve on machines without one.
3. Prefer `<d3d11_4.h>` / `<d3d12.h>` over the oldest headers when creating device objects,
   so the interface includes every method that modern Windows DLLs export.
4. The current code only consumes POD enums/structs, so nothing we have written so far
   depends on the risky part of the mirror.

`MinGW-w64`'s own `d3d11.h` is a permissively licensed alternative to `extern/DXSDK`, but
it is just as unofficial; if the licensing question ever becomes blocking, that is the
migration path.
