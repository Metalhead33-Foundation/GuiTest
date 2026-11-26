\page mod_unittests UnitTests

# UnitTests

`UnitTests` is the project's automated test suite (`TARGET = UnitTests`), built on **Catch2**. It
exists to prove that data survives round-trips through the libraries: a font face written by
FontPacker and read by Lotte, an image encoded and decoded again, a payload compressed with zstd or
Snappy, a configuration written as text and parsed back. Those are the invariants that silently rot
when a format or a codec changes.

The manual, human-driven scenarios live in [`../Sandbox`](../Sandbox/README.md).

**Status: implemented and passing.** 223 test cases / 2920 assertions covering the
asset runtime, compression, image codecs, the font containers, configuration,
bit-level packet coding, the networking foundation (transport/session/clock/RPC,
snapshots, field deltas, interpolation, prediction, lockstep and the reference
pipeline), the minimal HTTP parser/client/server, the platform wrappers and a
localhost UDP smoke test, plus the graphics side: HLSL/GLSL -> SPIR-V shader
compilation and SPIR-V -> GLSL cross-compilation, the Vulkan/DXGI format tables,
backend configuration clamping (including the GL executor policy keys), the SDL
window-handle extraction, the Kaldi HAL capability fallbacks, the Kaldi
command-dispatch layer (both the computed-goto and the `switch` structure), the
object-oriented recording facade (`Kaldi/KldCommandBuffer.hpp`), the GUI
render-tree/widget/theme contract, and the documentation tree itself (`[docs]`).

## Shader assets

HLSL is the project's primary shading language, so the suite compiles real HLSL
instead of hand-written SPIR-V words. Two independent paths are covered:

- `tests/ShaderCompileTests.cpp` drives `Lotte::Gfx::compileShaderProgram()` at run
  time (the same entry point a game or tool uses).
- `shaders/*.hlsl` are compiled to `.spv` **at build time** by `UnitTests.pro`,
  using `QMAKE_EXTRA_COMPILERS` exactly like the WasmJit test harness compiles
  AssemblyScript to `.wasm`. `tests/ShaderAssetTests.cpp` then loads those blobs
  from the build tree and feeds them to the SPIR-V -> GLSL cross-compiler, so a
  broken shader file breaks the build/test run rather than a backend later.

The build-time rule needs `glslang` (Khronos). It defaults to `/usr/bin/glslang`
and can be pointed elsewhere with `qmake UnitTests.pro GLSLANG=/path/to/glslang`;
if the compiler is missing the rule is disabled with a configure-time warning and
the asset tests are compiled out.

Note that **glslang's HLSL front-end is deprecated upstream** (as of April 2026,
removed at the next major version; KhronosGroup/glslang issue #4210). The rule is
kept because it is the only build-time proof that the shader assets compile at
all; the intended replacement is DXC with `-HV 2016` plus its legacy flags, which
keeps the accepted language at Shader Model 4/5 level even though DXC only emits
Shader Model 6 profiles. The migration is **deferred indefinitely**, and
[`docs/graphics/SHADERS.md`](../docs/graphics/SHADERS.md) records the reference `dxc` invocation,
what every flag does, and the acceptance gate for the switch.

## Build and run

```sh
cd build/Desktop-Debug
/usr/bin/qmake6 ../../Project.pro
env MAKE=/usr/bin/make /usr/bin/make -j16 QMAKE=/usr/bin/qmake6
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka:KaldiVK ./UnitTests/UnitTests
```

`LD_LIBRARY_PATH` is required because the sibling libraries are not found through an rpath.
`KaldiVK` is on the path because the test binary links the Vulkan backend's translation layer
(`VkUtil`); it does not create a Vulkan device.

The suite exits non-zero when a check fails, so it can be used as a gate. Useful Catch2 arguments:

