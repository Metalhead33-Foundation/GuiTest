#include <MhLib/Media/AdvancedAudio/MhModulePlayer.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>

namespace MH33 {
namespace Audio {

const ModuleRenderer &ModulePlayer::getRenderer() const
{
	return renderer;
}

PlayStatus ModulePlayer::getState() const
{
	return state;
}

void ModulePlayer::setState(const PlayStatus &value)
{
	state = value;
	if(state == PlayStatus::STOPPED) renderer.setPosition(0.0);
}

FrameRate ModulePlayer::preferredSampleRate = FrameRate(44100);
ChannelCount ModulePlayer::preferredChannelCount = ChannelCount(2);
FrameRate ModulePlayer::getFrameRate() const
{
	return preferredSampleRate;
}

ChannelCount ModulePlayer::getChannelCount() const
{
	return preferredChannelCount;
}

ModulePlayer::ModulePlayer(const Io::sDevice &iodev)
	: renderer(iodev), state(PlayStatus::STOPPED)
{

}

ModulePlayer::ModulePlayer(Io::sDevice &&iodev)
	: renderer(std::move(iodev)), state(PlayStatus::STOPPED)
{

}

ModulePlayer::ModulePlayer(Io::DeviceCreator iodev_creator, Io::Mode mode)
	: renderer(iodev_creator,mode), state(PlayStatus::STOPPED)
{

}

ModulePlayer::ModulePlayer(Io::System &iosys, const char *path, Io::Mode mode)
	: renderer(iosys,path,mode), state(PlayStatus::STOPPED)
{

}

ModulePlayer::ModulePlayer(Io::System &iosys, const std::string &path, Io::Mode mode)
	: renderer(iosys,path,mode), state(PlayStatus::STOPPED)
{

}

FrameCount ModulePlayer::outputTo(const Output &dst)
{
	if(state != PlayStatus::PLAYING) return FrameCount(0);
	switch (dst.interleaving) {
	case InterleavingType::DONT_CARE:
	case InterleavingType::INTERLEAVED: {
		switch (dst.channels.var) {
		case 1: return renderer.readMono(dst.framerate,dst.frameCount,dst.dst);
		case 2: return renderer.readInterleavedStereo(dst.framerate,dst.frameCount,dst.dst);
		case 4: return renderer.readInterleavedQuad(dst.framerate,dst.frameCount,dst.dst);
		default: throw ChannelCountMismatchError(dst.channels,ChannelCount(0));
		}
	}
	case InterleavingType::SEPARATE_CHANNELS: {
		switch (dst.channels.var) {
		case 1: return renderer.readMono(dst.framerate,dst.frameCount,dst.dst);
		case 2: return renderer.readStereo(dst.framerate,dst.frameCount,dst.dst,&dst.dst[dst.frameCount.var]);
		case 4: return renderer.readQuad(dst.framerate,dst.frameCount,dst.dst,&dst.dst[dst.frameCount.var],
									 &dst.dst[dst.frameCount.var*2],&dst.dst[dst.frameCount.var*3]);
		default: throw ChannelCountMismatchError(dst.channels,ChannelCount(0));
		}
	}
	}
}

ModuleRenderer &ModulePlayer::getRenderer()
{
	return renderer;
}

}
}
