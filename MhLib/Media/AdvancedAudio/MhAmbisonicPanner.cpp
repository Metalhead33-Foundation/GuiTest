#include <MhLib/Media/AdvancedAudio/MhAmbisonicPanner.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>
#include <cmath>

namespace MH33 {
namespace Audio {

const float twoRootTwo = std::sqrt(2.0f) * 2.0f;

float AmbisonicPanner::getHorizontalAngle() const
{
	return horizontalAngle;
}

void AmbisonicPanner::setHorizontalAngle(float value)
{
	horizontalAngle = value;
	syncCoefficients();
}

float AmbisonicPanner::getElevationAngle() const
{
	return elevationAngle;
}

void AmbisonicPanner::setElevationAngle(float value)
{
	elevationAngle = value;
	syncCoefficients();
}

void AmbisonicPanner::setAngle(float horizontalAngle, float elevationAngle)
{
	this->horizontalAngle = horizontalAngle;
	this->elevationAngle = elevationAngle;
	syncCoefficients();
}

sPlayable AmbisonicPanner::getPlayable() const
{
	return playable;
}

void AmbisonicPanner::setPlayable(const sPlayable &value)
{
	playable = value;
}

float AmbisonicPanner::getDistance() const
{
	return distance;
}

void AmbisonicPanner::setDistance(float value)
{
	distance = value;
}

void AmbisonicPanner::syncCoefficients()
{
	sincos = std::sin(horizontalAngle) * std::cos(elevationAngle);
	coscos = std::cos(horizontalAngle) * std::cos(elevationAngle);
}

FrameCount AmbisonicPanner::fillBuffer(FrameCount maxFrames)
{
	if(playable) {
		FrameCount currentlyDoneFrames(0);
		FrameCount framesProcessedSoFar(0);
		FrameCount framesToGo = std::min(FrameCount(buffer.size()),maxFrames);
		do {
			Output out;
			out.dst = &buffer[framesProcessedSoFar.var];
			out.channels = ChannelCount(1);
			out.framerate = playable->getFrameRate();
			out.interleaving = InterleavingType::DONT_CARE;
			out.frameCount = framesToGo;
			currentlyDoneFrames = playable->outputTo(out);
			framesProcessedSoFar += currentlyDoneFrames;
			framesToGo -= currentlyDoneFrames;
		} while(framesToGo.var || !currentlyDoneFrames.var);
		return framesProcessedSoFar;
	} else return FrameCount(0);
}

FrameRate AmbisonicPanner::getFrameRate() const
{
	return playable ? playable->getFrameRate() : FrameRate(0);
}

ChannelCount AmbisonicPanner::getChannelCount() const
{
	return ChannelCount(4);
}

AmbisonicPanner::AmbisonicPanner(FrameCount bufferSize) : buffer(bufferSize.var), distance(0.0f)
{

}

FrameCount AmbisonicPanner::outputTo(const Output &dst)
{
	if(!playable) return FrameCount(0);
	if(dst.channels.var != 4) throw ChannelCountMismatchError(dst.channels,ChannelCount(4));
	if(dst.framerate != playable->getFrameRate()) throw FrameratetMismatchError(dst.framerate,playable->getFrameRate());
	if(dst.interleaving != InterleavingType::INTERLEAVED) throw InterleavingMismatchError(dst.interleaving,InterleavingType::INTERLEAVED);
	FrameCount inFrames = fillBuffer(dst.frameCount);
	for(const auto it : inFrames) {
		const float S = buffer[it] / std::max(1.0f,distance);
		float * const outFrameStart = &dst.dst[it * 4];

		// Left-Front
		// Right-Front
		// Left-Back
		// Right-Back
		outFrameStart[0] = twoRootTwo * (S + (S*coscos) + (S*sincos)); // Left-Front
		outFrameStart[1] = twoRootTwo * (S + (S*coscos) - (S*sincos)); // Right-Front
		outFrameStart[2] = twoRootTwo * (S + (S*sincos) - (S*coscos)); // Left-Back
		outFrameStart[3] = twoRootTwo * (S - (S*coscos) - (S*sincos)); // Right-Back
	}
	return inFrames;
}
}
}
