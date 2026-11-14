#ifndef HRKCLAMPEFFECT_HPP
#define HRKCLAMPEFFECT_HPP
/**
 * @file HrkClampEffect.hpp
 * @brief Hard-limiting (normalizing clamp) effect for Haruka.
 *
 * Salvaged from `MhLib/Media/AdvancedAudio/FX/MhClampEffect.{hpp,cpp}` on the
 * `NewBeginning` branch. The math is the original one:
 *
 *     out = sign(in) * min(|in|, maximumLevel) / maximumLevel
 *
 * which is a *normalizing* clamp: signals below the threshold pass through
 * linearly, signals above it are hard-limited, and the output is scaled so that
 * `|out| <= 1`.
 *
 * Deviations from the original:
 * - The original default-constructed with `maximumLevel = 0.0f`, which divided
 *   by zero. A non-positive level now means "no limiting" (identity) instead.
 * - `Util::sign` from the old tree is inlined as `std::copysign`, which also
 *   behaves correctly for negative zero.
 * - The per-sample virtual is gone; this is a functor wrapped by
 *   @ref Hrk::SampleLevelEffect (see `HrkSampleLevelEffect.hpp`).
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/FX/HrkSampleLevelEffect.hpp>
#include <algorithm>
#include <cmath>

namespace Hrk {
namespace FX {

/** @brief Functor form of the normalizing clamp. */
struct Clamp {
	/** @brief Level above which the signal is hard-limited; `<= 0` disables the effect. */
	float maximumLevel = 1.0f;

	/** @brief Applies the normalizing clamp to one sample. */
	float operator()(float input) const
	{
		if (maximumLevel <= 0.0f) return input;
		const float magnitude = std::min(std::abs(input), maximumLevel);
		return std::copysign(magnitude / maximumLevel, input);
	}
};

/** @brief @ref Hrk::Effect that hard-limits every sample at a fixed level. */
using ClampEffect = SampleLevelEffect<Clamp>;

}
}

#endif // HRKCLAMPEFFECT_HPP
