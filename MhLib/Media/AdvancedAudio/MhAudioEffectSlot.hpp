#ifndef EFFECTSLOT_HPP
#define EFFECTSLOT_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioEffect.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp>
#include <deque>
#include <vector>

namespace MH33 {
namespace Audio {

class MH_ADV_AUDIO_API EffectSlot : public Playable
{
public:
	typedef std::pair<sEffect,float> SlotEntry;
	typedef std::deque<SlotEntry> SlotVector;
	typedef SlotVector::iterator iterator;
	typedef SlotVector::const_iterator const_iterator;
	const FrameRate framerate;
	const ChannelCount channels;
	const FrameCount bufferSize;
private:
	bool requiresInterlacing;
	SlotVector slots;
	sPlayable playable;
	std::vector<float> buffA, buffB;
	void refreshInterlacingRequirement();
	FrameCount fillBuffA(FrameCount maxFrames);
	void deinterlace(FrameCount maxFrames);
	void interlace(FrameCount maxFrames);
public:
	EffectSlot(FrameRate framerate,ChannelCount channels, FrameCount bufferSize);
	FrameCount outputTo(const Output& dst);
	FrameRate getFrameRate() const;
	ChannelCount getChannelCount() const;
	// Vector wrappers
	size_t size() const;
	const SlotEntry& operator[](size_t i) const;
	SlotEntry& operator[](size_t i);
	const SlotEntry& at(size_t i) const;
	SlotEntry& at(size_t i);
	iterator erase(const_iterator pos);
	iterator insert(const_iterator pos, const SlotEntry& entry);
	void push_back(const SlotEntry& entry);
	void push_front(const SlotEntry& entry);
	const_iterator find(sEffect effect) const;
	iterator find(sEffect effect);
	const_iterator begin() const;
	iterator begin();
	const_iterator end() const;
	iterator end();
	sPlayable getPlayable() const;
	void setPlayable(const sPlayable &value);
};

}
}
#endif // EFFECTSLOT_HPP
