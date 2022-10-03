#ifndef MODULEPLAYER_HPP
#define MODULEPLAYER_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioStatus.hpp>
#include <MhLib/Media/Audio/MhModuleRenderer.hpp>

namespace MH33 {
namespace Audio {

class MH_ADV_AUDIO_API ModulePlayer : public Playable
{
private:
	ModuleRenderer renderer;
	PlayStatus state;
public:
	ModulePlayer(const Io::sDevice& iodev);
	ModulePlayer(Io::sDevice&& iodev);
	ModulePlayer(Io::DeviceCreator iodev_creator, Io::Mode mode);
	ModulePlayer(Io::System& iosys, const char* path, Io::Mode mode);
	ModulePlayer(Io::System& iosys, const std::string& path, Io::Mode mode);
	FrameCount outputTo(const Output& dst);
	ModuleRenderer& getRenderer();
	const ModuleRenderer& getRenderer() const;
	PlayStatus getState() const;
	void setState(const PlayStatus &value);
	FrameRate getFrameRate() const;
	ChannelCount getChannelCount() const;
	static FrameRate preferredSampleRate;
	static ChannelCount preferredChannelCount;
};

}
}
#endif // MODULEPLAYER_HPP
