#ifndef HRKSOUNDPLAYER_HPP
#define HRKSOUNDPLAYER_HPP
/**
 * @file HrkSoundPlayer.hpp
 * @brief Fire-and-forget playback of in-memory sound buffers.
 *
 * `Hrk::Mixer` owns a `Playable` per child and keeps it forever; `Hrk::SoundSource`
 * plays a buffer and stops at its end. Something has to own the sources, register
 * them with the mixer, and take them off again - otherwise every click leaves a dead
 * voice in the child map. That something is this class.
 *
 * @section hrk_soundplayer_scope Scope
 *
 * The player is deliberately not a decoder and not a resampler. A caller prepares a
 * `SoundBufferHandle` with `Hrk::makeSoundBuffer()` (Haruka) from a
 * `Euph::Media::Audio::SoundClip` (Euphemy), which has already conformed the data to
 * the mixer's channel count and rate. `SoundSource::outputTo()` *throws* on a
 * mismatch and it runs on the audio callback, so format conversion must not be able
 * to reach this layer.
 *
 * @section hrk_soundplayer_threading Threading
 *
 * The player mutates `Mixer::getChildren()`, which the audio callback reads. Every
 * method therefore has to be called with the device locked (`SDL_LockAudioDevice`, or
 * whatever the backend's equivalent is - the sandbox's `AudioLock`). The class does
 * not lock anything itself: a player has no idea which device it is feeding, and
 * taking a lock that the caller already holds would be a deadlock.
 *
 * `SoundPlayer::update()` is the one method designed to be called once per frame; it retires
 * the voices that have reached the end of their buffer.
 */
#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkMixer.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
#include <Haruka/IO/HrkSoundSource.hpp>
#include <cstddef>
#include <memory_resource>
#include <vector>

namespace Hrk {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(SoundPlayer)

/**
 * @brief Owns the short-lived voices the mixer plays.
 *
 * A "voice" is one `Hrk::SoundSource` plus the gain it was started with. Starting the
 * same buffer twice produces two independent voices, which is exactly what two rapid
 * clicks need.
 */
class MH_HARUKA_API SoundPlayer
{
public:
	/**
	 * @brief Creates a player that feeds one mixer.
	 * @param mixer Mixer whose child map receives the voices; must outlive the player.
	 * @param memRes Memory resource for the voice list.
	 */
	explicit SoundPlayer(Mixer& mixer, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Stops every voice and unregisters them from the mixer. */
	~SoundPlayer();

	SoundPlayer(const SoundPlayer&) = delete;
	SoundPlayer& operator=(const SoundPlayer&) = delete;

	/**
	 * @brief Starts one voice for @p buffer.
	 * @param buffer Buffer to play; an invalid handle is ignored.
	 * @param gain Linear gain of this voice, before the master volume.
	 * @return True when a voice was started.
	 */
	bool play(const SoundBufferHandle& buffer, float gain = 1.0f);
	/**
	 * @brief Starts one voice and takes ownership of a freshly built buffer.
	 * @param buffer Buffer to play; it moves into the voice.
	 * @param gain Linear gain of this voice, before the master volume.
	 * @return True when a voice was started.
	 *
	 * The moving overload keeps a caller from paying for a reference-count round trip
	 * when the buffer is created for this one playback anyway.
	 */
	bool play(SoundBufferHandle&& buffer, float gain = 1.0f);

	/** @brief Master gain applied on top of every voice gain. */
	float masterVolume() const noexcept { return master; }
	/**
	 * @brief Replaces the master gain and rewrites the mixer's child gains.
	 * @param newVolume Linear gain; negative values are clamped to 0.
	 */
	void setMasterVolume(float newVolume);
	/** @brief True when the master volume is forced to silence. */
	bool muted() const noexcept { return isMuted; }
	/**
	 * @brief Mutes or unmutes without losing the master volume.
	 * @param newMuted True to silence every voice.
	 */
	void setMuted(bool newMuted);

	/**
	 * @brief Retires the voices that reached the end of their buffer.
	 *
	 * Call once per frame, under the device lock. A voice that is still playing is
	 * left alone; a stopped one is erased from both the mixer and this player.
	 */
	void update();
	/** @brief Stops and unregisters every voice immediately. */
	void stopAll();
	/** @brief Number of voices this player currently owns. */
	std::size_t voiceCount() const noexcept { return voices.size(); }

private:
	/** @brief One playing buffer and the gain it was started with. */
	struct Voice {
		/** @brief The source the mixer pulls. */
		uSoundSource source;
		/** @brief Gain requested by the caller, before the master volume. */
		float gain = 1.0f;
	};

	/** @brief Gain the mixer should use for one voice right now. */
	float effectiveGain(const Voice& voice) const noexcept;
	/** @brief Writes the effective gain of one voice into the mixer. */
	void applyGain(Voice& voice);

	/** @brief Mixer whose child map holds the voices. */
	Mixer* mixer = nullptr;
	/** @brief Voices owned by this player, in start order. */
	std::pmr::vector<Voice> voices;
	/** @brief Master gain multiplied into every voice. */
	float master = 1.0f;
	/** @brief True when the master volume is not applied. */
	bool isMuted = false;
};

} // namespace Hrk

#endif // HRKSOUNDPLAYER_HPP
