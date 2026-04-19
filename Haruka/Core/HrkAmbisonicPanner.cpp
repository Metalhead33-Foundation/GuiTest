#include "HrkAmbisonicPanner.hpp"
#include <Haruka/Core/HrkAudioError.hpp>
#include <algorithm>
#include <cmath>
namespace Hrk {

float AmbisonicPanner::getTheta() const
{
	return theta;
}

float AmbisonicPanner::getPhi() const
{
	return phi;
}

float AmbisonicPanner::getLF() const
{
	return LF;
}

float AmbisonicPanner::getLB() const
{
	return LB;
}

float AmbisonicPanner::getRF() const
{
	return RF;
}

float AmbisonicPanner::getRB() const
{
	return RB;
}

float AmbisonicPanner::getDistInv() const
{
	return distInv;
}

FrameCount AmbisonicPanner::outputTo(Output& output)
{
	if(!playable) return FrameCount(0);
	if(!output.frameCount.var) return FrameCount(0);
	if(output.channels != ChannelCount(4)) throw ChannelCountMismatchError(output.channels, ChannelCount(4));
	if(output.interleaving != interleavingType) throw InterleavingMismatchError(output.interleaving,interleavingType);
	FrameCount processed(0);
	FrameCount currentlyDoneFrames(0);
	FrameCount framesToGo(std::min(static_cast<FrameCount::dataType>(buffer.size()), output.frameCount.var));
	Output tmpOut;
	tmpOut.channels = ChannelCount(1);
	tmpOut.dst = buffer.data();
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
					const float inFrame = tmpOut.dst[frameIndex];
					float* outFrameStart = &outPtr[frameIndex * 4u];
					outFrameStart[0] = inFrame * LF;
					outFrameStart[1] = inFrame * RF;
					outFrameStart[2] = inFrame * LB;
					outFrameStart[3] = inFrame * RB;
				}
				break;
			}
			case InterleavingType::SEPARATE_CHANNELS: {
				const uintptr_t processedOffset = processed.var;
				const uintptr_t outputFrames = output.frameCount.var;
				float* outLF = &output.dst[(0u * outputFrames) + processedOffset];
				float* outRF = &output.dst[(1u * outputFrames) + processedOffset];
				float* outLB = &output.dst[(2u * outputFrames) + processedOffset];
				float* outRB = &output.dst[(3u * outputFrames) + processedOffset];
				for(auto frameIndex : currentlyDoneFrames) {
					const float inFrame = tmpOut.dst[frameIndex];
					outLF[frameIndex] = inFrame * LF;
					outRF[frameIndex] = inFrame * RF;
					outLB[frameIndex] = inFrame * LB;
					outRB[frameIndex] = inFrame * RB;
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

InterleavingType AmbisonicPanner::getInterleavingType() const
{
	return interleavingType;
}

void AmbisonicPanner::setInterleavingType(InterleavingType newInterleavingType)
{
	interleavingType = newInterleavingType;
}

AmbisonicPanner::AmbisonicPanner(InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes)
	: interleavingType(interleavingType), buffer(frameCount, memRes),
	theta(0.0f), phi(0.0f), distInv(0.0f), LF(0.0f), LB(0.0f), RF(0.0f), RB(0.0f)
{

}

void AmbisonicPanner::resetBuffer(FrameCount frameCount, std::pmr::memory_resource* memRes)
{
	buffer = std::pmr::vector<float>(frameCount, memRes);
}

void AmbisonicPanner::updatePosition(float theta, float phi, float distInv)
{
	this->theta = theta;
	this->phi = phi;
	this->distInv = distInv;

	constexpr float SQRT8 = 2.8284271247461900976f;

	const float cosTheta = std::cos(theta);
	const float sinTheta = std::sin(theta);
	const float cosPhi   = std::cos(phi);

	const float x = cosTheta * cosPhi;
	const float y = sinTheta * cosPhi;

	const float base = distInv * SQRT8;

	LF = base * (1.0f + x + y);
	LB = base * (1.0f - x + y);
	RB = base * (1.0f - x - y);
	RF = base * (1.0f + x - y);
}

}
