# Elvavena

Elvavena is the foundational utility library of MhLib. Every other module builds on
it: it owns the shared I/O device abstraction, the generic containers and allocators,
the dynamic-library and event-loop helpers, and the serialization primitives that the
higher layers reuse instead of reimplementing.

**Status: implemented, with the networking foundation through M4 + the M5 reference
pipeline, and the platform I/O seam.** The library is usable today. It is a shared
library with eight compiled translation units; the rest of its surface is header-only.

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Elvavena` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_UTIL_EXPORT` while building, `MH_UTIL_API` for consumers |
| Dependencies | C++20 standard library, POSIX `dl` (for `DynamicLibrary`) |
| Consumer hook | `include(Elvavena/Elvavena.pri)` adds the include path and `-lElvavena` |

Compiled units: `Io/ElvDataSerializationSchema.cpp`, `Io/ElvIoDeviceProxy.cpp`,
`Net/ElvNetReplicationSchema.cpp`, `Net/ElvNetSession.cpp`, `Util/ElvDynamicLib.cpp`,
`Util/ElvEventLoop.cpp`, `Util/ElvPollingEventLoop.cpp`, `Util/ElvThreadPool.cpp`.
Everything else is inline/template code in headers.

## Implemented

### `Io/` - the device abstraction shared by higher layers

| Header | Contents |
|---|---|
| `ElvIoDevice.hpp` | `Mode`, `SeekOrigin`, the `DeviceLike` concept, the abstract `Device` (read/write/seek/tell/size/eof/flush/isValid/getMode), and the `pDevice`/`sDevice`/`uDevice` aliases |
| `ElvIoDeviceProxy.hpp` | `BufferedDeviceProxy`, `BufferedReadDeviceProxy`, `BufferedWriteDeviceProxy` |
| `ElvIoSystem.hpp` | Abstract `System` filesystem interface (open, exists, enumerate, mkdir, remove, separator, ...) |
| `ElvDataStream.hpp` | Endian-templated `DataStream<IoType>` with `operator<<`/`operator>>` for scalars, strings (null-inclusive `uint32` length, matching FontPacker), STL containers/maps (`uint32` counts), `std::optional`/`std::monostate`, plus bounds-checked ULEB128/SLEB128 readers (Wasm-style byte-length and overflow limits on decode) |
| `ElvIoAsync.hpp` | `AsyncResult<T>`, `AsyncOperationCanceled`, and the `Async<Executor>` adapter (`read`, `write`, `seek`, `size`, `flush`, `readAll`, `*Then` callback variants, cancellation via `std::stop_token`) |
| `ElvBitStream.hpp` | MSB-first `BitWriter`/`BitReader`: bit-packed integers, zig-zag signed values, LEB128 varints, IEEE-754 float/half fields, quantized floats, byte alignment, bounds/error state. Non-throwing and allocation-free; complements the byte-oriented `DataStream` |
| `ElvEventPoller.hpp` | Portable readiness seam: `NativeHandle`, `EventMask`, `PollFlags`, `PollEvent`, and the abstract `EventPoller` (`add`/`modify`/`remove`/`wait`). epoll/kqueue/WSAPoll implementations live in Euphemy |
| `ElvEventLoopWakeup.hpp` | Portable wakeup seam used by `PollingEventLoop`; concrete eventfd/pipe/socket-pair implementations live in Euphemy |
| `ElvAsyncCompletion.hpp` | Backend-neutral `AsyncCompletion` record shared by io_uring and IOCP wrappers |

### `Net/` - transport-agnostic networking foundation

The networking layer follows the same seam as the device layer: Elvavena owns the
protocol and the interfaces; the OS socket implementation lives in Euphemy.

