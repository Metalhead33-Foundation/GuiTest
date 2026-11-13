# Elvavena

Elvavena is the foundational utility library of MhLib. Every other module builds on
it: it owns the shared I/O device abstraction, the generic containers and allocators,
the dynamic-library and event-loop helpers, and the serialization primitives that the
higher layers reuse instead of reimplementing.

**Status: implemented.** The library is usable today. It is a shared library with four
compiled translation units; the rest of its surface is header-only.

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Elvavena` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_UTIL_EXPORT` while building, `MH_UTIL_API` for consumers |
| Dependencies | C++20 standard library, POSIX `dl` (for `DynamicLibrary`) |
| Consumer hook | `include(Elvavena/Elvavena.pri)` adds the include path and `-lElvavena` |

Compiled units: `Io/ElvIoDeviceProxy.cpp`, `Util/ElvDynamicLib.cpp`,
`Util/ElvEventLoop.cpp`, `Util/ElvThreadPool.cpp`. Everything else is inline/template
code in headers.

## Implemented

### `Io/` - the device abstraction shared by higher layers

| Header | Contents |
|---|---|
| `ElvIoDevice.hpp` | `Mode`, `SeekOrigin`, the `DeviceLike` concept, the abstract `Device` (read/write/seek/tell/size/eof/flush/isValid/getMode), and the `pDevice`/`sDevice`/`uDevice` aliases |
| `ElvIoDeviceProxy.hpp` | `BufferedDeviceProxy`, `BufferedReadDeviceProxy`, `BufferedWriteDeviceProxy` |
| `ElvIoSystem.hpp` | Abstract `System` filesystem interface (open, exists, enumerate, mkdir, remove, separator, ...) |
| `ElvDataStream.hpp` | Endian-templated `DataStream<IoType>` with `operator<<`/`operator>>` for scalars, strings (null-inclusive `uint32` length, matching FontPacker), STL containers/maps (`uint32` counts), `std::optional`/`std::monostate`, plus bounds-checked ULEB128/SLEB128 readers (Wasm-style byte-length and overflow limits on decode) |
| `ElvIoAsync.hpp` | `AsyncResult<T>`, `AsyncOperationCanceled`, and the `Async<Executor>` adapter (`read`, `write`, `seek`, `size`, `flush`, `readAll`, `*Then` callback variants, cancellation via `std::stop_token`) |

### `Util/` - containers, allocators, algorithms and runtime helpers

| Area | Headers |
|---|---|
| Allocators / memory | `ElvAlexandrescuResource.hpp` (alignment-correct PMR bridge over Alexandrescu allocators), `ElvAllocatorBasic.hpp` (allocator concept and smart-pointer adapters), `ElvBitmapAllocator.hpp`, `ElvFreelist.hpp` (`FixedFreeListAllocator`, `ContiguousFreeListAllocator`, `FreelistMemoryManager`), `ElvStackAllocator.hpp`, `ElvHeapArray.hpp`, `ElvChunkyArray.hpp`, `ElvIntrusiveArray.hpp`, `ElvIdPool.hpp` (thread-safe ID recycling) |
| Containers | `ElvBinaryTree.hpp`, `ElvContainerBasic.hpp`, `ElvDoubleBufferedVector.hpp`, `ElvStringhashMap.hpp`, `ElvThreadsafeContainer.hpp`, `ElvThreadsafeQueue.hpp`, `ElvObserv.hpp` (observer pattern) |
| Algorithms / math | `ElvFixedPoint.hpp`, `ElvFixedPointMath.hpp`, `ElvInterpolation.hpp`, `ElvMathUtil.hpp`, `ElvNormDenorm.hpp`, `ElvDither.hpp`, `ElvEdgeFunction.hpp`, `ElvFuzzy.hpp` (fuzzy logic), `ElvSpanHelpers.hpp`, `ElvIntegralIterator.hpp`, `ElvContinuousIterator.hpp`, `ElvEndianness.hpp` |
| Runtime / platform | `ElvDynamicLib.hpp` (`dlopen`/`LoadLibrary` wrapper), `ElvEventLoop.hpp` (single background command thread with `enqueue`/`enqueueAsync`/`enqueueSync`), `ElvThreadPool.hpp`, `ElvAsyncExecutor.hpp` (executor concept), `ElvException.hpp`, `ElvUtilGlobals.hpp` (API visibility macros, `DEFINE_PTR`/`DEFINE_CLASS`/`DEFINE_STRUCT`) |
| RAII / scope cleanup | `ElvScopeGuard.hpp` (`ScopeGuard<Fn>`, `ScopeExitMode`, `makeScopeGuard`/`atScopeExit`, and the `ELV_SCOPE_EXIT` / `ELV_SCOPE_FAIL` / `ELV_SCOPE_SUCCESS` macros; failure/success modes use `std::uncaught_exceptions`), `ElvDeferStack.hpp` (`DeferStack`: fixed-capacity, non-allocating LIFO cleanup thunks in caller-owned storage; `defer`, LIFO `dismiss`, `runAll`) |
| Third-party | `half.hpp` - vendored half-float implementation |

## TODOs and known gaps

The sources contain no explicit `TODO`/`FIXME` markers; the following are the known
limits of the current implementation:

- [ ] `EventLoop` is only a sequential command pump. It has no integration with OS
      event sources (timers, signals, sockets, file descriptors) and no shutdown
      handshake beyond enqueueing the stop flag in its destructor.
- [ ] There is no concrete `Elv::Io::Device` or `Elv::Io::System` implementation in this
      module; the concrete files, sockets, compression and encryption devices live in
      Euphemy. Elvavena only defines the interfaces and the generic proxies.
- [ ] `DataStream` is a binary serialization primitive only. There is no schema,
      versioning or reflection layer; higher layers (for example Lotte's font format)
      define their own stream operators.
- [ ] There is no dedicated test target in this module. Elvavena is covered indirectly
      from `Test/test.cpp` (allocators, iterators, `DataStream`, async I/O, thread pool).
- [ ] `ElvAllocatorBasic.hpp`, `ElvBinaryTree.hpp`, `ElvContainerBasic.hpp`,
      `ElvDither.hpp`, `ElvEdgeFunction.hpp`, `ElvFuzzy.hpp`, `ElvObserv.hpp`,
      `ElvStringhashMap.hpp`, `ElvThreadsafeContainer.hpp` and `ElvThreadsafeQueue.hpp`
      have no direct test in the Test app yet.
- [ ] `ElvScopeGuard.hpp` and `ElvDeferStack.hpp` are newly merged and have no direct
      test in the Test app yet. Their contracts to keep in mind: scope-guard callables
      must not throw, `dismiss()`/`activate()` are the only state changes, and
      `DeferStack` requires LIFO dismissal and never allocates (a full stack returns an
      invalid token instead).
