#ifndef HRKSOUNDSTREAMER_HPP
#define HRKSOUNDSTREAMER_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
#include <Haruka/IO/HrkPlaybackState.hpp>
#include <Euphemy/Media/Audio/EuphSoundFile.hpp>

namespace Hrk {
class MH_HARUKA_API SoundStreamer : public Playable, public IHasPlaybackState
{
private:
	Euph::Media::Audio::SoundFile soundfile;
	PlaybackState state;
public:
	SoundStreamer(Elv::Io::uDevice&& fileDev);
	const PlaybackState& getState() const override;
	PlaybackState& getState() override;
	FrameCount outputTo(Output& output) override;
	const Euph::Media::Audio::SoundFile& getSoundfile() const;
	Euph::Media::Audio::SoundFile& getSoundfile();
};
}

#endif // HRKSOUNDSTREAMER_HPP
