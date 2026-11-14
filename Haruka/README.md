# Haruka

Haruka is the audio engine of MhLib. It provides a pull-based audio graph: `Playable`
nodes write frames into an output packet, and mixers, panners, resamplers and streamers
compose those nodes. Decoding is delegated to Euphemy (libsndfile, libopenmpt,
libsamplerate).

**Status: partial.** The graph primitives, the streamers and the effect chain work;
there is no audio output device yet (see the TODO list).

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Haruka` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_HARUKA_EXPORT` while building, `MH_HARUKA_API` for consumers |
| Consumer hook | `include(Haruka/Haruka.pri)` adds the include path and `-lHaruka` |
| Dependency | Elvavena (includes), Euphemy (codec types, resampler, FFT convolution), libsndfile, libsamplerate, libopenmpt |

`HarukaLib.hpp` currently only defines the export macro; there is no single umbrella
header that includes every Haruka component.

## Implemented

### `Core/` - graph nodes and DSP

| Header | Contents |
|---|---|
| `HrkAudioPacket.hpp` | `Input`/`Output` packet types, `PlayStatus`, `InterleavingType`, `SAMPLE_RATE_DONT_CARE` and the re-exported frame/sample types |
| `HrkPlayable.hpp` | `Playable` - the abstract pull-based node (`outputTo(Output&)`) |
| `HrkProxyPlayable.hpp` | `ProxyPlayable` - base for nodes that wrap another playable, with non-owning and `shared_ptr`-owning targets |
| `HrkMixer.hpp` / `.cpp` | `Mixer` - mixes child playables by linear gain using a double-buffered PMR scratch buffer; validates channel count, sample rate and interleaving |
| `HrkMatrixPanner.hpp` / `.cpp` | `MatrixPanner` - caller-editable in/out channel matrix with editable coefficients |
| `HrkAmbisonicPanner.hpp` / `.cpp` | `AmbisonicPanner` - derives four speaker gains (LF/LB/RF/RB) from polar position (`theta`, `phi`, inverse distance) |
| `HrkResampler.hpp` / `.cpp` | `Resampler` - proxy node that resamples its source through Euphemy's `FullResampler` (libsamplerate) |
| `HrkAudioError.hpp` / `.cpp` | `ChannelCountMismatchError`, `SamplerateMismatchError`, `InterleavingMismatchError` |

### `IO/` - playback state and streamers

| Header | Contents |
|---|---|
| `HrkSoundBuffer.hpp` | `SoundBufferMetadata` and the intrusive, reference-counted `SoundBufferHandle` (`Elv::Util::IntrusiveArray<float, ...>`) |
| `HrkPlaybackState.hpp` / `.cpp` | `PlaybackState` - cursor, play status and repeat flag |
| `HrkPlaybackStatePlayable.hpp` / `.cpp` | `PlaybackStatePlayable` - combines `Playable` with `IHasPlaybackState` |
| `HrkSoundSource.hpp` / `.cpp` | `SoundSource` - plays an in-memory `SoundBufferHandle`, honouring cursor/repeat and validating the output format |
| `HrkRingBufferedStreamer.hpp` / `.cpp` | `RingBufferedStreamer` - base for pull-based decoders with a seekable ring buffer and decoder/state-cursor synchronisation |
| `HrkSoundStreamer.hpp` / `.cpp` | `SoundStreamer` - `RingBufferedStreamer` backed by `Euph::Media::Audio::SoundFile` (libsndfile) |
| `HrkModuleStreamer.hpp` / `.cpp` | `ModuleStreamer` - `RingBufferedStreamer` backed by `Euph::Media::Audio::ModuleRenderer` (libopenmpt) |

### `FX/` - the effect chain

| Header | Contents |
|---|---|
| `HrkAudioEffect.hpp` | `Effect` - block-oriented processing contract: one `process(const ModificationStream&)` call per chunk, plus `getRequiredInterleaving()` so the slot knows whether it must deinterlace |
| `HrkSampleLevelEffect.hpp` | `SampleLevelEffect<Functor>` and `makeSampleLevelEffect()` - adapts a per-sample functor into a block effect without a virtual call per sample |
| `HrkClampEffect.hpp` | `FX::Clamp` / `FX::ClampEffect` - normalizing hard limiter (`sign(in) * min(|in|, level) / level`) |
| `HrkCrushEffect.hpp` | `FX::Crush` / `FX::CrushEffect` - quantization ("bit crush") into a fixed step count |
| `HrkEffectSlot.hpp` / `.cpp` | `EffectSlot` - `ProxyPlayable` that pulls its source through a serial dry/wet effect chain; contiguous `std::pmr` storage, cached planar requirement, fixed scratch buffers, interleaved *and* channel-planar output |
| `HrkImpulseResponse.hpp` | `ImpulseResponse` - channel-planar float storage with an explicit sample rate (replaces `fftconvolver::IrBuffer`) |
| `HrkImpulseGenerator.hpp` / `.cpp` | `createLowpassFilter()`, `createHighpassFilter()`, `createBandpassFilter()`, `createBandrejectFilter()`, `createDelay()` - offline Blackman-windowed-sinc kernel synthesis |
| `HrkConvolver.hpp` / `.cpp` | `Convolver` - one Euphemy `FFTConvolver` per channel, driven by per-channel impulse responses (reverb, cabinet, delay, filters) |

## TODOs and known gaps

- [ ] No audio output device/backend (ALSA, PulseAudio, PipeWire, SDL, ...). Haruka is a
      pull-based graph only; the application must pull frames itself.
- [ ] No dedicated reverb/echo node. `FX::Convolver` plus `FX::createDelay()` and the
      filter generators cover the building blocks, but feedback loops and a "reverb"
      preset are not modelled.
- [ ] `EffectSlot` has no automatic makeup gain and no ramp on `wet` changes; a mixing
      console built on it must smooth `wet` itself to avoid zipper noise.
- [ ] No listener or 3D scene graph. `AmbisonicPanner` and `MatrixPanner` take manual
      gains/angles; there is no source/listener position model.
- [ ] `Mixer` requires every child to match its own channel count, sample rate and
      interleaving (it throws on mismatch). Automatic per-child resampling is not
      implemented; insert a `Resampler` node manually.
- [ ] Test coverage is limited to `testAudioEffects()` in the Test app (effect chain,
      dry/wet blending, planar routing, IR generation, convolution). Panners,
      resampler, streamers and source playback are still untested.
- [ ] No public umbrella header; consumers include individual `Haruka/...` headers.
