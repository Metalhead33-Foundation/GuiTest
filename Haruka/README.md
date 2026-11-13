# Haruka

Haruka is the audio engine of MhLib. It provides a pull-based audio graph: `Playable`
nodes write frames into an output packet, and mixers, panners, resamplers and streamers
compose those nodes. Decoding is delegated to Euphemy (libsndfile, libopenmpt,
libsamplerate).

**Status: partial.** The graph primitives and the streamers work; there is no audio
output device and there are no effect nodes yet (see the TODO list).

## Build and consumption

| Item | Value |
|---|---|
| qmake target | `Haruka` (`TEMPLATE = lib`, `CONFIG += shared c++2a`, no Qt) |
| Export macro | `MH_HARUKA_EXPORT` while building, `MH_HARUKA_API` for consumers |
| Consumer hook | `include(Haruka/Haruka.pri)` adds the include path and `-lHaruka` |
| Dependencies | Elvavena (includes), Euphemy (codec types), libsndfile, libsamplerate, libopenmpt |

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

## TODOs and known gaps

- [ ] No audio output device/backend (ALSA, PulseAudio, PipeWire, SDL, ...). Haruka is a
      pull-based graph only; the application must pull frames itself.
- [ ] No dedicated effect nodes. Euphemy exposes FFT convolution wrappers, but there is
      no Haruka `Convolver`/`Reverb`/`Delay`/`Echo` node wiring them into the graph.
- [ ] No listener or 3D scene graph. `AmbisonicPanner` and `MatrixPanner` take manual
      gains/angles; there is no source/listener position model.
- [ ] `Mixer` requires every child to match its own channel count, sample rate and
      interleaving (it throws on mismatch). Automatic per-child resampling is not
      implemented; insert a `Resampler` node manually.
- [ ] No tests. `Test/test.cpp` does not include Haruka, so none of this module is
      exercised by the Test app.
- [ ] No public umbrella header; consumers include individual `Haruka/...` headers.
