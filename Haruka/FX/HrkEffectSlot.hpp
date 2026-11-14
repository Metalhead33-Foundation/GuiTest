#ifndef HRKEFFECTSLOT_HPP
#define HRKEFFECTSLOT_HPP
/**
 * @file HrkEffectSlot.hpp
 * @brief Serial dry/wet effect chain node for Haruka.
 *
 * Salvaged from `MhLib/Media/AdvancedAudio/MhAudioEffectSlot.{hpp,cpp}` on the
 * `NewBeginning` branch and rewritten for the pull-based Haruka graph, where
 * the slot is a @ref Hrk::ProxyPlayable that owns a sequence of
 * @ref Hrk::Effect objects with individual dry/wet mixes.
 *
 * @section hrk_effectslot_design Design notes (what changed and why)
 *
 * - **Contiguous storage.** The original kept its chain in a `std::deque` to
 *   get stable references across insertions. Effect chains are short, edited
 *   rarely and traversed on every audio block, so a contiguous
 *   `std::pmr::vector<SlotEntry>` is a better trade: iteration stays linear in
 *   one cache-line stream, and the chain is still declarative
 *   (add/remove/find/setWet) rather than index-driven.
 * - **Cached layout requirement.** `requiresDeinterlacing()` used to rescan the
 *   whole chain, calling a virtual on every entry, each time it was queried.
 *   The count of planar effects is now recomputed only when the chain is
 *   mutated, so the audio path never pays for it.
 * - **Three fixed scratch buffers.** Interleaved source, channel-planar copy
 *   and effect output. They are sized once from the constructor arguments, so
 *   `outputTo()` performs no allocation. Mono streams skip the
 *   deinterlace/interlace passes entirely, because the two layouts coincide.
 * - **Cascade semantics.** The original blended every effect against the
 *   *original* dry buffer, which effectively made the chain parallel and
 *   discarded all but the last effect's contribution. Here effect *n* reads the
 *   output of effect *n-1*, and the dry/wet blend is applied per stage:
 *   `out = in * (1 - wet) + processed(in) * wet`. A fully wet stage is swapped
 *   in without a copy.
 * - **No infinite pull loop.** The original `do { ... } while(framesToGo ||
 *   !currentlyDoneFrames)` spun forever when the source stopped producing
 *   frames. The pull loop now terminates when the source returns zero frames,
 *   and returns a short block.
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkProxyPlayable.hpp>
#include <Haruka/FX/HrkAudioEffect.hpp>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <vector>

namespace Hrk {

/**
 * @brief Playable node that pulls its source through a serial dry/wet effect chain.
 *
 * @section hrk_effectslot_contract Contract
 *
 * - The output packet must match @ref getChannelCount() and
 *   @ref getSampleRate(); a @ref SAMPLE_RATE_DONT_CARE request is answered with
 *   the slot's own rate.
 * - Both interleaved and channel-planar output requests are supported.
 * - At most @ref getBufferSize() frames are produced per call, because the
 *   internal scratch buffers are fixed at construction time. A larger request
 *   yields a short block, which the pull-based graph already handles.
 * - Effects are *aliased*, not owned. Keep them alive for as long as they are in
 *   the chain, and configure them before they reach the audio thread.
 */
class MH_HARUKA_API EffectSlot : public ProxyPlayable
{
public:
	/** @brief One effect plus its dry/wet mix. */
	struct SlotEntry {
		/** @brief Effect to run. Null entries are skipped. */
		Effect* effect = nullptr;
		/** @brief Mix amount: `0.0` is fully dry, `1.0` is fully wet. */
		float wet = 1.0f;
	};

	/** @brief Index value returned when a lookup fails. */
	static constexpr std::size_t npos = std::size_t(-1);
private:
	ChannelCount channelCount;
	SampleRate sampleRate;
	FrameCount bufferSize;
	std::pmr::vector<float> interleavedBuffer;
	std::pmr::vector<float> planarBuffer;
	std::pmr::vector<float> scratchBuffer;
	std::pmr::vector<SlotEntry> slots;
	std::size_t planarSlotCount = 0;
	void refreshInterleavingRequirement();
	FrameCount pullSource(FrameCount maxFrames);
public:
	/**
	 * @brief Creates an empty effect chain.
	 * @param channelCount Channel count of the processed stream.
	 * @param sampleRate Sample rate of the processed stream.
	 * @param bufferSize Maximum frames processed per @ref outputTo() call.
	 * @param memRes Memory resource backing the scratch buffers and the chain.
	 */
	EffectSlot(ChannelCount channelCount, SampleRate sampleRate, FrameCount bufferSize,
			   std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

	/** @brief Pulls from the source, runs the chain, and writes the result. */
	FrameCount outputTo(Output& output) override;

	/**
	 * @brief Appends an effect to the end of the chain.
	 * @param effect Effect to run; must outlive its removal from the chain.
	 * @param wet Initial dry/wet mix, clamped to `[0,1]`.
	 */
	void addEffect(Effect* effect, float wet = 1.0f);

	/**
	 * @brief Removes the first occurrence of an effect from the chain.
	 * @param effect Effect to look for by identity.
	 * @return `true` when an entry was removed.
	 */
	bool removeEffect(const Effect* effect);

	/**
	 * @brief Removes every occurrence of an effect from the chain.
	 * @param effect Effect to look for by identity.
	 * @return Number of removed entries.
	 */
	std::size_t removeEffects(const Effect* effect);

	/** @brief Removes all effects from the chain. */
	void clearEffects();

	/** @brief Returns the number of chain entries. */
	std::size_t getEffectCount() const { return slots.size(); }
	/** @brief Returns true when the chain has no entries. */
	bool empty() const { return slots.empty(); }

	/** @brief Returns chain entry @a index without bounds checking. */
	SlotEntry& getEffect(std::size_t index) { return slots[index]; }
	/** @brief Returns chain entry @a index without bounds checking. */
	const SlotEntry& getEffect(std::size_t index) const { return slots[index]; }

	/** @brief Returns the whole chain for iteration or bulk editing. */
	std::span<SlotEntry> getEffects() { return slots; }
	/** @brief Returns the whole chain for iteration. */
	std::span<const SlotEntry> getEffects() const { return slots; }

	/**
	 * @brief Changes the dry/wet mix of one chain entry.
	 * @param index Chain index.
	 * @param wet New mix amount, clamped to `[0,1]`.
	 * @return `true` when @a index exists.
	 */
	bool setWet(std::size_t index, float wet);

	/**
	 * @brief Finds the first chain entry holding an effect.
	 * @param effect Effect to look for by identity.
	 * @return Chain index, or @ref npos.
	 */
	std::size_t find(const Effect* effect) const;

	/** @brief Returns true when at least one effect needs channel-planar input. */
	bool requiresDeinterlacing() const noexcept { return planarSlotCount > 0; }

	/** @brief Returns the channel count of the processed stream. */
	ChannelCount getChannelCount() const { return channelCount; }
	/** @brief Returns the sample rate of the processed stream. */
	SampleRate getSampleRate() const { return sampleRate; }
	/** @brief Returns the maximum number of frames processed per call. */
	FrameCount getBufferSize() const { return bufferSize; }
	/** @brief Changes the sample rate written into output packets. */
	void setSampleRate(SampleRate newSampleRate) { sampleRate = newSampleRate; }
};

}

#endif // HRKEFFECTSLOT_HPP
