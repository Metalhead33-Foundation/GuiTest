# Euphemy

Euphemy is the media and platform layer of MhLib. It builds on Elvavena and provides
concrete I/O devices, image and audio codecs, the INI-style configuration format, an
optional set of PMR arenas, and the CPU-side asset streaming runtime.

**Status: implemented, with a few known gaps.** All subsystems below have working code
and compiled translation units. The gaps are listed at the end of this file.

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Euphemy` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_EUPH_EXPORT` while building, `MH_EUPH_API` for consumers |
| Consumer hook | `include(Euphemy/Euphemy.pri)` adds the include path and `-lEuphemy` |
| Internal dependency | Elvavena (`-lElvavena`) |
| External libraries | libsndfile, libsamplerate, libopenmpt, libpng16, gif, turbojpeg, libwebp (+decoder/demux/mux), zstd, snappy, PhysFS, OpenSSL (`ssl`, `crypto`); optional `liburing` |

## Implemented

### `Io/` - concrete device and filesystem implementations

| Header | Contents |
|---|---|
| `EuphFile.hpp` | `File` (POSIX/Win32 file device), `MemoryMappedFile`, `Filesystem` (`Elv::Io::System` implementation) |
| `EuphPlatformDependentFileBase.hpp` | Shared platform file-base class used by the file devices |
| `EuphBufferDevice.hpp` / `EuphConstBufferDevice.hpp` | Growable buffer-backed device and read-only const-buffer device |
| `EuphMemoryDevice.hpp` / `EuphPmrMemoryDevice.hpp` | In-memory device backed by `std::vector` or a PMR resource |
| `EuphMemoryMapped.hpp` | Memory mapping primitive (anonymous, file and temporary mappings) |
| `EuphNullDevice.hpp` | Sink/discard device |
| `EuphRandomDevice.hpp` | Random-byte source device |
| `EuphStdfileWrapper.hpp` | Adapts `FILE*` / standard streams to `Elv::Io::Device` |
| `EuphTempFile.hpp` | Temporary-file creation (`mkstemp`, `O_TMPFILE`, ...) and RAII cleanup |
| `EuphSocket.hpp` | Sockets as `Elv::Io::Device`: address families, TCP/UDP, connect/bind/listen/accept, opaque `Elv::Net::Endpoint` conversion, non-blocking datagram send/receive. Owns WinSock lifetime and frees `addrinfo` |
| `EuphZstd.hpp` / `.cpp` | `ZstdCompressor` (write proxy) and `ZstdDecompressor` (read proxy) |
| `EuphSnappy.hpp` / `.cpp` | `SnappyCompressor` (write proxy) and `SnappyDecompressor` (read proxy) over a small chunked container (`"SNPY"` magic, per-chunk compressed/uncompressed sizes) |
| `EuphCryptoProxyDevice.hpp` | `CryptoWriteDeviceProxy` / `CryptoReadDeviceProxy` plus the `CipherType` enum (OpenSSL-backed) |
| `PhysFS.hpp` | `PhysFS` namespace wrapper around the PhysFS virtual filesystem |

### `Net/` - concrete transports over the platform sockets

| Header | Contents |
|---|---|
| `EuphUdpTransport.hpp` | `UdpTransport` implements `Elv::Net::DatagramTransport` over `Euph::Io::Socket`: unbound client sockets, server bind with ephemeral ports, non-blocking receive and `resolve`/`localEndpoint`. This is the bridge between Elvavena's session layer and the OS |

The transport is intentionally thin. Protocol state (reliability, fragmentation,
heartbeats) lives in `Elvavena/Net/ElvNetSession.*`; encryption/authentication and NAT
traversal are not part of this adapter.

### `Platform/` - event pollers and completion wrappers

