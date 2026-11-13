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
| External libraries | libsndfile, libsamplerate, libopenmpt, libpng16, gif, turbojpeg, libwebp (+decoder/demux/mux), zstd, PhysFS, OpenSSL (`ssl`, `crypto`) |

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
| `EuphSocket.hpp` | POSIX sockets as `Elv::Io::Device` (address family, socket type, connect/listen/accept) |
| `EuphZstd.hpp` | `ZstdCompressor` (write proxy) and `ZstdDecompressor` (read proxy) |
| `EuphCryptoProxyDevice.hpp` | `CryptoWriteDeviceProxy` / `CryptoReadDeviceProxy` plus the `CipherType` enum (OpenSSL-backed) |
| `PhysFS.hpp` | `PhysFS` namespace wrapper around the PhysFS virtual filesystem |

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
implemented by Lotte's `GpuAssetStreamer`.

## TODOs and known gaps

- [ ] `BufferDevice::isValid()` in `Io/EuphBufferDevice.hpp` (around line 791) is a
      placeholder that always returns `true`; real validation is still missing.
- [ ] GIF and DDS are decode-only. There is no GIF or DDS encoder.
- [ ] The asset runtime has no GPU knowledge by design; use Lotte's
      `Lotte/Asset/LteGpuAssetStreamer.hpp` for upload/eviction scheduling.
- [ ] `Euphemy.pro` and `Euphemy.pri` still add the vestigial NSPR include path
      (`/usr/include/nspr`) after the MozJS dependency was dropped, and they link
      `X11`, `SDL2`, `SDL2_image`, `freetype`, `snappy`, `glslang`, `SPIRV`,
      `spirv-cross-*` and `yaml-cpp`, which no current Euphemy source includes. This is
      stale build metadata and should be pruned.
- [ ] The `MemoryManager` arenas are optional and currently unused by the rest of the
      tree. Wire them into a concrete consumer (asset streaming payloads or per-frame
      scratch) before treating them as production code.
- [ ] There is no direct test for most codecs or the asset scheduler beyond the
      TGA/PNG and asset-runtime tests in the Test app.
