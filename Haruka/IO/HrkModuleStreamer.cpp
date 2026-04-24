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
	// TODO: Add buffered prefetching if direct decode proves too expensive in realtime paths.
	// This concern is shared with SoundStreamer.
	if(state.getPlayStatus() != PlayStatus::PLAYING) return FrameCount(0);
	if(!output.frameCount.var) return FrameCount(0);
	// Holy shit... openmpt has no way to retrieve the current frame position. So we can't even seek at all? Wtf.

	// openmpt supports flexible render formats, so we normalize only when caller leaves fields unspecified.
	if(output.samplerate == SAMPLE_RATE_DONT_CARE) {
		output.samplerate = 44100;
	}
	if(output.interleaving == InterleavingType::DONT_CARE) {
		output.interleaving = InterleavingType::INTERLEAVED;
	}
	moduleRenderer.setRepeating(state.getRepeat());

	// TODO: Replace this direct-read path with a buffered strategy if needed.
	FrameCount processed(0);
	FrameCount remaining = output.frameCount;
	do {
		FrameCount chunk(0);
		switch (output.interleaving) {
			case InterleavingType::DONT_CARE: // Fall-through. Though this should never happen.
			case InterleavingType::INTERLEAVED: {
				float* const dst = &output.dst[Hrk::framesToSamples(processed, output.channels).var];
				switch(output.channels.var) {
					case 1: chunk = moduleRenderer.readMono(output.samplerate, remaining, dst); break;
					case 2: chunk = moduleRenderer.readInterleavedStereo(output.samplerate, remaining, dst); break;
					case 4: chunk = moduleRenderer.readInterleavedQuad(output.samplerate, remaining, dst); break;
					default: throw std::runtime_error("Invalid number of channels!");
				}
				break;
			}
			case InterleavingType::SEPARATE_CHANNELS: {
				switch(output.channels.var) {
					case 1:
						chunk = moduleRenderer.readMono(output.samplerate, remaining, &output.dst[processed.var]);
						break;
					case 2:
						chunk = moduleRenderer.readStereo(output.samplerate, remaining,
														  &output.dst[processed.var],
														  &output.dst[output.frameCount.var + processed.var]);
						break;
					case 4:
						chunk = moduleRenderer.readQuad(output.samplerate, remaining,
														&output.dst[processed.var],
														&output.dst[output.frameCount.var + processed.var],
														&output.dst[(output.frameCount.var * 2) + processed.var],
														&output.dst[(output.frameCount.var * 3) + processed.var]);
						break;
					default: throw std::runtime_error("Invalid number of channels!");
				}
				break;
			}
		}
		if(!chunk.var) {
			if(!state.getRepeat()) {
				state.setPlayStatus(PlayStatus::STOPPED);
			}
			break;
		}
		processed += chunk.var;
		remaining -= chunk.var;
	} while(remaining.var);
	state.setCursor(state.getCursor() + processed);
	return processed;
}

}