| Header | Contents |
|---|---|
| `ElvNetTypes.hpp` | `PeerId`, `ChannelId`, `Tick`, `NetSequence`, `NetError`, `SessionState`, `ChannelReliability`, `SendOptions`, opaque aligned `Endpoint` + hash, `NetEndian` |
| `ElvNetSequence.hpp` | Wrap-safe 16-bit sequence comparison and a 64-entry `AckBitfield` |
| `ElvDatagramTransport.hpp` | Abstract non-blocking datagram transport (`sendTo`, `receiveFrom`, `resolve`, `localEndpoint`, `maxDatagramSize`) |
| `ElvLoopbackTransport.hpp` | Deterministic virtual wire with configurable latency, jitter, loss and duplication; seeded RNG for replayable tests |
| `ElvNetProtocol.hpp` | Packet header, handshake and fragment layouts; magic/version validation; bit-level encoding helpers |
| `ElvNetFragment.hpp` | Bounded reassembly of one fragmented message |
| `ElvNetCongestion.hpp` | `CongestionController` seam, `NullCongestion`, and a per-peer `FixedRateCongestion` token bucket |
| `ElvNetSession.hpp` / `.cpp` | Poll-driven session: handshake with retry, per-channel reliability, ack windows, fragmentation, heartbeat/timeout, ack/loss tags, event sink. Owns no thread and no tick loop |
| `ElvNetClockSync.hpp` | `ClockSample`, EWMA `ClockEstimator` (lowest-RTT sample wins) and `TickClock` time/tick conversion |
| `ElvNetRpc.hpp` | Typed RPC argument codecs, `RpcRegistry` (typed binding, type-erased dispatch), `RpcEncoder`, `RpcDispatcher`, `RpcPolicy`, routing-target enum |
| `ElvNetId.hpp` | 20-bit index + 12-bit generation `NetId` and recycling `NetIdAllocator`; stale handles fail |
| `ElvNetReplicationSchema.hpp` / `.cpp` | `ComponentDefinition`, `ComponentOps`, `EntityKindDefinition`, validated `ComponentRegistry`, schema hash; object-schema and trivial-POD codecs |
| `ElvNetReplicationRegistry.hpp` | Non-owning `EntityRecord` slots, component pointers/mask, spawn/destroy, generation-checked lookup, iteration |
| `ElvNetInterest.hpp` | Batch `InterestManager` hook plus `ReplicateAllInterest` and `OwnerInterest` defaults |
| `ElvNetSnapshot.hpp` | Spawn/update/despawn snapshot framing, byte-framed component payloads, ack-tag baseline promotion/rejection, interest-filtered writes, zero-copy reader payload views |
| `ElvNetInterpolation.hpp` | `InterpolationOps`, arithmetic and explicit schema-field interpolation, generic tick `JitterBuffer`, snapshot payload jitter buffer |
| `ElvNetPrediction.hpp` | Ordered input queues, delayed input buffer, checksum history, `PredictionSession` mismatch detection, `replayInputs` |
| `ElvNetLockstep.hpp` | All-peers-ready input collector, per-peer checksum exchange, first divergent tick |
| `ElvNetReplicationPipeline.hpp` | Reference per-peer server writers with interest filtering/ack routing, client receive/jitter buffer |

`docs/NETCODE.md` records the layer boundaries, the wire contract and the remaining
milestones. `docs/PLATFORM.md` records the event-poller/completion-wrapper split and
the two ring-buffer contracts.

### `Util/` - containers, allocators, algorithms and runtime helpers

| Area | Headers |
|---|---|
| Allocators / memory | `ElvAlexandrescuResource.hpp` (alignment-correct PMR bridge over Alexandrescu allocators), `ElvAllocatorBasic.hpp` (allocator concept and smart-pointer adapters), `ElvBitmapAllocator.hpp`, `ElvFreelist.hpp` (`FixedFreeListAllocator`, `ContiguousFreeListAllocator`, `FreelistMemoryManager`), `ElvStackAllocator.hpp`, `ElvHeapArray.hpp`, `ElvChunkyArray.hpp`, `ElvIntrusiveArray.hpp`, `ElvIdPool.hpp` (thread-safe ID recycling) |
| Containers | `ElvBinaryTree.hpp`, `ElvContainerBasic.hpp`, `ElvDoubleBufferedVector.hpp`, `ElvRingBuffer.hpp` (fixed-capacity optional-slot history buffer), `ElvContiguousRingBuffer.hpp` (streaming contiguous read/write spans; the extracted Haruka streamer buffer), `ElvStringhashMap.hpp`, `ElvThreadsafeContainer.hpp`, `ElvThreadsafeQueue.hpp`, `ElvObserv.hpp` (observer pattern) |
| Algorithms / math | `ElvFixedPoint.hpp`, `ElvFixedPointMath.hpp`, `ElvInterpolation.hpp`, `ElvMathUtil.hpp`, `ElvNormDenorm.hpp`, `ElvDither.hpp`, `ElvEdgeFunction.hpp`, `ElvFuzzy.hpp` (fuzzy logic), `ElvSpanHelpers.hpp`, `ElvIntegralIterator.hpp`, `ElvContinuousIterator.hpp`, `ElvEndianness.hpp`, `ElvChecksum.hpp` (CRC-32C and FNV-1a for stable wire checksums) |
| Runtime / platform | `ElvDynamicLib.hpp` (`dlopen`/`LoadLibrary` wrapper), `ElvEventLoop.hpp` (single background command thread with `enqueue`/`enqueueAsync`/`enqueueSync`), `ElvPollingEventLoop.hpp` (command pump + `EventPoller`/wakeup reactor and function-pointer readiness handlers), `ElvThreadPool.hpp`, `ElvAsyncExecutor.hpp` (executor concept), `ElvSignal.hpp` (`Signal`/`Connection`/`ScopedConnection` — the in-tree replacement for the third-party `sigslot.h`), `ElvException.hpp`, `ElvUtilGlobals.hpp` (API visibility macros, `DEFINE_PTR`/`DEFINE_CLASS`/`DEFINE_STRUCT`) |
| RAII / scope cleanup | `ElvScopeGuard.hpp` (`ScopeGuard<Fn>`, `ScopeExitMode`, `makeScopeGuard`/`atScopeExit`, and the `ELV_SCOPE_EXIT` / `ELV_SCOPE_FAIL` / `ELV_SCOPE_SUCCESS` macros; failure/success modes use `std::uncaught_exceptions`), `ElvDeferStack.hpp` (`DeferStack`: fixed-capacity, non-allocating LIFO cleanup thunks in caller-owned storage; `defer`, LIFO `dismiss`, `runAll`) |
| Third-party | `half.hpp` - vendored half-float implementation |

