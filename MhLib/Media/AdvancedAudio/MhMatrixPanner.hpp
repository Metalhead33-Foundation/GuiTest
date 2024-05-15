#ifndef MATRIXPANNER_HPP
#define MATRIXPANNER_HPP
#include <MhLib/Media/AdvancedAudio/MhAdvancedAudio.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp>
#include <vector>
namespace MH33 {
namespace Audio {

class MH_ADV_AUDIO_API MatrixPanner : public Playable
{
private:
	sPlayable playable;
	FrameCount bufferSize;
	ChannelCount inChannels, outChannels;
	std::vector<float> matrix;
	std::vector<float> buffer;
	void refreshMatrix();
	int getMatrixPos(ChannelIndex inChan, ChannelIndex outChan) const;
	FrameCount fillBuffer(FrameCount maxFrames);
public:
	MatrixPanner(FrameCount bufferSize, ChannelCount inChannels, ChannelCount outChannels);
	FrameCount outputTo(const Output& dst);
	FrameRate getFrameRate() const;
	ChannelCount getChannelCount() const;
	sPlayable getPlayable() const;
	void setPlayable(const sPlayable &value);
	ChannelCount getInChannels() const;
	void setInChannels(const ChannelCount &value);
	ChannelCount getOutChannels() const;
	void setOutChannels(const ChannelCount &value);
	float& matrixVal(ChannelIndex inChan, ChannelIndex outChan);
	const float& matrixVal(ChannelIndex inChan, ChannelIndex outChan) const;
};

}
}

#endif // MATRIXPANNER_HPP
