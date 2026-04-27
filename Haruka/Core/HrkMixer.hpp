#ifndef HRKMIXER_HPP
#define HRKMIXER_HPP
/**
 * @file HrkMixer.hpp
 * @brief Declares the HrkMixer API in the Haruka/Core module.
 *
 * This header is part of the public declaration surface for Haruka/Core.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/Core/HrkPlayable.hpp>
#include <vector>
#include <memory_resource>
#include <unordered_map>
namespace Hrk {

/** @brief Mixes multiple playable children into one output stream. */
class MH_HARUKA_API Mixer : public Playable
{
public:
	/** @brief Map from child playable pointer to linear gain. */
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
	/** @brief Creates a mixer with the requested output format and scratch buffer size. */
	Mixer(ChannelCount channelCount, InterleavingType interleavingType, SampleRate sampleRate, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Reinitializes the mixer format and scratch buffer. */
	void reset(ChannelCount channelCount, InterleavingType interleavingType, SampleRate sampleRate, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	/** @brief Mixes child playables into the provided output packet. */
	FrameCount outputTo(Output& output) override;
	/** @brief Returns the mixer channel count. */
	ChannelCount getChannelCount() const;
	/** @brief Returns the current output interleaving mode. */
	InterleavingType getInterleavingType() const;
	/** @brief Sets the output interleaving mode. */
	void setInterleavingType(InterleavingType newInterleavingType);
	/** @brief Returns the mixer sample rate. */
	SampleRate getSampleRate() const;
	/** @brief Sets the mixer sample rate. */
	void setSampleRate(SampleRate newSampleRate);
	/** @brief Returns the frame block size used for mixing. */
	FrameCount getFrameCount() const;
	/** @brief Returns the immutable child gain map. */
	const MixerMap& getChildren() const;
	/** @brief Returns the mutable child gain map. */
	MixerMap& getChildren();
};

}
#endif // HRKMIXER_HPP