## TODOs and known gaps

The sources contain no explicit `TODO`/`FIXME` markers; the following are the known
limits of the current implementation:

- [ ] `EventLoop` remains the pure command pump; `PollingEventLoop` is the second
      variant wired to an `EventPoller` and an `EventLoopWakeup` (eventfd/pipe on
      POSIX, loopback socket pair on Windows). Remaining polish: per-handle timers
      and a cancellation token for an in-flight poll wait.
- [ ] The readiness seam has epoll, kqueue and WSAPoll backends. A `select(2)`
      fallback is still missing; kqueue/WSAPoll need platform CI.
- [ ] `IoUring` is optional at compile time and runtime: liburing is linked only when
      present, and Docker/Android/ChromeOS may disable `io_uring_setup` even when it
      is linked. The epoll/kqueue/WSAPoll path is the fallback for readiness; blocking
      `Elv::Io::Async<ThreadPool>` remains the fallback for file I/O.
- [ ] `ContiguousRingBuffer` and `RingBuffer` are intentionally separate: the first
      exposes contiguous read/write spans for streaming, the second stores optional
      slots for histories and supports move-only/non-default-constructible types.
      Keep both unless a future design can preserve both contracts in one type.
- [ ] There is no concrete `Elv::Io::Device` or `Elv::Io::System` implementation in this
      module; the concrete files, sockets, compression and encryption devices live in
      Euphemy. Elvavena only defines the interfaces and the generic proxies.
- [ ] `DataStream` and `ElvDataSerializationSchema` are binary serialization
      primitives only. The schema has no version negotiation or reflection; higher
      layers still define their own stream operators where a bespoke format is needed.
- [ ] The networking foundation now includes snapshots/deltas, interpolation/jitter,
      prediction/rollback bookkeeping, lockstep input/checksum exchange and the
      reference pipeline. Remaining: broader WinSock/IPv6 coverage and an optional
      `FieldDeltaOps` optimization (the current delta compares encoded component
      payloads). `Session` has no encryption or authentication and no NAT traversal;
      the connection token is anti-spoofing, not security.
- [ ] Fragmented messages are delivered when reassembly completes, not in
      message-ordered position relative to other fragmented messages. Strict
      ordering across fragmented messages remains a refinement.
- [ ] The networking code is covered by `UnitTests` (`[net]`, plus `[net][socket]`
      for the localhost UDP path), but there is still no dedicated test target in
      the Elvavena module itself.
- [ ] `ElvAllocatorBasic.hpp`, `ElvBinaryTree.hpp`, `ElvContainerBasic.hpp`,
      `ElvDither.hpp`, `ElvEdgeFunction.hpp`, `ElvFuzzy.hpp`, `ElvObserv.hpp`,
      `ElvStringhashMap.hpp`, `ElvThreadsafeContainer.hpp` and `ElvThreadsafeQueue.hpp`
      have no direct test in `Sandbox` yet.
- [ ] `ElvScopeGuard.hpp` and `ElvDeferStack.hpp` are newly merged and have no direct
      test in `Sandbox` yet. Their contracts to keep in mind: scope-guard callables
      must not throw, `dismiss()`/`activate()` are the only state changes, and
      `DeferStack` requires LIFO dismissal and never allocates (a full stack returns an
      invalid token instead).
