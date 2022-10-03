#include <MhLib/Media/AdvancedAudio/MhAudioMixer.hpp>
#include <cstring>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>
namespace MH33 {
namespace Audio {

FrameCount Mixer::processIndividual(Playable &playable, float volume)
{
	FrameCount currentlyDoneFrames(0);
	FrameCount framesProcessedSoFar(0);
	FrameCount framesToGo = this->frameCount;
	do {
		Output out;
		out.dst = &buffB[framesToSamples(framesProcessedSoFar,channelCount).var];
		out.channels = this->channelCount;
		out.framerate = this->frameRate;
		out.interleaving = InterleavingType::INTERLEAVED;
		out.frameCount = framesToGo;
		currentlyDoneFrames = playable.outputTo(out);
		framesProcessedSoFar += currentlyDoneFrames;
		framesToGo -= currentlyDoneFrames;
	} while(framesToGo.var || !currentlyDoneFrames.var);
	for(const auto jt : framesProcessedSoFar) {
		const float * const inFrameStart = &buffB[jt * channelCount.var];
		float * const outFrameStart = &buffA[jt * channelCount.var];
		for(const auto zt : channelCount) {
			outFrameStart[zt] += inFrameStart[zt] * volume;
		}
	}
	return framesProcessedSoFar;
}

FrameCount Mixer::process()
{
	memset(this->buffA.data(),0,sizeof(float)*this->buffA.size());
	FrameCount largest(0);
	for(auto it = std::begin(playables); it != std::end(playables); ++it) {
		largest = std::max(largest,processIndividual(*it->first,it->second));
	}
	return largest;
}

Mixer::Mixer(FrameCount frameCount, FrameRate frameRate, ChannelCount channelCount)
	: frameCount(frameCount), frameRate(frameRate), channelCount(channelCount),
	  buffA(framesToSamples(frameCount,channelCount).var), buffB(framesToSamples(frameCount,channelCount).var)
{

}

FrameCount Mixer::outputTo(const Output &dst)
{
	if(dst.channels != this->channelCount) throw ChannelCountMismatchError(dst.channels,this->channelCount);
	if(dst.framerate != this->frameRate) throw FrameratetMismatchError(dst.framerate,this->frameRate);
	if(channelCount.var > 1 && dst.interleaving != InterleavingType::INTERLEAVED) throw InterleavingMismatchError(dst.interleaving,InterleavingType::INTERLEAVED);
	FrameCount processed = process();
	memcpy(dst.dst,buffA.data(),samplesToBytes(framesToSamples(processed,channelCount)));
	return processed;
}

FrameCount Mixer::getFrameCount() const
{
	return frameCount;
}

FrameRate Mixer::getFrameRate() const
{
	return frameRate;
}

ChannelCount Mixer::getChannelCount() const
{
	return channelCount;
}

bool Mixer::empty() const { return playables.empty(); }
size_t Mixer::size() const { return playables.size(); }
void Mixer::clear() { playables.clear(); }
std::pair<Mixer::iterator,bool> Mixer::set(const sPlayable& playable, float volume) {
	return playables.insert_or_assign(playable,volume);
}
std::pair<Mixer::iterator,bool> Mixer::set(sPlayable&& playable, float volume) {
	return playables.insert_or_assign(playable,volume);
}
Mixer::iterator Mixer::erase(iterator pos) {
	return playables.erase(pos);
}
Mixer::iterator Mixer::find(const sPlayable& playable) {
	return playables.find(playable);
}
Mixer::const_iterator Mixer::find(const sPlayable& playable) const {
	return playables.find(playable);
}
Mixer::iterator Mixer::begin() { return playables.begin(); }
Mixer::const_iterator Mixer::begin() const { return playables.begin(); }
Mixer::iterator Mixer::end() { return playables.end(); }
Mixer::const_iterator Mixer::end() const { return playables.end(); }

}
}
