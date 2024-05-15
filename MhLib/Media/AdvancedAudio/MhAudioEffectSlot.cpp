#include <MhLib/Media/AdvancedAudio/MhAudioEffectSlot.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>
#include <cstring>

namespace MH33 {
namespace Audio {

sPlayable EffectSlot::getPlayable() const
{
	return playable;
}

void EffectSlot::setPlayable(const sPlayable &value)
{
	playable = value;
}

void EffectSlot::refreshInterlacingRequirement()
{
	requiresInterlacing = false;
	for(const auto& it : slots) {
		requiresInterlacing = std::max(requiresInterlacing,it.first->requiresDeinterlacing());
	}
}

FrameCount EffectSlot::fillBuffA(FrameCount maxFrames)
{
	FrameCount currentlyDoneFrames(0);
	FrameCount framesProcessedSoFar(0);
	FrameCount framesToGo = std::min(maxFrames,this->bufferSize);
	do {
		Output out;
		out.dst = &buffA[framesToSamples(framesProcessedSoFar,channels).var];
		out.channels = this->channels;
		out.framerate = this->framerate;
		out.interleaving = InterleavingType::INTERLEAVED;
		out.frameCount = framesToGo;
		currentlyDoneFrames = playable->outputTo(out);
		framesProcessedSoFar += currentlyDoneFrames;
		framesToGo -= currentlyDoneFrames;
	} while(framesToGo.var || !currentlyDoneFrames.var);
	return framesProcessedSoFar;
}

void EffectSlot::deinterlace(FrameCount maxFrames)
{
	std::vector<float*> ptrs(channels.var);
	const FrameCount framesToDo = std::min(maxFrames,bufferSize);
	for(const auto it : channels) {
		ptrs[it] = &buffB[framesToDo.var * it];
	}
	for(const auto it : framesToDo) {
		const float* const start = &buffA[it * channels.var];
		for(const auto zt : channels) {
			ptrs[zt][it] = start[zt];
		}
	}
}

void EffectSlot::interlace(FrameCount maxFrames)
{
	std::vector<const float*> ptrs(channels.var);
	const FrameCount framesToDo = std::min(maxFrames,bufferSize);
	for(const auto it : channels) {
		ptrs[it] = &buffB[framesToDo.var * it];
	}
	for(const auto it : framesToDo) {
		float* const start = &buffA[it * channels.var];
		for(const auto zt : channels) {
			start[zt] = ptrs[zt][it];
		}
	}
}

EffectSlot::EffectSlot(FrameRate framerate, ChannelCount channels, FrameCount bufferSize)
	: framerate(framerate), channels(channels), bufferSize(bufferSize)
{
	const SampleCount samples = framesToSamples(bufferSize,channels);
	buffA.resize(samples.var);
	buffB.resize(samples.var);
}

FrameCount EffectSlot::outputTo(const Output &dst)
{
	if(dst.channels != this->channels) throw ChannelCountMismatchError(dst.channels,this->channels);
	if(dst.framerate != this->framerate) throw FrameratetMismatchError(dst.framerate,this->framerate);
	if(requiresInterlacing) {
		const FrameCount gotAudio = fillBuffA(dst.frameCount);
		const SampleCount samples = framesToSamples(gotAudio,channels);
		deinterlace(gotAudio);
		ModificationStream stream;
		stream.src = buffB.data();
		stream.dst = buffA.data();
		stream.channels = channels;
		stream.framerate = framerate;
		stream.frameCount = gotAudio;
		stream.interleaving = InterleavingType::SEPARATE_CHANNELS;
		for(auto it = std::begin(slots); it != std::end(slots); ++it) {
			const float wet = it->second;
			const float dry = 1.0f - wet;
			it->first->doEffect(stream);
			for(const auto zt : samples) {
				const float sample = (buffA[zt] * wet) + (buffB[zt] * dry);
				buffB[zt] = sample;
			}
		}
		if(dst.interleaving == InterleavingType::SEPARATE_CHANNELS) {
			memcpy(dst.dst,buffB.data(),samplesToBytes(samples));
		} else {
			interlace(gotAudio);
			memcpy(dst.dst,buffA.data(),samplesToBytes(samples));
		}
		return gotAudio;
	} else {
		if(channels.var > 1 && dst.interleaving != InterleavingType::INTERLEAVED) throw InterleavingMismatchError(dst.interleaving,InterleavingType::INTERLEAVED);
		const FrameCount gotAudio = fillBuffA(dst.frameCount);
		const SampleCount samples = framesToSamples(gotAudio,channels);
		ModificationStream stream;
		stream.src = buffA.data();
		stream.dst = buffB.data();
		stream.channels = channels;
		stream.framerate = framerate;
		stream.frameCount = gotAudio;
		stream.interleaving = InterleavingType::SEPARATE_CHANNELS;
		for(auto it = std::begin(slots); it != std::end(slots); ++it) {
			const float wet = it->second;
			const float dry = 1.0f - wet;
			it->first->doEffect(stream);
			for(const auto zt : samples) {
				const float sample = (buffA[zt] * wet) + (buffB[zt] * dry);
				buffB[zt] = sample;
			}
		}
		memcpy(dst.dst,buffA.data(),samplesToBytes(samples));
		return gotAudio;
	}
}

const EffectSlot::SlotEntry &EffectSlot::operator[](size_t i) const
{
	return slots[i];
}

EffectSlot::SlotEntry &EffectSlot::operator[](size_t i)
{
	return slots[i];
}

const EffectSlot::SlotEntry &EffectSlot::at(size_t i) const
{
	return slots.at(i);
}

EffectSlot::SlotEntry &EffectSlot::at(size_t i)
{
	return slots.at(i);
}

EffectSlot::iterator EffectSlot::erase(EffectSlot::const_iterator pos)
{
	iterator toReturn = slots.erase(pos);
	refreshInterlacingRequirement();
	return toReturn;
}

EffectSlot::iterator EffectSlot::insert(EffectSlot::const_iterator pos, const EffectSlot::SlotEntry &entry)
{
	iterator toReturn =  slots.insert(pos,entry);
	refreshInterlacingRequirement();
	return toReturn;
}

void EffectSlot::push_back(const EffectSlot::SlotEntry &entry)
{
	slots.push_back(entry);
	refreshInterlacingRequirement();
}

void EffectSlot::push_front(const EffectSlot::SlotEntry &entry)
{
	slots.push_front(entry);
	refreshInterlacingRequirement();
}

EffectSlot::const_iterator EffectSlot::find(sEffect effect) const
{
	for(auto it = std::begin(slots); it != std::end(slots); ++it) {
		if(it->first == effect) return it;
	}
	return std::end(slots);
}

EffectSlot::iterator EffectSlot::find(sEffect effect)
{
	for(auto it = std::begin(slots); it != std::end(slots); ++it) {
		if(it->first == effect) return it;
	}
	return std::end(slots);
}
EffectSlot::const_iterator EffectSlot::begin() const { return std::begin(slots); }
EffectSlot::iterator EffectSlot::begin() { return std::begin(slots); }
EffectSlot::const_iterator EffectSlot::end() const { return std::end(slots); }
EffectSlot::iterator EffectSlot::end() { return std::end(slots); }


FrameRate EffectSlot::getFrameRate() const
{
	return framerate;
}

ChannelCount EffectSlot::getChannelCount() const
{
	return channels;
}

size_t EffectSlot::size() const
{
	return slots.size();
}

}
}
