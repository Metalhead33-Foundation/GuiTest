#include "HrkMatrixPanner.hpp"
#include <Haruka/Core/HrkAudioError.hpp>
#include <algorithm>
namespace Hrk {

ChannelCount MatrixPanner::getInChannels() const
{
	return inChannels;
}

ChannelCount MatrixPanner::getOutChannels() const
{
	return outChannels;
}

FrameCount MatrixPanner::getFrameCount() const
{
	return frameCount;
}

InterleavingType MatrixPanner::getInterleavingType() const
{
	return interleavingType;
}

void MatrixPanner::setInterleavingType(InterleavingType newInterleavingType)
{
	interleavingType = newInterleavingType;
}

std::span<float> MatrixPanner::getMatrix()
{
	return std::span<float>(buffer.data(), getBufferOffsetForInput().var);
}

std::span<const float> MatrixPanner::getMatrix() const
{
	return std::span<const float>(buffer.data(), getBufferOffsetForInput().var);
}

SampleCount MatrixPanner::getBufferOffsetForInput() const
{
	return SampleCount(static_cast<SampleCount::dataType>(inChannels.var) * static_cast<SampleCount::dataType>(outChannels.var));
}

SampleCount MatrixPanner::getBufferOffsetFor(ChannelCount x, ChannelCount y) const
{
	return SampleCount((static_cast<SampleCount::dataType>(inChannels.var) * static_cast<SampleCount::dataType>(y.var)) + static_cast<SampleCount::dataType>(x.var));
}

void MatrixPanner::setToIdentity()
{
	for(auto y : outChannels) {
		for(auto x : inChannels) {
			SampleCount offset = getBufferOffsetFor(ChannelCount(x), ChannelCount(y));
			if(x == y) buffer[offset.var] = 1.0f;
			else buffer[offset.var] = 0.0f;
		}
	}
}

FrameCount MatrixPanner::outputTo(Output& output)
{
	// The size of this->buffer:
	// (inChannels*outChannels)+(inChannels*frameCount)
	// Why? Because start contains the matrix itself.
	// Starting offset of the scratch memory is at (inChannels*outChannels).

	if(!playable) return FrameCount(0);
	if(!output.frameCount.var) return FrameCount(0);
	if(output.channels != this->outChannels) throw ChannelCountMismatchError(output.channels,this->outChannels);
	if(outChannels.var > 1 && output.interleaving != interleavingType) throw InterleavingMismatchError(output.interleaving,interleavingType);
	FrameCount processed(0);
	FrameCount currentlyDoneFrames(0);
	FrameCount framesToGo(std::min(frameCount.var, output.frameCount.var));
	float* scratchBufferStart = &buffer[getBufferOffsetForInput().var];
	Output tmpOut;
	tmpOut.channels = inChannels;
	tmpOut.dst = scratchBufferStart;
	tmpOut.frameCount = framesToGo;
	tmpOut.samplerate = output.samplerate;
	tmpOut.interleaving = interleavingType;
	const SampleRate requestedSampleRate = output.samplerate;
	while(framesToGo.var) {
		currentlyDoneFrames = playable->outputTo(tmpOut);
		if(!currentlyDoneFrames.var) break;
		if(requestedSampleRate != SAMPLE_RATE_DONT_CARE && tmpOut.samplerate != requestedSampleRate) {
			throw SamplerateMismatchError(tmpOut.samplerate, requestedSampleRate);
		}
		if(output.samplerate == SAMPLE_RATE_DONT_CARE && tmpOut.samplerate != SAMPLE_RATE_DONT_CARE) {
			output.samplerate = tmpOut.samplerate;
		}
		float* outPtr = &output.dst[Hrk::framesToSamples(processed, output.channels).var];
		// This is the part we actually process the audio...
			switch (interleavingType) {
				case InterleavingType::DONT_CARE: //! Fall-through
				case InterleavingType::INTERLEAVED: {
					for(auto frameIndex : currentlyDoneFrames) {
						const float* inFrameStart = &scratchBufferStart[static_cast<uintptr_t>(frameIndex) * inChannels.var];
						float* outFrameStart = &outPtr[static_cast<uintptr_t>(frameIndex) * outChannels.var];
						for(auto outChannel : outChannels) {
							float sum = 0.0f;
							for(auto inChannel : inChannels) {
								sum += inFrameStart[inChannel] * buffer[getBufferOffsetFor(ChannelCount(inChannel), ChannelCount(outChannel)).var];
							}
							outFrameStart[outChannel] = sum;
						}
					}
					break;
				}
				case InterleavingType::SEPARATE_CHANNELS: {
					for(auto outChannel : outChannels) {
						float* outChannelStart = &output.dst[(static_cast<uintptr_t>(outChannel) * output.frameCount.var) + processed.var];
						for(auto frameIndex : currentlyDoneFrames) {
							float sum = 0.0f;
							for(auto inChannel : inChannels) {
								const float* inChannelStart = &scratchBufferStart[static_cast<uintptr_t>(inChannel) * framesToGo.var];
								sum += inChannelStart[frameIndex] * buffer[getBufferOffsetFor(ChannelCount(inChannel), ChannelCount(outChannel)).var];
							}
							outChannelStart[frameIndex] = sum;
						}
					}
					break;
				}
			}
		processed += currentlyDoneFrames.var;
		framesToGo -= currentlyDoneFrames.var;
		tmpOut.frameCount = framesToGo;
	}
	return processed;
}

MatrixPanner::MatrixPanner(ChannelCount inChannels, ChannelCount outChannels, InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes)
	: inChannels(inChannels), outChannels(outChannels), interleavingType(interleavingType), frameCount(frameCount),
	buffer(getBufferOffsetForInput().var + Hrk::framesToSamples(frameCount, inChannels).var, memRes)
{
	setToIdentity();
}

void MatrixPanner::reset(ChannelCount inChannels, ChannelCount outChannels, InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes)
{
	this->inChannels = inChannels;
	this->outChannels = outChannels;
	this->interleavingType = interleavingType;
	this->frameCount = frameCount;
	this->buffer = std::pmr::vector<float>(getBufferOffsetForInput().var + Hrk::framesToSamples(frameCount, inChannels).var, memRes);
	setToIdentity();
}

}