| Header | Contents |
|---|---|
| `EuphEventPollerFactory.hpp` | `createEventPoller()` selects epoll on Linux, kqueue on Apple/BSD and WSAPoll on Windows; returns null when unavailable. `createIoUringIfAvailable()` gives a runtime fallback, and `createPollingEventLoop()` wires a native poller + wakeup |
| `EuphEpollPoller.hpp` / `.cpp` | `EpollPoller` implements `Elv::Io::EventPoller`: add/modify/remove, level/edge/one-shot flags, poll timeout and event-token mapping |
| `EuphKqueuePoller.hpp` / `.cpp` | `KqueuePoller` maps read/write filters, `EV_CLEAR`/`EV_ONESHOT` and EOF/error flags to the same seam; unavailable stub elsewhere |
| `EuphWSAPollPoller.hpp` / `.cpp` | `WSAPollPoller` implements the same seam for Windows sockets; non-Windows builds provide an unavailable stub |
| `EuphEventLoopWakeup.hpp` / `.cpp` | `WakeupEvent` uses eventfd/pipe on POSIX and a loopback UDP pair on Windows; `createWakeupEvent()` returns null when unsupported |
| `EuphIoUring.hpp` / `.cpp` | `IoUring` RAII wrapper over liburing: NOP/read/write/accept/connect/poll/timeout, registered fixed buffers, batched completion wait and runtime availability probe. Compiled only against liburing when detected; otherwise stubbed with `-ENOSYS` |
| `EuphIocp.hpp` / `.cpp` | `Iocp` RAII wrapper over Windows I/O completion ports: associate handles, post packets, batched wait. Overlapped accept/connect/send/receive are caller-submitted operations delivered through the port |

`EventPoller` is readiness (which handle is ready); `IoUring`/`Iocp` are completion
(one record per submitted operation). They are separate because forcing a completion
API into a readiness interface loses the operation result and buffer ownership model.
`liburing` is optional in the qmake project; Docker, Android and ChromeOS can also
disable `io_uring_setup` at runtime, so callers must fall back to epoll/WSAPoll for
readiness or `Elv::Io::Async<ThreadPool>` for blocking file work.

### `Media/Image/` - image containers and codecs

| Header | Contents |
|---|---|
| `EuphImageType.hpp` | `Format` enum, `ImageView`, `TextureFiltering`, `AlphaBlending`, `Wrap` |
| `EuphPixelFormat.hpp` | Typed pixel structs (`PixelRGBA`, `PixelRGB565`, `PixelARGB4444`, ...) and the `PixelConcept` |
| `EuphImage.hpp` | `ImageDimensions`, `Image2D<Pixel>`, `PalettedImage2D<Pixel>`, `AnyImage2D` variant, conversion helpers to/from `DecodeTarget` |
| `EuphImageDecodeTarget.hpp` | Decoder-facing `Palette`, `Frame`, `DecodeTarget` (mip/frame list, palette, animation flag, delay) |
| `EuphStandardColour.hpp` | `StandardColourEnum` and the name-to-colour map |
| `EuphPNG.hpp` / `.cpp` | PNG decode **and** encode (libpng) |
| `EuphJPEG.hpp` / `.cpp` | JPEG decode **and** encode (turbojpeg, buffer and device overloads) |
| `EuphTGA.hpp` / `.cpp` | TGA decode **and** encode, including extension information |
| `EuphWEBP.hpp` / `.cpp` | WebP decode **and** encode (buffer and device overloads) |
| `EuphGIF.hpp` / `.cpp` | GIF **decode only** |
| `EuphDDS.hpp` / `.cpp` | DDS **decode only**, including block-compressed payloads |

### `Media/Audio/` - audio codecs and DSP primitives

| Header | Contents |
|---|---|
| `EuphAudioIterator.hpp` | `ChannelCount`, `FrameCount`, `FrameIndex`, `SampleCount`, `SampleRate` and frame/sample conversion helpers |
| `EuphSoundFile.hpp` / `.cpp` | `SoundFile` - libsndfile wrapper with typed `read`/`write`, `readf`/`writef`, seeking, format query and metadata setters, plus the large `SoundFormat` enum |
| `EuphModuleRenderer.hpp` / `.cpp` | `ModuleRenderer` - libopenmpt-backed tracker/module rendering (MOD, S3M, XM, IT, ...) |
| `EuphResampler.hpp` / `.cpp` | libsamplerate wrappers: `SimpleResampler`, `FullResampler`, `CallbackResampler`/`CallbackInterface`, `ResampleType` |
| `EuphConvolver.hpp` / `.cpp` | RAII wrappers over the vendored FFTConvolver: `AudioFFT`, `FFTConvolver`, `TwoStageFFTConvolver` |
| `FFTConvolver/` | Vendored third-party FFT convolution library (see its own README and COPYING) |

### `Config/` - configuration format

