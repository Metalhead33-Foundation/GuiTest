#ifndef SOUNDSOURCE_HPP
#define SOUNDSOURCE_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioStatus.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioBuffer.hpp>

namespace MH33 {
namespace Audio {
class MH_ADV_AUDIO_API SoundSource : public Playable
{
private:
	sBuffer buffer;
	FrameIndex cursor;
	PlayStatus state;
	bool repeating;
public:
	SoundSource();
	SoundSource(const sBuffer &value);
	SoundSource(sBuffer&& value);
	FrameCount outputTo(const Output& dst);
	const sBuffer& getBuffer() const;
	sBuffer& getBuffer();
	void setBuffer(const sBuffer &value);
	void setBuffer(sBuffer&& value);
	FrameIndex getCursor() const;
	void setCursor(const FrameIndex &value);
	PlayStatus getState() const;
	void setState(const PlayStatus &value);
	bool getRepeating() const;
	void setRepeating(bool value);
	FrameRate getFrameRate() const;
	ChannelCount getChannelCount() const;
};

}
}
#endif // SOUNDSOURCE_HPP
