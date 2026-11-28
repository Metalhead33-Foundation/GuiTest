#ifndef EUPHSOUNDCLIP_HPP
#define EUPHSOUNDCLIP_HPP
/**
 * @file EuphSoundClip.hpp
 * @brief A decoded, resampled, mixer-ready block of PCM audio.
 *
 * `Euph::Media::Audio::SoundFile` is a streaming decoder: it hands out frames as the
 * caller asks for them and knows nothing about the format the mixer wants. A short
 * sound - a click, a notification, a footstep - is the opposite problem: it is small,
 * it is played many times, and every voice must arrive at the mixer in exactly one
 * format. This class is that prepared block.
 *
 * @section euph_soundclip_conform Conforming to a mixer
 *
 * `SoundClip::conformTo()` performs the two conversions a one-shot needs, once, at load time:
 *
 * 1. **Channels.** A mono click against a stereo mixer is replicated, a stereo file
 *    against a mono mixer is averaged, and anything else is mapped channel-wise
 *    without a pan law (see `SoundClip::mapChannels()`). `Hrk::SoundSource` *throws* on a
 *    channel mismatch, and it throws from the audio callback, so a mismatch is not
 *    something a caller can recover from at playback time.
 * 2. **Sample rate.** libsamplerate converts the whole clip in one pass
 *    (`SoundClip::resample()`), which is the cheapest place to do it: the cost is paid once
 *    per clip instead of once per playback, and no per-voice resampler state is kept.
 *
 * The result is interleaved float frames, which is what
 * `Hrk::makeSoundBuffer()` hands to a `Hrk::SoundSource`.
 */
#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Media/Audio/EuphAudioIterator.hpp>
#include <Euphemy/Media/Audio/EuphResampler.hpp>
#include <Elvavena/Io/ElvIoDevice.hpp>
#include <vector>

namespace Euph {
namespace Media {
namespace Audio {

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(SoundClip)

/**
 * @brief Interleaved float PCM plus the format it is stored in.
 *
 * An empty clip is a valid object: @ref valid() is `false`, and a caller that wants
 * "play this if it exists, otherwise nothing" can keep one around instead of
 * branching on an optional.
 */
class MH_EUPH_API SoundClip
{
public:
	/** @brief Creates an empty clip at 0 frames, 0 channels and 0 Hz. */
	SoundClip();
	/**
	 * @brief Adopts already-decoded samples.
	 * @param samples Interleaved float frames.
	 * @param channels Channel count the samples are interleaved by.
	 * @param sampleRate Rate the samples were captured at.
	 */
	SoundClip(std::vector<float> samples, ChannelCount channels, SampleRate sampleRate);
	/** @brief Copies a clip. */
	SoundClip(const SoundClip&) = default;
	/** @brief Moves a clip. */
	SoundClip(SoundClip&&) = default;
	/** @brief Copies a clip. */
	SoundClip& operator=(const SoundClip&) = default;
	/** @brief Moves a clip. */
	SoundClip& operator=(SoundClip&&) = default;
	/** @brief Releases the samples. */
	~SoundClip() = default;

	/**
	 * @brief Decodes a whole sound file through libsndfile.
	 * @param device Open read device; the clip takes ownership of the stream.
	 * @return The decoded clip, at the file's own rate and channel count.
	 * @throws std::runtime_error When the stream cannot be parsed as audio.
	 * @throws std::length_error When the frame count does not fit the sample buffer.
	 */
	static SoundClip load(Elv::Io::uDevice device);

	/// @name Format
	///@{
	/** @brief True when the clip holds at least one frame. */
	bool valid() const noexcept { return channelCount.var != 0 && !samples.empty(); }
	/** @brief Number of frames stored. */
	FrameCount frameCount() const noexcept;
	/** @brief Number of floats stored. */
	SampleCount sampleCount() const noexcept;
	/** @brief Channel count the samples are interleaved by. */
	ChannelCount channels() const noexcept { return channelCount; }
	/** @brief Rate the samples are stored at. */
	SampleRate sampleRate() const noexcept { return rate; }
	/** @brief Length in seconds; `0` for an empty clip. */
	double duration() const noexcept;
	///@}

	/// @name Samples
	///@{
	/** @brief Read-only pointer to the interleaved samples; null when empty. */
	const float* data() const noexcept { return samples.empty() ? nullptr : samples.data(); }
	/** @brief Mutable pointer to the interleaved samples; null when empty. */
	float* data() noexcept { return samples.empty() ? nullptr : samples.data(); }
	/** @brief The interleaved samples. */
	const std::vector<float>& storage() const noexcept { return samples; }
	///@}

	/// @name Conversion
	///@{
	/**
	 * @brief Resamples the whole clip in one pass.
	 * @param target Rate the clip should end up at.
	 * @param type Converter quality; the default is the cheapest sinc mode.
	 * @return True when the clip was converted; false when it already matched or is
	 *         empty.
	 * @throws std::runtime_error When the converter fails or cannot be created.
	 */
	bool resample(SampleRate target, ResampleType type = ResampleType::SINC_FASTEST);
	/**
	 * @brief Converts the channel count.
	 * @param target Channel count the clip should end up with.
	 * @return True when the clip was converted; false when it already matched or is
	 *         empty.
	 *
	 * Mono becomes every target channel, several channels become their average when a
	 * single channel is requested, and any other combination maps target channel `t`
	 * onto source channel `t % sourceChannels`. That is a plain format conversion, not
	 * a downmix: no pan law, no centre attenuation, no phase handling.
	 */
	bool mapChannels(ChannelCount target);
	/**
	 * @brief Maps the channels and then resamples, in that order.
	 * @param targetChannels Channel count the mixer expects.
	 * @param targetRate Rate the mixer runs at.
	 * @return True when anything had to change.
	 */
	bool conformTo(ChannelCount targetChannels, SampleRate targetRate);
	///@}

private:
	/** @brief Interleaved float frames. */
	std::vector<float> samples;
	/** @brief Channel count of @ref samples. */
	ChannelCount channelCount { 0 };
	/** @brief Sample rate of @ref samples. */
	SampleRate rate { 0 };
};

} // namespace Audio
} // namespace Media
} // namespace Euph

#endif // EUPHSOUNDCLIP_HPP
