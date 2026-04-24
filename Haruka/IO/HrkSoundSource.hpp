#ifndef HRKSOUNDSOURCE_HPP
#define HRKSOUNDSOURCE_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
#include <Haruka/IO/HrkPlaybackState.hpp>
namespace Hrk {
class MH_HARUKA_API SoundSource : public Playable, public IHasPlaybackState
{
private:
	SoundBufferHandle buffer;
	PlaybackState state;
public:
	SoundSource();
	const SoundBufferHandle& getBuffer() const;
	void setBuffer(const SoundBufferHandle& newBuffer);
	void setBuffer(SoundBufferHandle&& newBuffer);
	Euph::Media::Audio::FrameCount outputTo(Output& output) override;
	const PlaybackState& getState() const override;
	PlaybackState& getState() override;
};

}
#endif // HRKSOUNDSOURCE_HPP
