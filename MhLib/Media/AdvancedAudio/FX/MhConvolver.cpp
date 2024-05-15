#include <MhLib/Media/AdvancedAudio/FX/MhConvolver.hpp>
#include <MhLib/Media/AdvancedAudio/MhAudioError.hpp>

namespace MH33 {
namespace Audio {
namespace  FX {

Convolver::Convolver(ChannelCount channels) : channels(channels), convolvers(channels.var)
{

}

void Convolver::doEffect(const ModificationStream &modstream)
{
	if(modstream.channels != this->channels) throw ChannelCountMismatchError(this->channels,modstream.channels);
	for(const auto& it : convolvers) {
		if(modstream.framerate != it.getIrBuffer()->getFramerate()) throw FrameratetMismatchError(it.getIrBuffer()->getFramerate(),modstream.framerate);
	}
	if(channels.var > 1 && modstream.interleaving != InterleavingType::SEPARATE_CHANNELS) throw InterleavingMismatchError(InterleavingType::SEPARATE_CHANNELS,modstream.interleaving);
	for(const auto it : channels) {
		const float* const in = &modstream.src[it * modstream.frameCount.var];
		float* const out = &modstream.dst[it * modstream.frameCount.var];
		convolvers[it].process(in,out,modstream.frameCount.var);
	}
}

bool Convolver::requiresDeinterlacing()
{
	return true;
}

const fftconvolver::sIrBuffer Convolver::getBuffer(ChannelIndex channel) const
{
	return convolvers[channel.var].getIrBuffer();
}

void Convolver::setBuffer(ChannelIndex channel, fftconvolver::sIrBuffer buffer)
{
	convolvers[channel.var].init(buffer);
}

}
}
}
