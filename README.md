# MhLib

An experimental C++ library (hobby project) intended to have the following features:
- 3D rendering engine, abstracting around OpenGL 4.x (4.x, but with optional 4.6 things), OpenGL ES 3.2, Direct3D 11, Vulkan and Direct3D 12.
- Audio engine
- I/O abstractions
- Various convenient utility classes based off .NET and Qt

The project structure is as follows:
- [Elvavena](Elvavena/README.md) is the foundational utility library, consisting of core utility types and algorithms, memory and container helpers, event loop and dynamic library helpers, I/O abstractions shared by higher-level modules.
- [Euphemy](Euphemy/README.md) is the media-focused library in MhLib: file and device I/O, audio-related media support, image decoding and image utilities, configuration and platform-aware support code.
- [Haruka](Haruka/README.md) is an audio engine aimed at providing audio effects, streaming buffers, 3D position audio and convolution effects (e.g. reverb, echo, delay, IRs).
- [Kaldi](Kaldi/README.md) is a graphics library abstraction layer / RHI / HAL that Lotte is built on top of. It is a data-oriented interface (command queue for performance) with an object-oriented facade (for easier ergonomics). Implementations include [OpenGL 4.x](KaldiGL/README.md), [OpenGL ES 3.2](KaldiGLES/README.md), [Direct3D 11](KaldiD3D11/README.md), [Direct3D 12](KaldiD3D12/README.md) and [Vulkan](KaldiVK/README.md), with support for third-party backends developed by others.
- [Lotte](Lotte/README.md) is the higher-level graphics library built on top of Kaldi, providing GUI rendering and 3D rendering capabilities.
- [UnitTests](UnitTests/README.md) is the automated (Catch2) suite that proves data survives
  round-trips through the libraries.
- [Sandbox](Sandbox/README.md) is the manual playground application for humans (hardcoded paths and
  experiments allowed).

## Module status

This is the short version; each module README has the detailed "implemented vs TODO"
list, and the headers are the API reference.

| Module | Status | What works today | Main remaining work |
|---|---|---|---|
| [Elvavena](Elvavena/README.md) | Implemented | Device abstraction, containers/allocators, `DataStream` with checked LEB128, scope guards/defer stack, async I/O, event loop, thread pool, dynamic libraries, signals/slots | OS event-source integration for `EventLoop`; direct tests |
| [Euphemy](Euphemy/README.md) | Implemented, small gaps | File/memory/socket/temp devices, zstd + crypto proxies, PNG/JPEG/TGA/WebP encode+decode, GIF/DDS decode, sound files, module rendering, resampling, convolution, config, optional PMR arenas, CPU asset runtime | `BufferDevice::isValid()` stub; GIF/DDS encoders; prune stale build dependencies |
| [Haruka](Haruka/README.md) | Partial | Pull-based `Playable` graph, mixer, matrix/ambisonic panners, resampler, sound/module streamers, in-memory sources, dry/wet effect chain (clamp, crush, convolver, impulse-response synthesis) | No output device, no listener/3D model, wider test coverage |
| [Kaldi](Kaldi/README.md) | Contract + facade implemented | HAL 1.0/1.1 contract, 98-opcode command vocabulary, header-only recording facade, merge/deferred-destroy helpers | No working backend yet; swapchain/render-pass proxies; tests |
| [KaldiGL](KaldiGL/README.md), [KaldiGLES](KaldiGLES/README.md), [KaldiVK](KaldiVK/README.md), [KaldiD3D11](KaldiD3D11/README.md), [KaldiD3D12](KaldiD3D12/README.md) | Stubs | Shared library target and C ABI exports | All rendering, context/swapchain creation, command translation |
| [Lotte](Lotte/README.md) | Partial | Kaldi facade aliases, CPU→GPU asset streamer, FontPacker font (`WODF`) and vector-image (`WODI`) formats, MML parser, rich-text runs, text layout, glyph quads, mesh and GUI geometry batches | Renderer, GUI widgets, glyph atlas, 3D |
| [UnitTests](UnitTests/README.md) | Working | Catch2 suite: asset runtime behaviour, zstd/Snappy round-trips, PNG/TGA round-trips, font `WODF`/`WODI` round-trips, configuration round-trip | More codec and Kaldi backend coverage |
| [Sandbox](Sandbox/README.md) | Manual playground | Ad-hoc scenarios: GPU asset streamer, async I/O, image dumps, audio effects, text pipeline, signals, geometry batches | Grows as needed by hand; not part of the gate |

## Documentation

- Module READMEs (linked above) are the source of truth for **what is implemented and
  what is still a TODO**.
- Header comments are written in Qt-style Doxygen (`//!`, `/*!`, `\brief`, `\param`, ...).
  `docs/DOCUMENTATION.md` describes the conventions and the maintenance rules.
- `docs/SALVAGE.md` records what was carried over from the older trees
  (`GuiTest-legacy`, `GuiTest-legacy2`/`NewBeginning`), what was deliberately dropped,
  and why each surviving concept changed shape.
- Render the reference with:
  ```sh
  doxygen Doxyfile
  $BROWSER build/doxygen/html/index.html
  ```
  The output goes to `build/doxygen/`, never into the source tree. CI does the same in
  [`.github/workflows/doxygen.yml`](.github/workflows/doxygen.yml).

## Building

This is a qmake project. Makefiles are generated artifacts; keep every build product
inside `build/`.

```sh
cd build/Desktop-Debug
/usr/bin/qmake6 ../../Project.pro
env MAKE=/usr/bin/make /usr/bin/make -j16 QMAKE=/usr/bin/qmake6
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka ./UnitTests/UnitTests
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka ./Sandbox/Sandbox
```

The sibling shared libraries are not found through an rpath, so the `LD_LIBRARY_PATH`
prefix is required when running the test binaries from the shadow build tree.
`UnitTests` exits non-zero on a failed check; `Sandbox` is a manual playground and is
expected to be edited and run by hand.

Use the absolute `make`/`qmake6` paths: a terminal may rewrite `argv[0]`, and GNU make
then derives a wrong `MAKE` variable and spawns new IDE windows instead of recursive
builds. See [`AGENTS.md`](AGENTS.md) for the full rule.

## Long-term plans

- Developing Haruka
- Development of the various Kaldi backends
- More development on everything, until we have the foundations for a 3D engine (Lotte)
- Eventual conversion from QMake to CMake (QMake is convenient for usage in QtCreator and other IDEs, making the addition of new files trivial for human developers)
- Eventual integration of [WasmJIT](https://github.com/Metalhead33-Foundation/Wasmjit/tree/master) into this project, to make it a first-class citizen of MhLib.

The project is intended as a spiritual successor of the original 1998 Unreal Engine and 1999 Quake 3 engine, albeit using modern graphics APIs (strictly programmable shaders, no fixed-function or immediate-mode hijinks), surrounded by a utility library loosely inspired by .NET and Qt. The Unreal Engine aspect is mainly manifested in the "rendering backends are dynamic linking libraries loaded at runtime, implementing an abstract interface: the engine itself is strictly backend-agnostic" philosophy, while the Quake 3 engine aspect is manifested in the long-term goal of integrating a spiritual successor of QVM: [WasmJIT](https://github.com/Metalhead33-Foundation/Wasmjit/), a WebAssembly engine that has Euphemy and Elvavena as its dependencies.
