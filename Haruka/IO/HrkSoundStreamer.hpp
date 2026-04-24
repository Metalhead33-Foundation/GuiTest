#ifndef HRKSOUNDSTREAMER_HPP
#define HRKSOUNDSTREAMER_HPP
#include <Haruka/IO/HrkRingBufferedStreamer.hpp>
#include <Euphemy/Media/Audio/EuphSoundFile.hpp>

namespace Hrk {
class MH_HARUKA_API SoundStreamer : public RingBufferedStreamer
{
private:
	Euph::Media::Audio::SoundFile soundfile;
	void resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate) override;
	FrameCount decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool seekDecoder(FrameIndex cursor, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool rewindDecoder(ChannelCount streamChannels, SampleRate streamSampleRate) override;
public:
	SoundStreamer(Elv::Io::uDevice&& fileDev, FrameCount ringCapacityFrames = FrameCount(8192), std::pmr::memory_resource* memRes = get_default_memory_resource());
	const Euph::Media::Audio::SoundFile& getSoundfile() const;
	Euph::Media::Audio::SoundFile& getSoundfile();
};
}

#endif // HRKSOUNDSTREAMER_HPP
