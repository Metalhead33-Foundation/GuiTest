#include <MhLib/Media/AdvancedAudio/FX/MhImpulseGenerator.hpp>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <cstring>
#include <vector>
namespace MH33 {
namespace Audio {
namespace  FX {

template<typename T> double sinc(T a)
{
	if(a == 0.00) return 1.00;
	else return sin(a)/a;
}

template<typename T>
std::vector<T>
conv(std::vector<T> const &f, std::vector<T> const &g) {
	int const nf = f.size();
	int const ng = g.size();
	int const n  = nf + ng - 1;
	std::vector<T> out(n, T());
	for(auto i(0); i < n; ++i) {
		int const jmn = (i >= ng - 1)? i - (ng - 1) : 0;
		int const jmx = (i <  nf - 1)? i            : nf - 1;
		for(auto j(jmn); j <= jmx; ++j) {
			out[i] += (f[j] * g[i - j]);
		}
	}
	return out;
}

enum FilterType : uint8_t
{
	lowpass,
	highpass,
	bandpass,
	bandreject
};

static void fillSingleFilter(std::vector<float>& vec, FilterType type, int inputSamplerate,int CutoffFrequency, bool normalize=false)
{
	const double fc = double(CutoffFrequency)/(double(inputSamplerate)/2.0);
	const double b = fc*0.8;
	int N = int(std::ceil(4.0/b));

	if ((N % 2) == 0) N++;

	std::vector<int> n(N,0);

	std::iota(n.begin(), n.end(), 0);

	vec.resize(N);
	memset(vec.data(),0,sizeof(float)*vec.size());

	std::transform(n.begin(), n.end(), vec.begin(), [fc, b, N](int n){
		const double h = sinc(2.0 * fc * (n - (N - 1) / 2.0));

		const double w = 0.42 - 0.5 * std::cos(2 * M_PI * n / (N - 1)) +
						 0.08 * std::cos(4 * M_PI * n / (N - 1));

		return float(h * w);

	});

	float h_sum = std::accumulate(vec.begin(), vec.end(), 0.0f);
	for(auto it = vec.begin(); it != vec.end(); ++it)
	{
		switch(type)
		{
		case lowpass:
			*it /= h_sum;
			break;
		case highpass:
			*it /= h_sum;
			*it *= -1.0f;
			break;
		default:
			*it /= h_sum;
			break;
		}
	}
	if(type == highpass)
	{
		vec[(N - 1) / 2] += 1.0f;
	}
	if(normalize) {
		const float divisor=1 / *std::max_element(vec.begin(),vec.end());
		for(auto it = vec.begin(); it != vec.end(); ++it)
		{
			*it *= divisor;
		}
	}
}
static void fillBandFilter(std::vector<float>& vec, FilterType type, int inputSamplerate,int lowCutoff,int highCutoff, bool normalize=false)
{
	const double fL = double(lowCutoff)/(double(inputSamplerate)/2.0);
	const double fH = double(highCutoff)/(double(inputSamplerate)/2.0);
	const double b = fH*0.8; // Transition band
	int N = int(std::ceil(4.0/b));

	if ((N % 2) == 0) N++;

	std::vector<int> n(N,0);

	std::iota(n.begin(), n.end(), 0);

	vec.resize(N);
	memset(vec.data(),0,sizeof(float)*vec.size());
	std::vector<float> lowpass(N);
	std::vector<float> highpass(N);
	memset(lowpass.data(),0,sizeof(float)*lowpass.size());
	memset(highpass.data(),0,sizeof(float)*highpass.size());

	/* Calculate passes */
	std::transform(n.begin(), n.end(), lowpass.begin(), [fH, b, N](int n){
		const double h = sinc(2.0 * fH * (n - (N - 1) / 2.0));

		const double w = 0.42 - 0.5 * std::cos(2 * M_PI * n / (N - 1)) +
						 0.08 * std::cos(4 * M_PI * n / (N - 1));

		return float(h * w);

	});
	std::transform(n.begin(), n.end(), highpass.begin(), [fL, b, N](int n){
		const double h = sinc(2.0 * fL * (n - (N - 1) / 2.0));

		const double w = 0.42 - 0.5 * std::cos(2 * M_PI * n / (N - 1)) +
						 0.08 * std::cos(4 * M_PI * n / (N - 1));

		return float(h * w);
	});

	float h_sum = std::accumulate(lowpass.begin(), lowpass.end(), 0.0f);
	for(auto it = lowpass.begin(); it != lowpass.end(); ++it)
	{
		*it /= h_sum;
	}
	h_sum = std::accumulate(highpass.begin(), highpass.end(), 0.0f);
	for(auto it = highpass.begin(); it != highpass.end(); ++it)
	{
		*it /= h_sum;
		*it *= -1.0f;
	}
	highpass[(N - 1) / 2] += 1.0f;

	switch(type)
	{
	case bandpass:
		vec = conv(lowpass,highpass);
		break;
	case bandreject:
		for(size_t i = 0; i < vec.size();++i) vec[i] = lowpass[i] + highpass[i];
		break;
	default:
		break;
	}
	if(normalize) {
		const float divisor=1 / *std::max_element(vec.begin(),vec.end());
		for(auto it = vec.begin(); it != vec.end(); ++it)
		{
			*it *= divisor;
		}
	}
}

void createDelayIR(std::vector<float>& IR, size_t sampleOffset, float delayVolume=1.0f)
{
	IR.resize(sampleOffset);
	memset(IR.data(),0,IR.size()*sizeof(float));
	//IR[0] = 1.0f;
	IR[IR.size()-1] = delayVolume;
	/*for(int i = 0; i < 4; ++i)
	{
		IR.push_back(IR[IR.size()-1]*0.5f);
	}*/
}

fftconvolver::sIrBuffer createLowpassFilter(FrameRate inputSamplerate, FrameRate CutoffFrequency, bool normalize)
{
	std::vector<float> buff;
	fillSingleFilter(buff,FilterType::lowpass,inputSamplerate.var,CutoffFrequency.var,normalize);
	return std::make_shared<fftconvolver::IrBuffer>(buff.data(),buff.size(),inputSamplerate);
}

fftconvolver::sIrBuffer createHighpassFilter(FrameRate inputSamplerate, FrameRate CutoffFrequency, bool normalize)
{
	std::vector<float> buff;
	fillSingleFilter(buff,FilterType::highpass,inputSamplerate.var,CutoffFrequency.var,normalize);
	return std::make_shared<fftconvolver::IrBuffer>(buff.data(),buff.size(),inputSamplerate);
}

fftconvolver::sIrBuffer createBandpassFilter(FrameRate inputSamplerate, FrameRate lowCutoff, FrameRate highCutoff, bool normalize)
{
	std::vector<float> buff;
	fillBandFilter(buff,FilterType::bandpass,inputSamplerate.var,lowCutoff.var,highCutoff.var,normalize);
	return std::make_shared<fftconvolver::IrBuffer>(buff.data(),buff.size(),inputSamplerate);
}

fftconvolver::sIrBuffer createBandrejectFilter(FrameRate inputSamplerate, FrameRate lowCutoff, FrameRate highCutoff, bool normalize)
{
	std::vector<float> buff;
	fillBandFilter(buff,FilterType::bandreject,inputSamplerate.var,lowCutoff.var,highCutoff.var,normalize);
	return std::make_shared<fftconvolver::IrBuffer>(buff.data(),buff.size(),inputSamplerate);
}

fftconvolver::sIrBuffer createDelay(FrameRate inputSamplerate, SampleIndex sampleOffset, float delayVolume)
{
	std::vector<float> buff;
	createDelayIR(buff,sampleOffset.var,delayVolume);
	return std::make_shared<fftconvolver::IrBuffer>(buff.data(),buff.size(),inputSamplerate);
}

}
}
}
