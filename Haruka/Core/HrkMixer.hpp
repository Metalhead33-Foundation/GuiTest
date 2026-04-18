#ifndef HRKMIXER_HPP
#define HRKMIXER_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <vector>
#include <memory_resource>
#include <unordered_map>
namespace Hrk {

class MH_HARUKA_API Mixer : public Playable
{
public:
	typedef std::pmr::unordered_map<Playable*,float> MixerMap; //! To be fair, it could also be an std::shared_ptr<Playable> or std::weak_ptr<Playable> to prevent issues where the Mixer outlives its children... but that would prevent us from allocating any Playables on the stack.
private:
	ChannelCount channelCount;
	InterleavingType interleavingType;
	SampleRate sampleRate;
	FrameCount frameCount;
	std::pmr::vector<float> buffer; //! We utilize double-buffering. The buffer is actually divided in half, the size will be 2*frameCount*channelCount.
	MixerMap children;
	FrameCount processIndividual(Playable& playable, float volume);
	FrameCount process();
public:
	Mixer(ChannelCount channelCount, InterleavingType interleavingType, SampleRate sampleRate, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	void reset(ChannelCount channelCount, InterleavingType interleavingType, SampleRate sampleRate, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	FrameCount outputTo(Output& output) override;
	ChannelCount getChannelCount() const;
	InterleavingType getInterleavingType() const;
	void setInterleavingType(InterleavingType newInterleavingType);
	SampleRate getSampleRate() const;
	void setSampleRate(SampleRate newSampleRate);
	FrameCount getFrameCount() const;
	const MixerMap& getChildren() const;
	MixerMap& getChildren();
};

}
#endif // HRKMIXER_HPP
