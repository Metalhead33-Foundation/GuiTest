#include <MhLib/Media/AdvancedAudio/MhMatrixPanner.hpp>
#include <cstring>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>

namespace MH33 {
namespace Audio {
sPlayable MatrixPanner::getPlayable() const
{
	return playable;
}

void MatrixPanner::setPlayable(const sPlayable &value)
{
	playable = value;
}

ChannelCount MatrixPanner::getInChannels() const
{
	return inChannels;
}

void MatrixPanner::setInChannels(const ChannelCount &value)
{
	if(inChannels != value) {
		inChannels = value;
		refreshMatrix();
	}
}

ChannelCount MatrixPanner::getOutChannels() const
{
	return outChannels;
}

void MatrixPanner::setOutChannels(const ChannelCount &value)
{
	if(outChannels != value) {
		outChannels = value;
		refreshMatrix();
	}
}

float &MatrixPanner::matrixVal(ChannelIndex inChan, ChannelIndex outChan)
{
	return matrix[getMatrixPos(inChan,outChan)];
}

const float &MatrixPanner::matrixVal(ChannelIndex inChan, ChannelIndex outChan) const
{
	return matrix[getMatrixPos(inChan,outChan)];
}

void MatrixPanner::refreshMatrix()
{
	matrix.resize(inChannels.var * outChannels.var);
	memset(matrix.data(),0,sizeof(float)*matrix.size());
}

int MatrixPanner::getMatrixPos(ChannelIndex inChan, ChannelIndex outChan) const
{
	return int(inChan.var * outChannels.var) + outChan.var;
}

FrameCount MatrixPanner::fillBuffer(FrameCount maxFrames)
{
	if(playable) {
		FrameCount currentlyDoneFrames(0);
		FrameCount framesProcessedSoFar(0);
		FrameCount framesToGo = std::min(bufferSize,maxFrames);
		do {
			Output out;
			out.dst = &buffer[framesToSamples(framesProcessedSoFar,inChannels).var];
			out.channels = inChannels;
			out.framerate = playable->getFrameRate();
			out.interleaving = InterleavingType::INTERLEAVED;
			out.frameCount = framesToGo;
			currentlyDoneFrames = playable->outputTo(out);
			framesProcessedSoFar += currentlyDoneFrames;
			framesToGo -= currentlyDoneFrames;
		} while(framesToGo.var || !currentlyDoneFrames.var);
		return framesProcessedSoFar;
	} else return FrameCount(0);
}

MatrixPanner::MatrixPanner(FrameCount bufferSize, ChannelCount inChannels, ChannelCount outChannels)
	: bufferSize(bufferSize), inChannels(inChannels), outChannels(outChannels), buffer(framesToSamples(bufferSize,inChannels).var)
{
	refreshMatrix();
}

FrameCount MatrixPanner::outputTo(const Output &dst)
{
	if(!playable) return FrameCount(0);
	if(dst.channels != outChannels) throw ChannelCountMismatchError(dst.channels,outChannels);
	if(dst.framerate != playable->getFrameRate()) throw FrameratetMismatchError(dst.framerate,playable->getFrameRate());
	if(outChannels.var > 1 && dst.interleaving != InterleavingType::INTERLEAVED) throw InterleavingMismatchError(dst.interleaving,InterleavingType::INTERLEAVED);
	FrameCount inFrames = fillBuffer(dst.frameCount);
	if(inFrames.var) {
		memset(dst.dst,0,samplesToBytes(framesToSamples(inFrames,outChannels)));
		for(const auto it : inFrames) {
			const float * const inFrameStart = &buffer[it * inChannels.var];
			float * const outFrameStart = &dst.dst[it * outChannels.var];
			for(const auto oit : outChannels) {
				for(const auto iit : inChannels) {
					outFrameStart[oit] += inFrameStart[iit] * matrixVal(iit,oit);
				}
			}
		}
	}
	return inFrames;
}

FrameRate MatrixPanner::getFrameRate() const
{
	return playable ? playable->getFrameRate() : FrameRate(0);
}

ChannelCount MatrixPanner::getChannelCount() const
{
	return outChannels;
}
}
}
