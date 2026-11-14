#include "HrkResampler.hpp"
#include <Haruka/Core/HrkAudioError.hpp>
#include <algorithm>
#include <cstring>

namespace Hrk {

Resampler::Resampler(Euph::Media::Audio::FrameCount bufferSize, Euph::Media::Audio::ChannelCount channels, float speed, ResampleType resampleType, std::pmr::memory_resource* memRes)
	: channels(channels), resampleType(resampleType), bufferSize(bufferSize), speed(speed), resampler(channels,resampleType),
	buffer(framesToSamples(bufferSize,channels).var, memRes)
{

}

Euph::Media::Audio::FrameCount Resampler::outputTo(Output& output)
{
	if(!playable) return FrameCount(0);
	if(!output.frameCount.var) return FrameCount(0);
	if(output.channels != this->channels) throw ChannelCountMismatchError(output.channels,this->channels);
	if(channels.var > 1 && output.interleaving != InterleavingType::INTERLEAVED) throw InterleavingMismatchError(output.interleaving,InterleavingType::INTERLEAVED);

	const SampleRate requestedSampleRate = output.samplerate;
	FrameCount processedOutputFrames(0);
	Output tmpOut;
	tmpOut.channels = channels;
	tmpOut.dst = buffer.data();
	tmpOut.samplerate = SAMPLE_RATE_DONT_CARE;
	tmpOut.interleaving = InterleavingType::INTERLEAVED;

	while(processedOutputFrames.var < output.frameCount.var) {
		const FrameCount remainingOutputFrames(output.frameCount.var - processedOutputFrames.var);

		// Keep producing input into our scratch buffer while no data is buffered.
		while(!bufferedInputFrames.var) {
			const FrameCount toRequest(std::min(bufferSize.var, remainingOutputFrames.var));
			if(!toRequest.var) break;
			tmpOut.frameCount = toRequest;
			tmpOut.samplerate = SAMPLE_RATE_DONT_CARE;
			const FrameCount fetched = playable->outputTo(tmpOut);
			if(!fetched.var) break;
			bufferedInputFrames = fetched;
			if(tmpOut.samplerate != SAMPLE_RATE_DONT_CARE) {
				inSampleRate = tmpOut.samplerate;
			}
		}
		if(!bufferedInputFrames.var) break;

		// If source sample-rate is unknown, bypass resampling and copy source frames directly.
		if(inSampleRate == SAMPLE_RATE_DONT_CARE) {
			const FrameCount passthroughFrames(std::min(bufferedInputFrames.var, remainingOutputFrames.var));
			const SampleCount passthroughSamples = framesToSamples(passthroughFrames, channels);
			float* outPtr = &output.dst[framesToSamples(processedOutputFrames, output.channels).var];
			std::memcpy(outPtr, buffer.data(), samplesToBytes(passthroughSamples));
			processedOutputFrames += passthroughFrames.var;
			bufferedInputFrames -= passthroughFrames.var;
			if(bufferedInputFrames.var) {
				const SampleCount unreadSamples = framesToSamples(bufferedInputFrames, channels);
				const SampleCount consumedSamples = framesToSamples(passthroughFrames, channels);
				std::memmove(buffer.data(), &buffer[consumedSamples.var], samplesToBytes(unreadSamples));
			}
			continue;
		}

		if(requestedSampleRate != SAMPLE_RATE_DONT_CARE && output.samplerate != requestedSampleRate) {
			throw SamplerateMismatchError(output.samplerate, requestedSampleRate);
		}

		if(output.samplerate == SAMPLE_RATE_DONT_CARE) {
			output.samplerate = inSampleRate;
		}

		const double ratio = double(output.samplerate.var) / double(inSampleRate.var) / double(speed);
		float* outPtr = &output.dst[framesToSamples(processedOutputFrames, output.channels).var];
		resampler.set(buffer.data(), outPtr, bufferedInputFrames, remainingOutputFrames, ratio);
		resampler.process();

		const FrameCount usedInputFrames = resampler.getInputFramesUsed();
		const FrameCount generatedOutputFrames = resampler.getOutputFramesGenerated();
		processedOutputFrames += generatedOutputFrames.var;
		bufferedInputFrames -= usedInputFrames.var;
		if(bufferedInputFrames.var) {
			const SampleCount unreadSamples = framesToSamples(bufferedInputFrames, channels);
			const SampleCount consumedSamples = framesToSamples(usedInputFrames, channels);
			std::memmove(buffer.data(), &buffer[consumedSamples.var], samplesToBytes(unreadSamples));
		}

		// Avoid an endless loop if converter produced nothing and consumed nothing.
		if(!generatedOutputFrames.var && !usedInputFrames.var) break;
	}
	return processedOutputFrames;
}

}
