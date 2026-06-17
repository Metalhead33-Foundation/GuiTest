#ifndef HRKSOUNDSTREAMER_HPP
#define HRKSOUNDSTREAMER_HPP
/**
 * @file HrkSoundStreamer.hpp
 * @brief Declares the HrkSoundStreamer API in the Haruka/IO module.
 *
 * This header is part of the public declaration surface for Haruka/IO.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/IO/HrkRingBufferedStreamer.hpp>
#include <Euphemy/Media/Audio/EuphSoundFile.hpp>

namespace Hrk {
/** @brief Ring-buffered streamer backed by a decoded sound file. */
class MH_HARUKA_API SoundStreamer : public RingBufferedStreamer
{
private:
	Euph::Media::Audio::SoundFile soundfile;
	void resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate) override;
	FrameCount decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool seekDecoder(FrameIndex cursor, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool rewindDecoder(ChannelCount streamChannels, SampleRate streamSampleRate) override;
public:
	/** @brief Opens a sound stream from an I/O device. */
	SoundStreamer(Elv::Io::uDevice&& fileDev, FrameCount ringCapacityFrames = FrameCount(8192), std::pmr::memory_resource* memRes = get_default_memory_resource());
	/** @brief Returns immutable access to the sound file decoder. */
	const Euph::Media::Audio::SoundFile& getSoundfile() const;
	/** @brief Returns mutable access to the sound file decoder. */
	Euph::Media::Audio::SoundFile& getSoundfile();
};
}

#endif // HRKSOUNDSTREAMER_HPP
