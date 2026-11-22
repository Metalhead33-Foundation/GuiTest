#ifndef HRKMODULESTREAMER_HPP
#define HRKMODULESTREAMER_HPP
/**
 * @file HrkModuleStreamer.hpp
 * @brief Declares the HrkModuleStreamer API in the Haruka/IO module.
 *
 * This header is part of the public declaration surface for Haruka/IO.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/IO/HrkRingBufferedStreamer.hpp>
#include <Euphemy/Media/Audio/EuphModuleRenderer.hpp>

namespace Hrk {
DEFINE_CLASS_WITH_POLYMORPHIC_ALLOCATOR(ModuleStreamer)
/** @brief Ring-buffered streamer backed by a module/tracker renderer. */
class MH_HARUKA_API ModuleStreamer : public RingBufferedStreamer
{
private:
	Euph::Media::Audio::ModuleRenderer moduleRenderer;
	void resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate) override;
	FrameCount decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool seekDecoder(FrameIndex cursor, ChannelCount streamChannels, SampleRate streamSampleRate) override;
	bool rewindDecoder(ChannelCount streamChannels, SampleRate streamSampleRate) override;
public:
	/** @brief Opens a module stream from an I/O device. */
	ModuleStreamer(Elv::Io::uDevice&& fileDev, FrameCount ringCapacityFrames = FrameCount(8192), std::pmr::memory_resource* memRes = get_default_memory_resource());
	/** @brief Returns immutable access to the module renderer. */
	const Euph::Media::Audio::ModuleRenderer& getModuleRenderer() const;
	/** @brief Returns mutable access to the module renderer. */
	Euph::Media::Audio::ModuleRenderer& getModuleRenderer();

	// getState() is inherited from PlaybackStatePlayable: do NOT add a second
	// PlaybackState here. RingBufferedStreamer::outputTo() consults the inherited
	// `state`, so shadowing it would make every setPlayStatus()/setCursor() call
	// from the outside a no-op and playback would never start.
};
}
#endif // HRKMODULESTREAMER_HPP
