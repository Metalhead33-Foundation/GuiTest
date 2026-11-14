#ifndef HRKAUDIOEFFECT_HPP
#define HRKAUDIOEFFECT_HPP
/**
 * @file HrkAudioEffect.hpp
 * @brief Declares the block-oriented audio effect contract used by Haruka.
 *
 * Haruka processes audio in chunks. An `Effect` therefore receives one
 * @ref Hrk::ModificationStream describing a whole block, and is expected to
 * transform that block with a single virtual dispatch. Per-sample virtual
 * calls are deliberately *not* part of this contract: they cost far more than
 * the DSP they perform and they prevent the compiler from vectorizing the
 * inner loops. Effects that operate on individual samples should derive from
 * @ref Hrk::SampleLevelEffect, which is templated on an inlinable functor.
 *
 * Salvaged from `MhLib/Media/AdvancedAudio/MhAudioEffect.hpp` on the
 * `NewBeginning` branch. The original interface used
 * `doEffect(const ModificationStream&)` plus a non-const
 * `requiresDeinterlacing()` query; the contract is preserved but the naming
 * and const-correctness follow Haruka conventions, and the interleaving query
 * now reports *which* layout the effect wants instead of a bare boolean.
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>

namespace Hrk {

/**
 * @brief Block-oriented audio processing node used by @ref EffectSlot.
 *
 * @section hrk_effect_contract Contract for implementations
 *
 * - `process()` is called once per chunk, never once per sample.
 * - @ref ModificationStream::src may be null, meaning "process in place";
 *   implementations must then read from and write to
 *   @ref ModificationStream::dst.
 * - `src` and `dst` must be treated as fully aliasing-capable: an
 *   implementation must not assume that they do not overlap unless it
 *   documented that guarantee itself.
 * - The stream layout (channel count, frame count, sample rate, interleaving)
 *   is guaranteed to match what @ref getRequiredInterleaving() asked for.
 *   @ref EffectSlot performs any deinterlacing/interlacing on the effect's
 *   behalf, so an implementation never has to handle both layouts.
 * - Effects must not throw for well-formed input, and must not allocate per
 *   chunk on the audio thread.
 */
class MH_HARUKA_API Effect {
public:
	/** @brief Releases an effect through the base interface. */
	virtual ~Effect() = default;

	/**
	 * @brief Transforms one block of audio.
	 * @param stream Source/destination pointers plus the block's audio format.
	 */
	virtual void process(const ModificationStream& stream) = 0;

	/**
	 * @brief Reports the interleaving layout this effect wants its input in.
	 *
	 * `InterleavingType::DONT_CARE` (the default) means the effect works on any
	 * layout, so no deinterlacing is needed. `SEPARATE_CHANNELS` means the
	 * effect needs one contiguous plane per channel (convolution, per-channel
	 * dynamics, anything with per-channel state).
	 *
	 * @return The interleaving type required by @ref process().
	 */
	virtual InterleavingType getRequiredInterleaving() const { return InterleavingType::DONT_CARE; }

	/**
	 * @brief Convenience query over @ref getRequiredInterleaving().
	 * @return `true` when the effect needs channel-planar (deinterleaved) input.
	 */
	bool requiresDeinterlacing() const { return getRequiredInterleaving() == InterleavingType::SEPARATE_CHANNELS; }
};

}

#endif // HRKAUDIOEFFECT_HPP
