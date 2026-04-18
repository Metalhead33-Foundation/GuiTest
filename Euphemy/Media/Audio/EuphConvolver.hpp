#ifndef EUPHCONVOLVER_HPP
#define EUPHCONVOLVER_HPP

#include <Euphemy/Config/EuphLib.hpp>
#include <Euphemy/Media/Audio/EuphAudioIterator.hpp>
#include <Elvavena/Util/ElvAllocatorBasic.hpp>
#include <Euphemy/Media/Audio/FFTConvolver/AudioFFT.h>
#include <Euphemy/Media/Audio/FFTConvolver/FFTConvolver.h>
#include <Euphemy/Media/Audio/FFTConvolver/TwoStageFFTConvolver.h>

namespace Euph {
namespace Media {
namespace Audio {

/**
 * @brief Alias for the sample type used by FFTConvolver.
 */
typedef fftconvolver::Sample ConvolutionSample;

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(AudioFFT)
/**
 * @brief Wrapper around `audiofft::AudioFFT`.
 */
class MH_EUPH_API AudioFFT {
private:
	audiofft::AudioFFT handle;
	AudioFFT( const AudioFFT &cpy ) = delete;
	AudioFFT& operator=( const AudioFFT &cpy ) = delete;

public:
	~AudioFFT( );
	AudioFFT( );

	/** @brief Initializes FFT buffers for a real-valued signal size. */
	void init( SampleCount size );
	/** @brief Executes forward FFT (real-to-split-complex). */
	void fft( const ConvolutionSample *data, ConvolutionSample *re, ConvolutionSample *im );
	/** @brief Executes inverse FFT (split-complex-to-real). */
	void ifft( ConvolutionSample *data, const ConvolutionSample *re, const ConvolutionSample *im );
	/** @brief Returns required split-complex buffer length for an input size. */
	static SampleCount complexSize( SampleCount size );
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(FFTConvolver)
/**
 * @brief Wrapper around `fftconvolver::FFTConvolver`.
 */
class MH_EUPH_API FFTConvolver {
private:
	fftconvolver::FFTConvolver handle;
	FFTConvolver( const FFTConvolver &cpy ) = delete;
	FFTConvolver& operator=( const FFTConvolver &cpy ) = delete;

public:
	~FFTConvolver( );
	FFTConvolver( );

	/**
	 * @brief Initializes the convolver with block size and impulse response.
	 * @return `true` on success.
	 */
	bool init( SampleCount block_size, const ConvolutionSample *ir, SampleCount ir_length );
	/** @brief Processes `length` samples from `input` to `output`. */
	void process( const ConvolutionSample *input, ConvolutionSample *output, SampleCount length );
	/** @brief Resets and clears the current impulse response. */
	void reset( );
};

DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(TwoStageFFTConvolver)
/**
 * @brief Wrapper around `fftconvolver::TwoStageFFTConvolver`.
 */
class MH_EUPH_API TwoStageFFTConvolver {
private:
	fftconvolver::TwoStageFFTConvolver handle;
	TwoStageFFTConvolver( const TwoStageFFTConvolver &cpy ) = delete;
	TwoStageFFTConvolver& operator=( const TwoStageFFTConvolver &cpy ) = delete;

public:
	~TwoStageFFTConvolver( );
	TwoStageFFTConvolver( );

	/**
	 * @brief Initializes the two-stage convolver.
	 * @return `true` on success.
	 */
	bool init( SampleCount head_block_size, SampleCount tail_block_size,
			  const ConvolutionSample *ir, SampleCount ir_length );
	/** @brief Processes `length` samples from `input` to `output`. */
	void process( const ConvolutionSample *input, ConvolutionSample *output, SampleCount length );
	/** @brief Resets and clears the current impulse response. */
	void reset( );
};

}
}
}

#endif // EUPHCONVOLVER_HPP
