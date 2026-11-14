#ifndef HRKIMPULSEGENERATOR_HPP
#define HRKIMPULSEGENERATOR_HPP
/**
 * @file HrkImpulseGenerator.hpp
 * @brief Synthesizes impulse responses (filters, delays) for @ref Hrk::FX::Convolver.
 *
 * Salvaged from `MhLib/Media/AdvancedAudio/FX/MhImpulseGenerator.{hpp,cpp}` on
 * the `NewBeginning` branch. The DSP is the original windowed-sinc design:
 * a Blackman-windowed sinc low-pass, a spectral-inversion high-pass, band
 * variants built from those two, and a single-tap delay. All generators are
 * offline operations - they allocate and run `O(N)` math, so call them while
 * loading assets, not from the audio thread.
 *
 * Deviations from the original:
 * - The return type is @ref Hrk::ImpulseResponse (sample-planar floats with an
 *   explicit sample rate) instead of `fftconvolver::sIrBuffer`.
 * - `FrameRate` is spelled @ref Hrk::SampleRate, and `SampleIndex` for the
 *   delay is a plain frame count.
 * - `M_PI` is replaced by `std::numbers::pi` for portability.
 * - The band-pass/band-reject kernels no longer encode their length in an
 *   out-parameter vector that was previously resized by the trigonometric
 *   transform itself; the length is computed first and reused.
 */

#include <Haruka/HarukaLib.hpp>
#include <Haruka/FX/HrkImpulseResponse.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>

namespace Hrk {
namespace FX {

/**
 * @brief Builds a Blackman-windowed sinc low-pass impulse response.
 * @param inputSampleRate Sample rate the response is generated for.
 * @param cutoffFrequency Filter cutoff in Hz.
 * @param normalize When true, scales the kernel so its largest tap is 1.0.
 * @return A mono impulse response, or null when the arguments are degenerate.
 */
MH_HARUKA_API sImpulseResponse createLowpassFilter(SampleRate inputSampleRate, SampleRate cutoffFrequency, bool normalize = false);

/**
 * @brief Builds a high-pass impulse response by spectral inversion of the low-pass kernel.
 * @param inputSampleRate Sample rate the response is generated for.
 * @param cutoffFrequency Filter cutoff in Hz.
 * @param normalize When true, scales the kernel so its largest tap is 1.0.
 * @return A mono impulse response, or null when the arguments are degenerate.
 */
MH_HARUKA_API sImpulseResponse createHighpassFilter(SampleRate inputSampleRate, SampleRate cutoffFrequency, bool normalize = false);

/**
 * @brief Builds a band-pass impulse response by convolving a low-pass and a high-pass kernel.
 * @param inputSampleRate Sample rate the response is generated for.
 * @param lowCutoff Lower band edge in Hz.
 * @param highCutoff Upper band edge in Hz.
 * @param normalize When true, scales the kernel so its largest tap is 1.0.
 * @return A mono impulse response, or null when the arguments are degenerate.
 */
MH_HARUKA_API sImpulseResponse createBandpassFilter(SampleRate inputSampleRate, SampleRate lowCutoff, SampleRate highCutoff, bool normalize = false);

/**
 * @brief Builds a band-reject impulse response by summing the low-pass and high-pass kernels.
 * @param inputSampleRate Sample rate the response is generated for.
 * @param lowCutoff Lower rejected band edge in Hz.
 * @param highCutoff Upper rejected band edge in Hz.
 * @param normalize When true, scales the kernel so its largest tap is 1.0.
 * @return A mono impulse response, or null when the arguments are degenerate.
 */
MH_HARUKA_API sImpulseResponse createBandrejectFilter(SampleRate inputSampleRate, SampleRate lowCutoff, SampleRate highCutoff, bool normalize = false);

/**
 * @brief Builds a single-tap delay impulse response.
 *
 * Convolving a signal with this response produces the same signal delayed by
 * @a sampleOffset frames and scaled by @a delayVolume. This is the primitive
 * behind echo/delay effects: chain several of them, or feed the result through
 * @ref Hrk::FX::Convolver with a feedback path.
 *
 * @param inputSampleRate Sample rate the response is generated for.
 * @param sampleOffset Delay length in frames; must be at least 1.
 * @param delayVolume Gain applied to the delayed copy.
 * @return A mono impulse response, or null when @a sampleOffset is zero.
 */
MH_HARUKA_API sImpulseResponse createDelay(SampleRate inputSampleRate, FrameCount sampleOffset, float delayVolume = 1.0f);

}
}

#endif // HRKIMPULSEGENERATOR_HPP
