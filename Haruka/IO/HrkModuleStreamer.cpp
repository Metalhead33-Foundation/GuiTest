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

ModuleStreamer::ModuleStreamer(Elv::Io::uDevice&& fileDev, FrameCount ringCapacityFrames, std::pmr::memory_resource* memRes)
	: RingBufferedStreamer(ringCapacityFrames, memRes), moduleRenderer(std::move(fileDev))
{}

void ModuleStreamer::resolveOutputFormat(Output& output, ChannelCount& streamChannels, SampleRate& streamSampleRate)
{
	// Holy shit... openmpt has no way to retrieve the current frame position. So we can't even seek at all? Wtf.
	if(output.samplerate == SAMPLE_RATE_DONT_CARE) {
		output.samplerate = SampleRate(44100);
	}
	if(output.interleaving == InterleavingType::DONT_CARE) {
		output.interleaving = InterleavingType::INTERLEAVED;
	}
	if(output.channels.var != 1 && output.channels.var != 2 && output.channels.var != 4) {
		throw std::runtime_error("Invalid number of channels!");
	}
	moduleRenderer.setRepeating(state.getRepeat());
	streamChannels = output.channels;
	streamSampleRate = output.samplerate;
}

FrameCount ModuleStreamer::decodeFrames(float* dstInterleaved, FrameCount frameCount, ChannelCount streamChannels, SampleRate streamSampleRate)
{
	switch(streamChannels.var) {
		case 1: return moduleRenderer.readMono(streamSampleRate, frameCount, dstInterleaved);
		case 2: return moduleRenderer.readInterleavedStereo(streamSampleRate, frameCount, dstInterleaved);
		case 4: return moduleRenderer.readInterleavedQuad(streamSampleRate, frameCount, dstInterleaved);
		default: throw std::runtime_error("Invalid number of channels!");
	}
}

bool ModuleStreamer::seekDecoder(FrameIndex cursor, ChannelCount, SampleRate streamSampleRate)
{
	if(!streamSampleRate.var) return false;
	moduleRenderer.setPosition(double(cursor.var) / double(streamSampleRate.var));
	return true;
}

bool ModuleStreamer::rewindDecoder(ChannelCount, SampleRate)
{
	moduleRenderer.setPosition(0.0);
	return true;
}

}
