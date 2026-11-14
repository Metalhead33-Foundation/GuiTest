#include "HrkConvolver.hpp"

#include <Haruka/Core/HrkAudioError.hpp>
#include <stdexcept>

namespace Hrk {
namespace FX {

const SampleCount Convolver::DEFAULT_BLOCK_SIZE = SampleCount(1024);

Convolver::Convolver(ChannelCount channels, SampleCount blockSize, std::pmr::memory_resource* memRes)
	: channelCount(channels), blockSize(blockSize),
	  convolvers(memRes), impulseResponses(memRes)
{
	convolvers.reserve(uintptr_t(channels.var));
	for (uintptr_t i = 0; i < uintptr_t(channels.var); ++i) {
		convolvers.emplace_back(std::make_unique<Euph::Media::Audio::FFTConvolver>());
	}
	impulseResponses.resize(uintptr_t(channels.var));
}

void Convolver::process(const ModificationStream& stream)
{
	if (!stream.frameCount.var) return;
	if (stream.channels != channelCount) throw ChannelCountMismatchError(stream.channels, channelCount);
	if (channelCount.var > 1 && stream.interleaving != InterleavingType::SEPARATE_CHANNELS) {
		throw InterleavingMismatchError(InterleavingType::SEPARATE_CHANNELS, stream.interleaving);
	}
	float* const dst = stream.dst;
	if (!dst) return;
	const float* const src = stream.src ? stream.src : dst;

	const uintptr_t frames = uintptr_t(stream.frameCount.var);
	const SampleCount sampleCount(frames);
	for (uintptr_t c = 0; c < uintptr_t(channelCount.var); ++c) {
		const scImpulseResponse& response = impulseResponses[c];
		if (!response) {
			throw std::runtime_error("Convolver: no impulse response configured for every channel");
		}
		if (stream.samplerate != SAMPLE_RATE_DONT_CARE && response->getSampleRate() != stream.samplerate) {
			throw SamplerateMismatchError(response->getSampleRate(), stream.samplerate);
		}
		convolvers[c]->process(src + c * frames, dst + c * frames, sampleCount);
	}
}

scImpulseResponse Convolver::getImpulseResponse(ChannelIndex channel) const
{
	if (channel.var >= channelCount.var) return nullptr;
	return impulseResponses[uintptr_t(channel.var)];
}

bool Convolver::setImpulseResponse(ChannelIndex channel, scImpulseResponse response)
{
	if (channel.var >= channelCount.var) return false;
	const uintptr_t index = uintptr_t(channel.var);
	if (!response || !response->getFrameCount().var) {
		impulseResponses[index].reset();
		convolvers[index]->reset();
		return false;
	}
	const bool initialized = convolvers[index]->init(blockSize, response->data(), SampleCount(uintptr_t(response->getFrameCount().var)));
	impulseResponses[index] = initialized ? std::move(response) : nullptr;
	return initialized;
}

bool Convolver::setImpulseResponse(scImpulseResponse response)
{
	bool allInitialized = true;
	for (uintptr_t c = 0; c < uintptr_t(channelCount.var); ++c) {
		allInitialized = setImpulseResponse(ChannelIndex(c), response) && allInitialized;
	}
	return allInitialized;
}

bool Convolver::isReady() const
{
	for (const scImpulseResponse& response : impulseResponses) {
		if (!response) return false;
	}
	return true;
}

void Convolver::reset()
{
	for (auto& convolver : convolvers) convolver->reset();
}

}
}
