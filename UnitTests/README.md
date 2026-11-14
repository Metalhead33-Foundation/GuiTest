# UnitTests

`UnitTests` is the project's automated test suite (`TARGET = UnitTests`), built on **Catch2**. It
exists to prove that data survives round-trips through the libraries: a font face written by
FontPacker and read by Lotte, an image encoded and decoded again, a payload compressed with zstd or
Snappy, a configuration written as text and parsed back. Those are the invariants that silently rot
when a format or a codec changes.

The manual, human-driven scenarios live in [`../Sandbox`](../Sandbox/README.md).

**Status: implemented and passing.** 27 test cases covering the asset runtime, compression, image
codecs, the font containers and configuration.

## Build and run

```sh
cd build/Desktop-Debug
/usr/bin/qmake6 ../../Project.pro
env MAKE=/usr/bin/make /usr/bin/make -j16 QMAKE=/usr/bin/qmake6
LD_LIBRARY_PATH=Elvavena:Euphemy:Lotte:Haruka ./UnitTests/UnitTests
```

`LD_LIBRARY_PATH` is required because the sibling libraries are not found through an rpath.

The suite exits non-zero when a check fails, so it can be used as a gate. Useful Catch2 arguments:

```sh
./UnitTests/UnitTests --list-tests          # names and tags
./UnitTests/UnitTests "[roundtrip]"        # only round-trip cases
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
| `tests/FontRoundTripTests.cpp` | Lotte `PreprocessedFontFace` and `StoredVectorImage` through `Elv::Io::DataStream`, comparing every persisted field (metadata, metrics, kerning, SDF payload, mipmaps) |
| `tests/ConfigTests.cpp` | Configuration text round-trip for every typed value; default caching for missing keys; parsing a hand-written document; rejection of keys without a type prefix |

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
