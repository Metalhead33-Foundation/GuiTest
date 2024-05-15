#include <MhLib/Media/AdvancedAudio/MhAudioBuffer.hpp>

namespace MH33 {
namespace Audio {

FrameCount Buffer::getFrameCount() const
{
	return frameCount;
}

FrameRate Buffer::getFrameRate() const
{
	return frameRate;
}

ChannelCount Buffer::getChannelCount() const
{
	return channelCount;
}

void Buffer::fillData(SoundFile &sndfile)
{
	this->channelCount = sndfile.getChannels();
	this->frameRate = sndfile.getFrameRate();
	this->frameCount = sndfile.getFrameNum();
	this->samples.resize(framesToSamples(this->frameCount,this->channelCount).var);
	sndfile.readf(this->samples.data(),sndfile.getFrameNum());
}

Buffer::Buffer(const Io::sDevice &iodev)
{
	SoundFile soundfile(iodev);
	fillData(soundfile);
}

Buffer::Buffer(Io::sDevice &&iodev)
{
	SoundFile soundfile(std::move(iodev));
	fillData(soundfile);
}

Buffer::Buffer(Io::DeviceCreator iodev_creator, Io::Mode mode)
{
	SoundFile soundfile(iodev_creator,mode);
	fillData(soundfile);
}

Buffer::Buffer(Io::System &iosys, const char *path, Io::Mode mode)
{
	SoundFile soundfile(iosys,path,mode);
	fillData(soundfile);
}

Buffer::Buffer(Io::System &iosys, const std::string &path, Io::Mode mode)
{
	SoundFile soundfile(iosys,path,mode);
	fillData(soundfile);
}

void Buffer::provideInput(Input &in, FrameIndex progress)
{
	in.channels = this->channelCount;
	in.framerate = this->frameRate;
	in.interleaving = InterleavingType::INTERLEAVED;
	in.frameCount = this->frameCount - FrameCount(progress.var);
	in.src = &samples[framesToSamples(FrameCount(progress.var),channelCount).var];
}

}
}