```sh
./UnitTests/UnitTests --list-tests          # names and tags
./UnitTests/UnitTests "[roundtrip]"        # only round-trip cases
./UnitTests/UnitTests "[net]"              # networking foundation (no OS sockets)
./UnitTests/UnitTests "[net][socket]"      # localhost UDP transport
./UnitTests/UnitTests "[ringbuffer]"       # contiguous streaming buffer + Haruka streamer
./UnitTests/UnitTests "[eventloop]"        # polling loop + epoll/eventfd integration
./UnitTests/UnitTests "[epoll]"            # Linux epoll readiness poller
./UnitTests/UnitTests "[kqueue]"           # kqueue availability/stub
./UnitTests/UnitTests "[uring]"            # io_uring availability path
./UnitTests/UnitTests "[iocp]"             # IOCP stub/Windows path
./UnitTests/UnitTests "[snapshot]"         # snapshot delta/baseline framing
./UnitTests/UnitTests "[interpolation]"    # jitter/schema interpolation
./UnitTests/UnitTests "[prediction]"       # input queues/checksum rollback
./UnitTests/UnitTests "[lockstep]"         # input collection/checksum exchange
./UnitTests/UnitTests "[pipeline]"         # per-peer replication server/client
./UnitTests/UnitTests "[http]"             # parser; "[http][socket]" also TCP/UDP
./UnitTests/UnitTests "[fielddelta]"       # snapshot field-level delta
./UnitTests/UnitTests "[shader]"           # HLSL/GLSL -> SPIR-V (+ SPIRV-Cross)
./UnitTests/UnitTests "[format]"           # Vulkan/DXGI format tables
./UnitTests/UnitTests "[backend]"          # Kaldi backend configuration contract
./UnitTests/UnitTests "[nativewindow]"     # SDL -> native window handles
./UnitTests/UnitTests "[dispatch]"         # Kaldi dispatch (computed goto + switch)
./UnitTests/UnitTests "[capabilities]"     # HAL fallbacks + status helpers
./UnitTests/UnitTests "[docs]"             # the note tree, the module pages and their images
./UnitTests/UnitTests "[noise]"            # Euphemy noise: determinism, ranges, degenerate inputs
./UnitTests/UnitTests "[voronoi]"          # tile Voronoi: distances and cell indices vs brute force
./UnitTests/UnitTests "[procedural]"       # GUI atlas: manifest, determinism, shading
./UnitTests/UnitTests "[widget]"           # a theme skin drives the rendered widget
./UnitTests/UnitTests "[theme]"            # theme schema, validation, DefinitionTheme
./UnitTests/UnitTests "[json]"             # the Sandbox theme loader (the only JSON consumer)
./UnitTests/UnitTests "Snappy*" -s         # one group, showing passing assertions
```

## Dependency: Catch2

Catch2 comes from the system installation - nothing is vendored. `UnitTests.pro` uses
`CONFIG += link_pkgconfig` with `PKGCONFIG += catch2-with-main`, which resolves to the framework
headers (`<catch2/...>`) plus `-lCatch2Main -lCatch2` (version 3.x, where `main()` is provided by the
library, so this module has no `main.cpp`).

If the pkg-config module is not available, point the build at the headers/libraries manually, or
build Catch2 as a single amalgamated header and replace the includes.

## Test suites

