#include "EuphConvolver.hpp"

#include <cstddef>

namespace Euph {
namespace Media {
namespace Audio {

AudioFFT::~AudioFFT() = default;

AudioFFT::AudioFFT() = default;

void AudioFFT::init( SampleCount size )
{
	handle.init( size_t( size.var ) );
}

void AudioFFT::fft( const ConvolutionSample *data, ConvolutionSample *re, ConvolutionSample *im )
{
	handle.fft( data, re, im );
}

void AudioFFT::ifft( ConvolutionSample *data, const ConvolutionSample *re, const ConvolutionSample *im )
{
	handle.ifft( data, re, im );
}

SampleCount AudioFFT::complexSize( SampleCount size )
{
	return SampleCount( uintptr_t( audiofft::AudioFFT::ComplexSize( size_t( size.var ) ) ) );
}

FFTConvolver::~FFTConvolver() = default;

FFTConvolver::FFTConvolver() = default;

bool FFTConvolver::init( SampleCount block_size, const ConvolutionSample *ir, SampleCount ir_length )
{
	return handle.init( size_t( block_size.var ), ir, size_t( ir_length.var ) );
}

void FFTConvolver::process( const ConvolutionSample *input, ConvolutionSample *output, SampleCount length )
{
	handle.process( input, output, size_t( length.var ) );
}

void FFTConvolver::reset()
{
	handle.reset();
}

TwoStageFFTConvolver::~TwoStageFFTConvolver() = default;

TwoStageFFTConvolver::TwoStageFFTConvolver() = default;

bool TwoStageFFTConvolver::init( SampleCount head_block_size, SampleCount tail_block_size,
						 const ConvolutionSample *ir, SampleCount ir_length )
{
	return handle.init( size_t( head_block_size.var ), size_t( tail_block_size.var ), ir,
					  size_t( ir_length.var ) );
}

void TwoStageFFTConvolver::process( const ConvolutionSample *input, ConvolutionSample *output,
							 SampleCount length )
{
	handle.process( input, output, size_t( length.var ) );
}

void TwoStageFFTConvolver::reset()
{
	handle.reset();
}

}
}
}
