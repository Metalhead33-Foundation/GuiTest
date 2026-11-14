#include "HrkImpulseGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <numeric>
#include <vector>

namespace Hrk {
namespace FX {

namespace {

/** @brief Pi as a double, avoiding the non-portable `M_PI` macro. */
constexpr double pi = std::numbers::pi;

/** @brief Unnormalized sinc, with the removable singularity at zero handled. */
template <typename T>
double sinc(T a)
{
	if (a == T(0)) return 1.0;
	return std::sin(a) / a;
}

/** @brief Kernel shapes supported by the generator. */
enum class FilterType : uint8_t {
	Lowpass,
	Highpass,
	Bandpass,
	Bandreject
};

/**
 * @brief Chooses an odd kernel length for a normalized transition band.
 * @param normalizedTransition Transition width as a fraction of the Nyquist frequency.
 * @return Odd kernel length, or zero when the transition band is degenerate.
 */
int kernelLength(double normalizedTransition)
{
	if (!(normalizedTransition > 0.0)) return 0;
	int length = int(std::ceil(4.0 / normalizedTransition));
	if ((length % 2) == 0) ++length;
	return std::max(length, 3);
}

/** @brief Blackman window coefficient for tap @a n of @a length taps. */
double blackmanWindow(int n, int length)
{
	const double denominator = double(length - 1);
	const double x = double(n) / denominator;
	return 0.42 - 0.5 * std::cos(2.0 * pi * x) + 0.08 * std::cos(4.0 * pi * x);
}

/** @brief Normalizes a kernel so its largest absolute tap becomes 1.0. */
void normalizeKernel(std::vector<float>& kernel)
{
	if (kernel.empty()) return;
	const auto it = std::max_element(kernel.begin(), kernel.end(), [](float a, float b) {
		return std::abs(a) < std::abs(b);
	});
	const float peak = *it;
	if (peak == 0.0f) return;
	const float divisor = 1.0f / peak;
	for (float& value : kernel) value *= divisor;
}

/**
 * @brief Builds a windowed-sinc low-pass kernel and normalizes its DC gain.
 * @param normalizedCutoff Cutoff as a fraction of the Nyquist frequency, in `(0,1)`.
 * @param length Odd kernel length.
 */
std::vector<float> makeLowpassKernel(double normalizedCutoff, int length)
{
	std::vector<float> kernel(std::size_t(length), 0.0f);
	const double center = double(length - 1) / 2.0;
	for (int n = 0; n < length; ++n) {
		const double h = sinc(2.0 * normalizedCutoff * (double(n) - center));
		kernel[std::size_t(n)] = float(h * blackmanWindow(n, length));
	}
	const double sum = std::accumulate(kernel.begin(), kernel.end(), 0.0);
	if (sum != 0.0) {
		for (float& value : kernel) value = float(double(value) / sum);
	}
	return kernel;
}

/**
 * @brief Builds a high-pass kernel by spectral inversion of the low-pass kernel.
 * @param normalizedCutoff Cutoff as a fraction of the Nyquist frequency, in `(0,1)`.
 * @param length Odd kernel length.
 */
std::vector<float> makeHighpassKernel(double normalizedCutoff, int length)
{
	std::vector<float> kernel = makeLowpassKernel(normalizedCutoff, length);
	for (float& value : kernel) value = -value;
	kernel[std::size_t((length - 1) / 2)] += 1.0f;
	return kernel;
}

/** @brief Naive discrete convolution; offline use only. */
std::vector<float> convolve(const std::vector<float>& f, const std::vector<float>& g)
{
	if (f.empty() || g.empty()) return {};
	const std::size_t nf = f.size();
	const std::size_t ng = g.size();
	std::vector<float> out(nf + ng - 1, 0.0f);
	for (std::size_t i = 0; i < out.size(); ++i) {
		const std::size_t first = (i >= ng - 1) ? i - (ng - 1) : 0;
		const std::size_t last = std::min(i, nf - 1);
		float accumulator = 0.0f;
		for (std::size_t j = first; j <= last; ++j) {
			accumulator += f[j] * g[i - j];
		}
		out[i] = accumulator;
	}
	return out;
}

void fillSingleFilter(std::vector<float>& vec, FilterType type, double sampleRate, double cutoff, bool normalize)
{
	const double nyquist = sampleRate / 2.0;
	if (!(nyquist > 0.0)) {
		vec.clear();
		return;
	}
	const double fc = cutoff / nyquist;
	if (!(fc > 0.0) || fc >= 1.0) {
		vec.clear();
		return;
	}
	const int length = kernelLength(fc * 0.8);
	if (!length) {
		vec.clear();
		return;
	}
	vec = (type == FilterType::Highpass) ? makeHighpassKernel(fc, length) : makeLowpassKernel(fc, length);
	if (normalize) normalizeKernel(vec);
}

/** @brief Builds a band kernel from a low-pass and a high-pass kernel. */
void fillBandFilter(std::vector<float>& vec, FilterType type, double sampleRate, double lowCutoff, double highCutoff, bool normalize)
{
	const double nyquist = sampleRate / 2.0;
	if (!(nyquist > 0.0)) {
		vec.clear();
		return;
	}
	const double fL = lowCutoff / nyquist;
	const double fH = highCutoff / nyquist;
	if (!(fL > 0.0) || !(fH > fL) || fH >= 1.0) {
		vec.clear();
		return;
	}
	const int length = kernelLength(fH * 0.8);
	if (!length) {
		vec.clear();
		return;
	}
	const std::vector<float> lowpass = makeLowpassKernel(fH, length);
	const std::vector<float> highpass = makeHighpassKernel(fL, length);
	if (type == FilterType::Bandpass) {
		vec = convolve(lowpass, highpass);
	} else {
		vec.resize(std::size_t(length));
		for (std::size_t i = 0; i < vec.size(); ++i) vec[i] = lowpass[i] + highpass[i];
	}
	if (normalize) normalizeKernel(vec);
}

/** @brief Wraps a finished kernel in an impulse response, or returns null when empty. */
sImpulseResponse makeImpulseResponse(const std::vector<float>& kernel, SampleRate sampleRate)
{
	if (kernel.empty()) return nullptr;
	auto response = std::make_shared<ImpulseResponse>(FrameCount(kernel.size()), sampleRate);
	std::copy(kernel.begin(), kernel.end(), response->data());
	return response;
}

} // namespace

sImpulseResponse createLowpassFilter(SampleRate inputSampleRate, SampleRate cutoffFrequency, bool normalize)
{
	std::vector<float> kernel;
	fillSingleFilter(kernel, FilterType::Lowpass, double(inputSampleRate.var), double(cutoffFrequency.var), normalize);
	return makeImpulseResponse(kernel, inputSampleRate);
}

sImpulseResponse createHighpassFilter(SampleRate inputSampleRate, SampleRate cutoffFrequency, bool normalize)
{
	std::vector<float> kernel;
	fillSingleFilter(kernel, FilterType::Highpass, double(inputSampleRate.var), double(cutoffFrequency.var), normalize);
	return makeImpulseResponse(kernel, inputSampleRate);
}

sImpulseResponse createBandpassFilter(SampleRate inputSampleRate, SampleRate lowCutoff, SampleRate highCutoff, bool normalize)
{
	std::vector<float> kernel;
	fillBandFilter(kernel, FilterType::Bandpass, double(inputSampleRate.var), double(lowCutoff.var), double(highCutoff.var), normalize);
	return makeImpulseResponse(kernel, inputSampleRate);
}

sImpulseResponse createBandrejectFilter(SampleRate inputSampleRate, SampleRate lowCutoff, SampleRate highCutoff, bool normalize)
{
	std::vector<float> kernel;
	fillBandFilter(kernel, FilterType::Bandreject, double(inputSampleRate.var), double(lowCutoff.var), double(highCutoff.var), normalize);
	return makeImpulseResponse(kernel, inputSampleRate);
}

sImpulseResponse createDelay(SampleRate inputSampleRate, FrameCount sampleOffset, float delayVolume)
{
	if (!sampleOffset.var) return nullptr;
	auto response = std::make_shared<ImpulseResponse>(sampleOffset, inputSampleRate);
	response->data()[std::size_t(sampleOffset.var) - 1] = delayVolume;
	return response;
}

}
}