| File | Covers |
|---|---|
| `tests/AssetRuntimeTests.cpp` | Residency state machine and its legal/illegal transitions; generation-checked handles; kind validation; strong references blocking destruction; eviction-request bookkeeping; budget measurement, pinning and victim selection; dependency-ordered loading; dependency-failure propagation (`DependencyFailed`); cancellation reported as `Canceled` with no payload installed; budget stalls (`budgetBlocked()`); queued-request admission control (`BudgetExceeded`) |
| `tests/CompressionTests.cpp` | zstd and Snappy proxy devices round-trip payloads from 0 bytes to 200 kB; non-default Snappy chunk sizes; empty streams; foreign/truncated input is rejected; zstd checksum/level settings; zstd nested inside Snappy |
| `tests/ImageCodecTests.cpp` | PNG and TGA encode -> decode reproduce the exact pixels of a 23x17 image; re-encoding a decoded PNG is byte-identical |
| `tests/FontRoundTripTests.cpp` | Lotte `PreprocessedFontFace` and `StoredVectorImage` through `Elv::Io::DataStream`, comparing every persisted field (metadata, metrics, kerning, SDF payload, mipmaps); `loadFontFace()` through both an `Elv::Io::System` double and an already-open `Elv::Io::Device`, plus `FontRepository::addFaceFrom{File,Device}()` |
| `tests/TextRenderingTests.cpp` | `FontRepository` family/weight resolution, lazy multi-page `FontAtlas` shelf packing and upload recording, and `TextRenderer` vertex output (per-vertex colour, weight-scaled threshold, MSDF mode, decoration quads, page runs) |
| `tests/ConfigTests.cpp` | Configuration text round-trip for every typed value; default caching for missing keys; parsing a hand-written document; rejection of keys without a type prefix |
| `tests/BackendConfigTests.cpp` | The Kaldi config contract end to end: GL provider defaults written with typed keys, key prefixes matching value types, clamping (`iGlSamples=999` -> 16), unknown enumerations falling back, missing sections keeping defaults, `.ini` serialize -> parse -> apply round-trip, per-purpose common defaults |
| `tests/BitStreamTests.cpp` | MSB-first bit packing; mixed-width fields; full-range zig-zag signed values; varints; exact float/half bit patterns; quantized float clamping and degenerate ranges; buffer-full/exhausted errors; overlong varint rejection |
| `tests/FormatTranslationTests.cpp` | Vulkan (`VkUtil`) and DXGI (`D3DUtil`) tables: formats that exist, formats the APIs cannot express, vertex formats, sampler/layout/stage masks, alignment helpers |
| `tests/GuiWidgetTests.cpp` | GUI base contract: accumulated and stable z-index ordering with min/max tracking, `RootNode` ownership/iteration/removal, widget state-bit signals and hit testing, `TexturedWidget` per-state atlas selection and no-op click/hidden behaviour, and the abstract theme skin/text/metric interface |
| `tests/KaldiCommandBufferTests.cpp` | The object-oriented facade without a device: one handle namespace across kinds with id recycling, write-buffer-only recording, proxy create/destroy commands, move semantics, `mergeCommandBuffers` ordering, `DeferredDestroyQueue` mapping to the right destroy opcode, `submit()` handing the read buffer to a recording device |
| `tests/HarukaStreamerRingTests.cpp` | Deterministic `RingBufferedStreamer` through a four-frame ring; interleaved and separate-channel output across repeated wraps |
| `tests/HttpTests.cpp` | Request/response serialization, incomplete/malformed/oversize parsing, HEAD suppression, TCP POST, TCP HEAD, `EventLoop` async request, UDP request/response, failure-signal emission |
| `tests/NativeWindowTests.cpp` | `Kld::nativeWindowHandlesFrom()` for unsupported subsystems, X11 and Wayland (including the `wl_surface` vs `wl_egl_window` split), plus the GL `ContextConfig`/`ContextApi` defaults |
| `tests/NetClockRpcTests.cpp` | RTT/offset estimation and smoothing, bounded sample history, tick/time conversion; typed RPC encode/dispatch, enums/blobs, unknown/malformed/denied calls and handler exceptions |
| `tests/NetPredictionInterpolationTests.cpp` | Jitter ordering/interpolation, schema field rules, snapshot jitter buffer, input/delay queues, checksum rollback, lockstep readiness and divergence |
| `tests/NetReplicationPipelineTests.cpp` | Per-peer `OwnerInterest` snapshots, ack routing, peer removal, client parse/jitter buffer |
| `tests/NetReplicationTests.cpp` | Generation-checked ID reuse, component/entity-kind validation, object-schema bit-stream round-trip, component masks, stale-handle rejection, interest filtering |
| `tests/NetSessionTests.cpp` | Two-session handshake and retry; reliable ordered delivery and ack tags; delivery over a 35% lossy link; fragmentation/reassembly; heartbeat timeout; graceful disconnect; unreliable channel delivery; input validation |
| `tests/NetSnapshotTests.cpp` | Full spawn baseline, ack-gated deltas, changed/removed component masks, despawns, rejected tags, malformed magic/version/truncation; schema field-delta full/delta modes |
| `tests/NetTransportTests.cpp` | Wrap-safe sequence comparison and ack windows; CRC-32C/FNV-1a vectors; ring-buffer ordering, overwrite and move-only elements; endpoint value semantics; loopback latency, loss, duplication, oversize rejection and endpoint lifecycle |
| `tests/PlatformWrappersTests.cpp` | Contiguous ring-buffer spans/wrap/bulk copy/resize; event-poller factory; epoll pipe readiness with modify/remove; polling loop commands/readiness/wakeup; kqueue stub; io_uring NOP when available; IOCP unavailable stub |
| `tests/ShaderCompileTests.cpp` | Lotte's HLSL/GLSL -> SPIR-V compiler: one blob per stage, `Kld::ShaderBinaryDescriptor` views consistent with the stored words, empty/duplicate/diagnostic/include failure paths, `Pipeline` ownership |
| `tests/UdpTransportTests.cpp` | Localhost non-blocking UDP request/reply through `Euph::Net::UdpTransport` and `Euph::Io::Socket`, including ephemeral-port binding and source-endpoint capture |
| `tests/DocumentationTests.cpp` | The note tree: every note declares a `{#doc_...}` id and is listed in its folder index, every `\subpage` row resolves, every module README is a `\page mod_<folder>` page listed in `docs/modules/index.md`, page ids are unique, `## References` is the last section of every note, and every image a note references exists with a base name Doxygen can copy without collision |

`DocumentationTests.cpp` reads the repository through the `UNITTESTS_SOURCE_ROOT` define that `UnitTests.pro` injects, so it never depends on the working directory and never looks at `extern/` or `build/`. It checks structure, not prose quality: the other half of the documentation gate is the `doxygen Doxyfile` render in `docs/DOCUMENTATION.md` §8. The image check exists because Doxygen emits an `<img>` tag for a Markdown image even when it cannot find the file, copies nothing and reports no warning — a moved or renamed mascot used to disappear from the site silently.

`support/TestSupport.hpp` holds the shared helpers: deterministic payload generators and
`readToEnd()`, which drains a decompressing proxy device (its `size()` reports the *compressed* size,
so the loop has to run until a read returns zero).

## Conventions

- One `TEST_CASE` per behaviour, with `[module][topic]` tags so groups can be filtered.
- Prefer `CHECK` for independent assertions and `REQUIRE` when the rest of the case would be
  meaningless after a failure.
- Use `CAPTURE()` inside loops over sizes so a failure names the input that broke.
- Tests must be deterministic and must not touch hardcoded machine paths: build inputs in memory
  (`Euph::Io::MemoryDevice`) and, if a fixture file is ever needed, read it from a path passed on the
  command line instead of a literal.
- New formats or codecs get a round-trip case here; exploratory scenarios belong in `Sandbox`.
