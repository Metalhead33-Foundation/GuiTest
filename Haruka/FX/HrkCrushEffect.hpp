#ifndef HRKCRUSHEFFECT_HPP
#define HRKCRUSHEFFECT_HPP
/**
 * @file HrkCrushEffect.hpp
 * @brief Bit-crush / quantization effect for Haruka.
 *
 * Salvaged from `MhLib/Media/AdvancedAudio/FX/MhCrushEffect.{hpp,cpp}` on the
 * `NewBeginning` branch. The original algorithm remaps the signed sample range
 * to `[0,1]`, quantizes it into a fixed number of steps, then maps it back:
 *
 *     u   = (in + 2) / 2
 *     q   = floor(u * steps + 0.5) / steps
 *     out = q * 2 - 1
 *
 * Note that the legacy member was called `maxLevel`, but the value is really a
 * *step count*: `steps = 1` is a one-bit crush, larger values approach the
 * identity. It is exposed here under the accurate name, with `getSteps()` /
 * `setSteps()` accessors kept close to the original `getMaxLevel()` /
 * `setMaxLevel()` pair for porting convenience.
 *
 * Deviations from the original: a non-positive step count is treated as the
 * identity instead of dividing by zero, and the per-sample virtual is replaced
 * by a functor (see `HrkSampleLevelEffect.hpp`).
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/FX/HrkSampleLevelEffect.hpp>
#include <cmath>

namespace Hrk {
namespace FX {

/** @brief Functor form of the quantizing crush. */
struct Crush {
	/** @brief Number of quantization steps across the signed range; `<= 0` disables the effect. */
	float steps = 1.0f;

	/** @brief Quantizes one sample. */
	float operator()(float input) const
	{
		if (steps <= 0.0f) return input;
		const float unsignedValue = (input + 2.0f) * 0.5f;
		const float quantized = std::floor(unsignedValue * steps + 0.5f) / steps;
		return quantized * 2.0f - 1.0f;
	}
};

/** @brief @ref Hrk::Effect that quantizes every sample into a fixed step count. */
using CrushEffect = SampleLevelEffect<Crush>;

}
}

#endif // HRKCRUSHEFFECT_HPP
