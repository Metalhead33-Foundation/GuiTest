#ifndef HRKMODULESTREAMER_HPP
#define HRKMODULESTREAMER_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
#include <Haruka/IO/HrkPlaybackState.hpp>
#include <Euphemy/Media/Audio/EuphModuleRenderer.hpp>

namespace Hrk {
class MH_HARUKA_API ModuleStreamer : public Playable, public IHasPlaybackState
{
private:
	Euph::Media::Audio::ModuleRenderer moduleRenderer;
	PlaybackState state;
public:
	ModuleStreamer(Elv::Io::uDevice&& fileDev);
	const PlaybackState& getState() const override;
	PlaybackState& getState() override;
	FrameCount outputTo(Output& output) override;
	const Euph::Media::Audio::ModuleRenderer& getModuleRenderer() const;
	Euph::Media::Audio::ModuleRenderer& getModuleRenderer();
};
}
#endif // HRKMODULESTREAMER_HPP
