#include "HrkModuleStreamer.hpp"
#include <stdexcept>
namespace Hrk {
const Euph::Media::Audio::ModuleRenderer& ModuleStreamer::getModuleRenderer() const
{
	return moduleRenderer;
}

Euph::Media::Audio::ModuleRenderer& ModuleStreamer::getModuleRenderer()
{
	return moduleRenderer;
}

ModuleStreamer::ModuleStreamer(Elv::Io::uDevice&& fileDev)
	: moduleRenderer(std::move(fileDev))
{}

const PlaybackState& ModuleStreamer::getState() const
{
	return state;
}

PlaybackState& ModuleStreamer::getState()
{
	return state;
}

FrameCount ModuleStreamer::outputTo(Output& output)
{
	// TODO: We'll probably need to implement some sort of ringbuffer to store the samples.
	// I'm fairly sure that decoding on the go is going to be slow.
	// Also, this issue/conundrum/dilemma is shared with SoundStreamer.
	if(state.getPlayStatus() != PlayStatus::PLAYING) return FrameCount(0);
	// Holy shit... openmpt has no way to retrieve the current frame position. So we can't even seek at all? Wtf.
	/*if(state.getCursor() >= moduleRenderer.getFrameNum() ) {
		if(state.getRepeat()) { state.setCursor(FrameIndex(0)); }
		else {
			state.setPlayStatus(PlayStatus::STOPPED);
			return FrameCount(0);
		}
	} else soundfile.seekSet(FrameCount(state.getCursor().var));*/

	// openmpt is EXTREMELY tolerant and flexible when it comes to sampling rates, interleaving types and channel count
	if(output.samplerate == SAMPLE_RATE_DONT_CARE) {
		output.samplerate = 44100;
	}
	if(output.interleaving == InterleavingType::DONT_CARE) {
		output.interleaving = InterleavingType::INTERLEAVED;
	}
	if(state.getRepeat()) state.setCursor(FrameIndex(0)) ;
	else {
		state.setPlayStatus(PlayStatus::STOPPED);
		return FrameCount(0);
	}

	// TODO: We'll probably need to implement ringbuffering, because I have a strong suspicion that this naive method will be slow.
	FrameCount processed(0);
	switch (output.interleaving) {
		case InterleavingType::DONT_CARE: // Fall-through. Though this should never happen.
		case InterleavingType::INTERLEAVED: {
			switch(output.channels.var) {
				case 1: processed = moduleRenderer.readMono(output.samplerate,output.frameCount,output.dst); break;
				case 2: processed = moduleRenderer.readInterleavedStereo(output.samplerate,output.frameCount,output.dst); break;
				case 4: processed = moduleRenderer.readInterleavedQuad(output.samplerate,output.frameCount,output.dst); break;
				default: throw std::runtime_error("Invalid number of channels!");
			}
		}
		case InterleavingType::SEPARATE_CHANNELS: {
			switch(output.channels.var) {
				case 1: processed = moduleRenderer.readMono(output.samplerate,output.frameCount,output.dst); break;
				case 2: processed = moduleRenderer.readStereo(output.samplerate,output.frameCount,
														  output.dst,&output.dst[output.frameCount.var]); break;
				case 4: processed = moduleRenderer.readQuad(output.samplerate,output.frameCount,
														output.dst, &output.dst[output.frameCount.var],
														&output.dst[output.frameCount.var*2],&output.dst[output.frameCount.var*3]); break;
				default: throw std::runtime_error("Invalid number of channels!");
			}
		}
			break;
	}
	// And we'll need a more ergonomic means of updating the cursor of the state.
	state.setCursor(state.getCursor() + processed);
	return processed;
}

}