| Header | Contents |
|---|---|
| `EuphConfigurationElement.hpp` | `SizedString` (shared string pool), `ConfigurationElementType`, `ConfigurationElement` |
| `EuphConfiguration.hpp` | `ConfigurationSection` and `Configuration` (sectioned `key=value` text, parse from `std::istream`/`std::string`, typed get/set, stream output) |
| `EuphLib.hpp` | `MH_EUPH_API` export macro |

### `Memory/` - optional PMR arenas

| Header | Contents |
|---|---|
| `EuphMemory.hpp` / `.cpp` | `AllocatorSubsystem` (`SCRATCH`, `AUDIO`, `VIDEO`, `SCRIPT`), `PoolSizes` defaults, and `MemoryManager`: explicit `initialize(PoolSizes)` / `initialize(Configuration&)`, `resource(AllocatorSubsystem)` returning a `std::pmr::memory_resource`, and a `std::pmr::get_default_resource()` fallback until initialized |

The arenas are opt-in. Nothing is allocated until `MemoryManager::initialize` is
called, there is no global configuration symbol, and the default pool sizes total
roughly 512 MiB, so callers should pass a `PoolSizes` value suited to their workload.
The resources are backed by `Elv::Util::FreelistMemoryManager` and bridged into PMR
through `Elv::Util::AlexandrescuResource`.

### `Asset/` - CPU-side asset streaming runtime

| Header | Contents |
|---|---|
| `EuphAssetTypes.hpp` | `AssetId`, `ResidencyState`, `FailureReason`, `ResidencyFlag`, `Compression`, `StreamPriority`, `AssetRecord` |
| `EuphAssetRuntime.hpp` / `.cpp` | `AssetCatalog`, `DependencyGraph` (cycle/missing-dependency detection), `ResourceRegistry` (typed, generation-checked handles and payloads), `ResidencyManager` (budgets, pinning, eviction), `StreamingScheduler` (dependency-aware async load -> decode -> commit) |

This layer is deliberately CPU-only: renderer/GPU residency is an integration concern,
implemented by Lotte's `GpuAssetStreamer`. The header documents the whole design in its
`@section`s (overview, pipeline, threading, object lifetime, payload ownership, known
limitations) and every public and private member; the residency state machine and its legal
transitions are documented on `ResidencyState` in `EuphAssetTypes.hpp`.

## TODOs and known gaps

- [ ] `BufferDevice::isValid()` in `Io/EuphBufferDevice.hpp` (around line 791) is a
      placeholder that always returns `true`; real validation is still missing.
- [ ] GIF and DDS are decode-only. There is no GIF or DDS encoder.
- [ ] The asset runtime has no GPU knowledge by design; use Lotte's
      `Lotte/Asset/LteGpuAssetStreamer.hpp` for upload/eviction scheduling.
- [ ] The `MemoryManager` arenas are optional and currently unused by the rest of the
      tree. Wire them into a concrete consumer (asset streaming payloads or per-frame
      scratch) before treating them as production code.
- [ ] Codec coverage is uneven: `UnitTests` round-trips PNG, TGA, zstd, Snappy, the font
      containers and the configuration format, but JPEG/WebP (lossy, buffer-based entry
      points) and GIF/DDS (decode only) have no automated check.
- [ ] The platform wrappers are only exercised on Linux: epoll, the polling loop and
      the io_uring availability path have tests; WSAPoll, IOCP and kqueue need
      Windows/BSD CI. A `select(2)` fallback is still missing.
- [ ] `liburing` is optional at compile time, but a build without it still compiles
      `EuphIoUring.cpp` as a stub. Runtime `io_uring_setup` failures (Docker seccomp,
      Android, ChromeOS) are detected by `IoUring::isValid()`/`runtimeAvailable()` and
      must be handled by falling back to epoll/kqueue/WSAPoll or a thread-pool I/O path.
- [ ] io_uring supports accept/connect and registered fixed buffers; `sendmsg`/
      `recvmsg` receive-buffer rings and buffer-update tags remain future work.
- [ ] The UDP transport's automated coverage is a Linux localhost smoke test
      (`UnitTests` tag `[net][socket]`). The WinSock branch, IPv6-specific paths and
      platform socket-option behaviour are not exercised by CI yet.
- [ ] `UdpTransport` is plaintext. Add authentication/encryption (DTLS or a
      game-level handshake) above the transport before exposing it to untrusted
      networks; the Elvavena session token is not a security mechanism.
