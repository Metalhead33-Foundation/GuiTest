#ifndef HRKMATRIXPANNER_HPP
#define HRKMATRIXPANNER_HPP
#include <Haruka/Core/HrkProxyPlayable.hpp>
#include <vector>
#include <memory_resource>
#include <span>
namespace Hrk {

class MH_HARUKA_API MatrixPanner : public ProxyPlayable
{
private:
	ChannelCount inChannels;
	ChannelCount outChannels;
	InterleavingType interleavingType;
	FrameCount frameCount;
	std::pmr::vector<float> buffer; // Size: (inChannels*outChannels)+(inChannels*frameCount)? Why? Because start contains the matrix. Starting offset of the scratch memory is at (inChannels*outChannels).
public:
	FrameCount outputTo(Output& output) override;
	MatrixPanner(ChannelCount inChannels, ChannelCount outChannels, InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	void reset(ChannelCount inChannels, ChannelCount outChannels, InterleavingType interleavingType, FrameCount frameCount, std::pmr::memory_resource* memRes = std::pmr::get_default_resource());
	ChannelCount getInChannels() const;
	ChannelCount getOutChannels() const;
	FrameCount getFrameCount() const;
	InterleavingType getInterleavingType() const;
	void setInterleavingType(InterleavingType newInterleavingType);
	std::span<float> getMatrix();
	std::span<const float> getMatrix() const;
	SampleCount getBufferOffsetForInput() const;
	SampleCount getBufferOffsetFor(ChannelCount x, ChannelCount y) const;
	void setToIdentity();
};

}
#endif // HRKMATRIXPANNER_HPP
