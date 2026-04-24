#ifndef HRKSOUNDSOURCE_HPP
#define HRKSOUNDSOURCE_HPP
#include <Haruka/IO/HrkPlaybackStatePlayable.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
namespace Hrk {
class MH_HARUKA_API SoundSource : public PlaybackStatePlayable
{
private:
	SoundBufferHandle buffer;
public:
	SoundSource();
	const SoundBufferHandle& getBuffer() const;
	void setBuffer(const SoundBufferHandle& newBuffer);
	void setBuffer(SoundBufferHandle&& newBuffer);
	Euph::Media::Audio::FrameCount outputTo(Output& output) override;
};

}
#endif // HRKSOUNDSOURCE_HPP
