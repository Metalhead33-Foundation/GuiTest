#ifndef HRKMODULESTREAMER_HPP
#define HRKMODULESTREAMER_HPP
#include <Haruka/IO/HrkRingBufferedStreamer.hpp>
#include <Euphemy/Media/Audio/EuphModuleRenderer.hpp>

namespace Hrk {
class MH_HARUKA_API ModuleStreamer : public RingBufferedStreamer
{
private:
	Euph::Media::Audio::ModuleRenderer moduleRenderer;
	void resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate) override;
	FrameCount decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool seekDecoder(FrameIndex cursor, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool rewindDecoder(ChannelCount streamChannels, SampleRate streamSampleRate) override;
public:
	ModuleStreamer(Elv::Io::uDevice&& fileDev, FrameCount ringCapacityFrames = FrameCount(8192), std::pmr::memory_resource* memRes = get_default_memory_resource());
	const Euph::Media::Audio::ModuleRenderer& getModuleRenderer() const;
	Euph::Media::Audio::ModuleRenderer& getModuleRenderer();
};
}
#endif // HRKMODULESTREAMER_HPP
