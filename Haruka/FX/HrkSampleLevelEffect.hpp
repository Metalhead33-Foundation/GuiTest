#ifndef HRKSAMPLELEVELEFFECT_HPP
#define HRKSAMPLELEVELEFFECT_HPP
/**
 * @file HrkSampleLevelEffect.hpp
 * @brief Adapts a per-sample functor into a block-oriented @ref Hrk::Effect.
 *
 * Salvaged from `MhLib/Media/AdvancedAudio/FX/SampleLevelEffect.hpp` on the
 * `NewBeginning` branch. The original was an abstract class whose chart was
 * `virtual float doSampleLevelEffect(float) const` - one *virtual call per
 * sample*, which defeats inlining and vectorization. Here the per-sample
 * operation is a template parameter instead, so the loop below is a plain
 * inlinable functor application: the only virtual dispatch left happens once
 * per chunk, when @ref Hrk::EffectSlot calls @ref process().
 *
 * @note Functors must be cheap to copy (they are stored by value) and must not
 * throw.
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/FX/HrkAudioEffect.hpp>
#include <cstdint>
#include <utility>

namespace Hrk {

/**
 * @brief @ref Effect that maps a stateless-or-stateful functor over every sample.
 *
 * @tparam Functor Callable with the signature `float operator()(float)`.
 */
template <typename Functor>
class SampleLevelEffect final : public Effect
{
private:
	Functor functor;
public:
	/** @brief Creates the effect from a per-sample functor. */
	explicit SampleLevelEffect(Functor functor)
		: functor(std::move(functor)) {}

	/** @brief Applies the functor to every sample of the block. */
	void process(const ModificationStream& stream) override
	{
		float* const dst = stream.dst;
		if (!dst) return;
		const SampleCount sampleCount = framesToSamples(stream.frameCount, stream.channels);
		if (!sampleCount.var) return;
		const float* const src = stream.src ? stream.src : dst;
		const uintptr_t count = uintptr_t(sampleCount.var);
		for (uintptr_t i = 0; i < count; ++i) {
			dst[i] = functor(src[i]);
		}
	}

	/** @brief Sample-level effects never need a specific channel layout. */
	InterleavingType getRequiredInterleaving() const override { return InterleavingType::DONT_CARE; }

	/** @brief Returns the stored functor. */
	const Functor& getFunctor() const { return functor; }
	/** @brief Returns the stored functor. */
	Functor& getFunctor() { return functor; }
	/** @brief Replaces the stored functor. */
	void setFunctor(Functor newFunctor) { functor = std::move(newFunctor); }
};

/**
 * @brief Deduces the functor type so call sites do not have to spell it out.
 * @tparam Functor Callable with the signature `float operator()(float)`.
 * @param functor Per-sample operation to wrap.
 * @return A @ref SampleLevelEffect owning a copy of @a functor.
 */
template <typename Functor>
inline SampleLevelEffect<Functor> makeSampleLevelEffect(Functor functor)
{
	return SampleLevelEffect<Functor>(std::move(functor));
}

}

#endif // HRKSAMPLELEVELEFFECT_HPP
