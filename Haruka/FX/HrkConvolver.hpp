#ifndef HRKCONVOLVER_HPP
#define HRKCONVOLVER_HPP
/**
 * @file HrkConvolver.hpp
 * @brief Multi-channel FFT convolution effect for Haruka.
 *
 * Salvaged from `MhLib/Media/AdvancedAudio/FX/MhConvolver.{hpp,cpp}` on the
 * `NewBeginning` branch. The idea is unchanged: one FFT convolver per channel,
 * each with its own impulse response, so an effect slot can host reverb,
 * cabinet simulation, delays and any other linear filter built from
 * @ref Hrk::FX::createLowpassFilter and friends.
 *
 * Deviations from the original:
 * - The IR type is @ref Hrk::ImpulseResponse instead of
 *   `fftconvolver::IrBuffer` (see `HrkImpulseResponse.hpp`).
 * - Convolvers are heap-allocated because Euphemy's `FFTConvolver` wrapper is
 *   neither copyable nor movable; a `std::pmr::vector` of owning pointers keeps
 *   the channel loop contiguous without requiring move semantics.
 * - A missing impulse response now raises `std::runtime_error` instead of
 *   dereferencing a null IR buffer inside the audio callback. Configure all
 *   channels before the effect enters the graph.
 * - `setImpulseResponse` reports whether the underlying FFT setup succeeded,
 *   and `reset()` clears the tail of all channels (useful when seeking).
 *
 * The effect is channel-planar (see `HrkAudioEffect.hpp`): it requires
 * `InterleavingType::SEPARATE_CHANNELS`, and @ref Hrk::EffectSlot performs the
 * deinterlacing/interlacing around it.
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/FX/HrkAudioEffect.hpp>
#include <Haruka/FX/HrkImpulseResponse.hpp>
#include <Euphemy/Media/Audio/EuphConvolver.hpp>
#include <memory>
#include <memory_resource>
#include <vector>

namespace Hrk {
namespace FX {

/** @brief Per-channel FFT convolution effect driven by impulse responses. */
class MH_HARUKA_API Convolver : public Effect
{
public:
	/** @brief Default FFT block size in samples, matching the vendored FFTConvolver guidance. */
	static const SampleCount DEFAULT_BLOCK_SIZE;
private:
	ChannelCount channelCount;
	SampleCount blockSize;
	std::pmr::vector<std::unique_ptr<Euph::Media::Audio::FFTConvolver>> convolvers;
	std::pmr::vector<scImpulseResponse> impulseResponses;
public:
	/**
	 * @brief Creates a convolver with one FFT stage per channel.
	 * @param channels Channel count of the streams this effect will process.
	 * @param blockSize FFT block size in samples.
	 * @param memRes Memory resource backing the per-channel bookkeeping vectors.
	 */
	explicit Convolver(ChannelCount channels, SampleCount blockSize = DEFAULT_BLOCK_SIZE,
					   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

	/** @brief Convolves every channel plane of the block in place. */
	void process(const ModificationStream& stream) override;

	/** @brief Convolution always needs one contiguous plane per channel. */
	InterleavingType getRequiredInterleaving() const override { return InterleavingType::SEPARATE_CHANNELS; }

	/** @brief Returns the channel count this effect was built for. */
	ChannelCount getChannelCount() const { return channelCount; }
	/** @brief Returns the FFT block size in samples. */
	SampleCount getBlockSize() const { return blockSize; }

	/**
	 * @brief Returns the impulse response currently assigned to a channel.
	 * @param channel Zero-based channel index.
	 * @return The shared IR, or null when the channel is unconfigured.
	 */
	scImpulseResponse getImpulseResponse(ChannelIndex channel) const;

	/**
	 * @brief Assigns (or clears) the impulse response of one channel.
	 * @param channel Zero-based channel index.
	 * @param response Impulse response to use; null clears the channel.
	 * @return `true` when the FFT stage accepted the response.
	 */
	bool setImpulseResponse(ChannelIndex channel, scImpulseResponse response);

	/** @brief Assigns the same impulse response to every channel. */
	bool setImpulseResponse(scImpulseResponse response);

	/** @brief Returns true when every channel has a working impulse response. */
	bool isReady() const;

	/** @brief Clears the internal convolution history of every channel. */
	void reset();
};

}
}

#endif // HRKCONVOLVER_HPP
