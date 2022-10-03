#include <MhLib/Media/AdvancedAudio/MhFramerateConverter.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>

namespace MH33 {
namespace Audio {

const sPlayable &FramerateConverter::getPlayable() const
{
	return playable;
}

void FramerateConverter::setPlayable(const sPlayable &value)
{
	playable = value;
}

FrameCount FramerateConverter::outputTo(const Output &dst)
{
	if(!playable) return FrameCount(0);
	if(dst.channels != this->channels) throw ChannelCountMismatchError(dst.channels,this->channels);
	if(channels.var > 1 && dst.interleaving != InterleavingType::INTERLEAVED) throw InterleavingMismatchError(dst.interleaving,InterleavingType::INTERLEAVED);
	double ratio = double(dst.framerate.var) / double(playable->getFrameRate().var) / double(speed);
	return resampler.read(ratio,dst.frameCount,dst.dst);
}

FrameRate FramerateConverter::getFrameRate() const
{
	return playable ? playable->getFrameRate() : FrameRate(0);
}

ChannelCount FramerateConverter::getChannelCount() const
{
	return channels;
}

long FramerateConverter::mh_sf_callback( void *self, float **data ) {
	if ( self ) {
		auto sampler = reinterpret_cast< MH33::Audio::FramerateConverter * >( self );
		auto playable = sampler->getPlayable();
		if(playable) {
		Output out;
		out.channels = sampler->channels;
		out.framerate = playable->getFrameRate();
		out.frameCount = sampler->bufferSize;
		out.interleaving = InterleavingType::INTERLEAVED;
		out.dst = sampler->buffer.data();
		FrameCount returnedFrames = playable->outputTo(out);
		*data = sampler->buffer.data();
		return returnedFrames.var;
		} else return 0;
	} else
		return 0;
}

float FramerateConverter::getSpeed() const
{
	return speed;
}

void FramerateConverter::setSpeed(float value)
{
	speed = value;
}

FramerateConverter::FramerateConverter(FrameCount bufferSize, ChannelCount channels, float speed, ResampleType resampleType)
	: channels(channels), resampleType(resampleType), bufferSize(bufferSize), resampler(mh_sf_callback,resampleType,channels,this), buffer(framesToSamples(bufferSize,channels).var),
	  speed(speed)
{

}

const CallbackResampler &FramerateConverter::getResampler() const
{
	return resampler;
}

CallbackResampler &FramerateConverter::getResampler()
{
	return resampler;
}

}
}
