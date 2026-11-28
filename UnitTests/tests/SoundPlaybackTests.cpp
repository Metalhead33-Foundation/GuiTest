/**
 * @file SoundPlaybackTests.cpp
 * @brief Tests for the sound clip (Euphemy), the buffer builder and player (Haruka).
 *
 * The clip is exercised through a real WAV built in memory and read back through
 * libsndfile's virtual I/O, which is the same path a loaded file takes. The player is
 * driven by a mixer pull, so the "a finished voice is retired" rule is verified where
 * it actually happens instead of being asserted from the outside.
 */
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <Euphemy/Io/EuphBufferDevice.hpp>
#include <Euphemy/Media/Audio/EuphSoundClip.hpp>
#include <Haruka/Core/HrkMixer.hpp>
#include <Haruka/Core/HrkSoundPlayer.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <vector>

namespace {

using namespace Euph::Media::Audio;

/** @brief Appends a little-endian 16-bit value. */
void appendU16(std::vector<std::byte>& bytes, std::uint16_t value)
{
	bytes.push_back(static_cast<std::byte>(value & 0xFFu));
	bytes.push_back(static_cast<std::byte>((value >> 8) & 0xFFu));
}

/** @brief Appends a little-endian 32-bit value. */
void appendU32(std::vector<std::byte>& bytes, std::uint32_t value)
{
	for (int shift = 0; shift < 32; shift += 8) {
		bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFFu));
	}
}

/** @brief Appends an ASCII chunk identifier. */
void appendId(std::vector<std::byte>& bytes, const char* id)
{
	for (int index = 0; index < 4; ++index) {
		bytes.push_back(static_cast<std::byte>(id[index]));
	}
}

/**
 * @brief Builds a 16-bit PCM WAV in memory.
 * @param sampleRate Rate to declare.
 * @param channels Channel count to declare.
 * @param frames Number of frames to synthesise.
 * @return The complete file image.
 */
std::vector<std::byte> makeWav(std::uint32_t sampleRate, std::uint16_t channels, std::size_t frames)
{
	const std::uint32_t dataBytes = std::uint32_t(frames * channels * 2u);
	std::vector<std::byte> bytes;
	bytes.reserve(44u + dataBytes);
	appendId(bytes, "RIFF");
	appendU32(bytes, 36u + dataBytes);
	appendId(bytes, "WAVE");
	appendId(bytes, "fmt ");
	appendU32(bytes, 16u);
	appendU16(bytes, 1u); // PCM
	appendU16(bytes, channels);
	appendU32(bytes, sampleRate);
	appendU32(bytes, sampleRate * std::uint32_t(channels) * 2u);
	appendU16(bytes, std::uint16_t(channels * 2u));
	appendU16(bytes, 16u);
	appendId(bytes, "data");
	appendU32(bytes, dataBytes);
	for (std::size_t frame = 0; frame < frames; ++frame) {
		const float phase = float(frame) / 32.0f;
		const std::int16_t sample =
			std::int16_t(std::sin(phase) * 12000.0f);
		for (std::uint16_t channel = 0; channel < channels; ++channel) {
			appendU16(bytes, std::uint16_t(sample));
		}
	}
	return bytes;
}

/** @brief Wraps a byte image in a read-only device the clip loader can adopt. */
Elv::Io::uDevice openMemory(const std::vector<std::byte>& bytes)
{
	// The read-only wrapper reports `Mode::READ`, which is the branch `SoundFile` opens
	// with `SFM_READ`; a writable buffer would ask libsndfile for SFM_RDWR instead.
	auto wrapper = Elv::Util::pmr_make_unique<Euph::Io::ReadOnlyBufferWrapper>(
		std::pmr::get_default_resource(), bytes.data(), bytes.size());
	return Elv::Io::uDevice(std::move(wrapper));
}

} // namespace

TEST_CASE("SoundClip decodes a WAV and reports its format", "[audio][soundclip]")
{
	const std::vector<std::byte> wav = makeWav(8000u, 1u, 400u);
	const SoundClip clip = SoundClip::load(openMemory(wav));

	REQUIRE(clip.valid());
	REQUIRE(clip.frameCount().var == 400u);
	REQUIRE(clip.sampleCount().var == 400u);
	REQUIRE(clip.channels().var == 1u);
	REQUIRE(clip.sampleRate().var == 8000u);
	REQUIRE(clip.duration() == Catch::Approx(0.05).epsilon(0.001));
	REQUIRE(clip.data() != nullptr);
}

TEST_CASE("SoundClip conforms channels and rate in one pass", "[audio][soundclip]")
{
	const std::vector<std::byte> wav = makeWav(8000u, 1u, 800u);
	SoundClip clip = SoundClip::load(openMemory(wav));
	REQUIRE(clip.valid());

	// A mono click against a stereo 48 kHz mixer is exactly the case a one-shot has to
	// handle: `SoundSource` would throw on the mismatch from the audio callback.
	REQUIRE(clip.conformTo(ChannelCount(2u), SampleRate(48000u)));
	REQUIRE(clip.channels().var == 2u);
	REQUIRE(clip.sampleRate().var == 48000u);
	// Six times the frames, within the converter's ramp.
	REQUIRE(clip.frameCount().var > 800u * 5u);
	REQUIRE(clip.frameCount().var < 800u * 7u);
	REQUIRE(clip.sampleCount().var == clip.frameCount().var * 2u);

	// Conforming again is a no-op: nothing changes and nothing is reported.
	REQUIRE_FALSE(clip.conformTo(ChannelCount(2u), SampleRate(48000u)));
}

