#include "HrkMixer.hpp"
#include <algorithm>
#include <cstring>
#include <Haruka/Core/HrkAudioError.hpp>
namespace Hrk {

ChannelCount Mixer::getChannelCount() const
{
	return channelCount;
}

InterleavingType Mixer::getInterleavingType() const
{
	return interleavingType;
}

SampleRate Mixer::getSampleRate() const
{
	return sampleRate;
}

FrameCount Mixer::getFrameCount() const
{
	return frameCount;
}

Mixer::MixerMap& Mixer::getChildren()
{
	return children;
}

const Mixer::MixerMap& Mixer::getChildren() const
{
	return children;
}

void Mixer::setInterleavingType(InterleavingType newInterleavingType)
{
	interleavingType = newInterleavingType;
}

void Mixer::setSampleRate(SampleRate newSampleRate)
{
	sampleRate = newSampleRate;
}

FrameCount Mixer::processIndividual(Playable& playable, float volume)
{
	FrameCount currentlyDoneFrames(0);
	FrameCount framesProcessedSoFar(0);
	FrameCount framesToGo = this->frameCount;
	float* buffA = buffer.data();
	float* buffB = &buffer[buffer.size() / 2];
	while(framesToGo.var) {
		Output out;
		out.dst = &buffB[Hrk::framesToSamples(framesProcessedSoFar, channelCount).var];
		out.channels = this->channelCount;
		out.samplerate = this->sampleRate;
		out.interleaving = interleavingType;
		out.frameCount = framesToGo;
		currentlyDoneFrames = playable.outputTo(out);
		if(!currentlyDoneFrames.var) break;
		framesProcessedSoFar += currentlyDoneFrames.var;
		framesToGo -= currentlyDoneFrames.var;
	}
	for(auto frameIndex : framesProcessedSoFar) {
		const uintptr_t sampleOffset = uintptr_t(frameIndex) * uintptr_t(channelCount.var);
		const float * const inFrameStart = &buffB[sampleOffset];
		float * const outFrameStart = &buffA[sampleOffset];
		for(auto channelIndex : channelCount) {
			outFrameStart[channelIndex] += inFrameStart[channelIndex] * volume;
		}
	}
	return framesProcessedSoFar;
}

FrameCount Mixer::process()
{
	std::memset(buffer.data(),0,sizeof(float)*buffer.size());
	FrameCount largest(0);
	for(auto it = std::begin(children); it != std::end(children); ++it) {
		largest = std::max(largest, processIndividual(*it->first, it->second));
	}
	return largest;
}

Mixer::Mixer(ChannelCount channelCount, InterleavingType interleavingType, SampleRate sampleRate, FrameCount frameCount, std::pmr::memory_resource* memRes)
	: channelCount(channelCount), interleavingType(interleavingType), sampleRate(sampleRate), frameCount(frameCount),
	buffer(Hrk::framesToSamples(frameCount, channelCount).var * 2u, memRes), children(memRes)
{}

void Mixer::reset(ChannelCount channelCount, InterleavingType interleavingType, SampleRate sampleRate, FrameCount frameCount, std::pmr::memory_resource* memRes)
{
	this->channelCount = channelCount;
	this->interleavingType = interleavingType;
	this->sampleRate = sampleRate;
	this->frameCount = frameCount;
	this->buffer = std::pmr::vector<float>(Hrk::framesToSamples(frameCount, channelCount).var * 2u, memRes);
	this->children = MixerMap(memRes);
}

FrameCount Mixer::outputTo(Output& output)
{
	if(!output.frameCount.var) return FrameCount(0);
	if(output.channels != this->channelCount) throw ChannelCountMismatchError(output.channels,this->channelCount);
	if(output.samplerate != this->sampleRate) throw SamplerateMismatchError(output.samplerate,this->sampleRate);
	if(channelCount.var > 1 && output.interleaving != interleavingType) throw InterleavingMismatchError(output.interleaving,interleavingType);
	FrameCount processed = process();
	memcpy(output.dst,buffer.data(),Hrk::samplesToBytes(Hrk::framesToSamples(processed,channelCount)));
	return processed;
}

}