TEST_CASE("SoundClip maps mono to every channel and folds back to mono", "[audio][soundclip]")
{
	// A mono clip with a known ramp, so the duplication and the average are checkable.
	std::vector<float> samples = { 0.0f, 0.5f, 1.0f, -0.5f };
	SoundClip clip(samples, ChannelCount(1u), SampleRate(1000u));
	REQUIRE(clip.valid());

	REQUIRE(clip.mapChannels(ChannelCount(2u)));
	REQUIRE(clip.channels().var == 2u);
	REQUIRE(clip.frameCount().var == 4u);
	for (std::size_t frame = 0; frame < 4u; ++frame) {
		REQUIRE(clip.storage()[frame * 2u] == samples[frame]);
		REQUIRE(clip.storage()[frame * 2u + 1u] == samples[frame]);
	}

	REQUIRE(clip.mapChannels(ChannelCount(1u)));
	REQUIRE(clip.channels().var == 1u);
	REQUIRE(clip.frameCount().var == 4u);
	for (std::size_t frame = 0; frame < 4u; ++frame) {
		REQUIRE(clip.storage()[frame] == samples[frame]);
	}

	// Nothing to do for an already matching clip or an empty one.
	REQUIRE_FALSE(clip.mapChannels(ChannelCount(1u)));
	SoundClip empty;
	REQUIRE_FALSE(empty.mapChannels(ChannelCount(2u)));
	REQUIRE_FALSE(empty.resample(SampleRate(48000u)));
	REQUIRE_FALSE(empty.valid());
}

TEST_CASE("makeSoundBuffer copies the frames and their metadata", "[audio][buffer]")
{
	const std::vector<float> frames = { 0.1f, 0.2f, 0.3f, 0.4f };
	const Hrk::SoundBufferHandle buffer = Hrk::makeSoundBuffer(frames.data(),
		Hrk::FrameCount(2u), Hrk::ChannelCount(2u), Hrk::SampleRate(44100u));
	REQUIRE(buffer.valid());
	REQUIRE(buffer.meta().frameCount.var == 2u);
	REQUIRE(buffer.meta().channels.var == 2u);
	REQUIRE(buffer.meta().sampleRate.var == 44100u);
	REQUIRE(buffer.meta().interleaving == Hrk::InterleavingType::INTERLEAVED);
	REQUIRE(buffer.data()[0] == frames[0]);
	REQUIRE(buffer.data()[3] == frames[3]);

	// Degenerate input produces an invalid handle instead of an empty one.
	REQUIRE_FALSE(Hrk::makeSoundBuffer(nullptr, Hrk::FrameCount(2u), Hrk::ChannelCount(2u),
									   Hrk::SampleRate(44100u)).valid());
	REQUIRE_FALSE(Hrk::makeSoundBuffer(frames.data(), Hrk::FrameCount(0u), Hrk::ChannelCount(2u),
									   Hrk::SampleRate(44100u)).valid());
}

TEST_CASE("SoundPlayer registers voices, applies gains and retires finished ones",
		  "[audio][soundplayer]")
{
	using namespace Hrk;
	auto mixer = Elv::Util::pmr_make_shared<Mixer>(std::pmr::get_default_resource(),
		ChannelCount(2u), InterleavingType::INTERLEAVED, SampleRate(48000u), FrameCount(64u));
	SoundPlayer player(*mixer);
	REQUIRE(player.voiceCount() == 0u);
	REQUIRE(mixer->getChildren().empty());

	// An invalid buffer is ignored rather than started as a silent voice.
	REQUIRE_FALSE(player.play(SoundBufferHandle(), 1.0f));

	std::vector<float> frames(100u * 2u, 0.25f);
	const SoundBufferHandle buffer = makeSoundBuffer(frames.data(), FrameCount(100u),
		ChannelCount(2u), SampleRate(48000u));
	REQUIRE(player.play(buffer, 0.5f));
	REQUIRE(player.voiceCount() == 1u);
	REQUIRE(mixer->getChildren().size() == 1u);
	REQUIRE(mixer->getChildren().begin()->second == Catch::Approx(0.5f));

	// The master volume multiplies every child gain in place.
	player.setMasterVolume(0.5f);
	REQUIRE(mixer->getChildren().begin()->second == Catch::Approx(0.25f));
	player.setMuted(true);
	REQUIRE(mixer->getChildren().begin()->second == Catch::Approx(0.0f));
	player.setMuted(false);
	REQUIRE(mixer->getChildren().begin()->second == Catch::Approx(0.25f));

	// One 64-frame pull leaves 36 frames, so the voice is still alive.
	std::vector<float> output(64u * 2u, 0.0f);
	auto pull = [&](FrameCount requested) {
		Output packet{ output.data(), requested, SampleRate(48000u), ChannelCount(2u),
					   InterleavingType::INTERLEAVED };
		return mixer->outputTo(packet);
	};
	pull(FrameCount(64u));
	player.update();
	REQUIRE(player.voiceCount() == 1u);

	// The second pull exhausts the buffer; the source stops itself and the player takes
	// it out of the mixer.
	pull(FrameCount(64u));
	player.update();
	REQUIRE(player.voiceCount() == 0u);
	REQUIRE(mixer->getChildren().empty());

	// `stopAll` clears a live voice immediately.
	REQUIRE(player.play(buffer, 1.0f));
	REQUIRE(mixer->getChildren().size() == 1u);
	player.stopAll();
	REQUIRE(player.voiceCount() == 0u);
	REQUIRE(mixer->getChildren().empty());
}
