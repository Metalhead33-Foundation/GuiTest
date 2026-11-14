#include "HrkEffectSlot.hpp"

#include <Haruka/Core/HrkAudioError.hpp>
#include <algorithm>
#include <cstring>
#include <utility>

namespace Hrk {

namespace {

/** @brief Clamps a dry/wet mix into the supported range. */
inline float clampWet(float wet) noexcept
{
	return std::clamp(wet, 0.0f, 1.0f);
}

/** @brief Splits interleaved frames into per-channel planes. */
void deinterlaceTo(const float* interleaved, float* planar, uintptr_t frames, uintptr_t channels) noexcept
{
	for (uintptr_t c = 0; c < channels; ++c) {
		float* const plane = planar + c * frames;
		for (uintptr_t f = 0; f < frames; ++f) plane[f] = interleaved[f * channels + c];
	}
}

/** @brief Merges per-channel planes into interleaved frames. */
void interlaceTo(const float* planar, float* interleaved, uintptr_t frames, uintptr_t channels) noexcept
{
	for (uintptr_t c = 0; c < channels; ++c) {
		const float* const plane = planar + c * frames;
		for (uintptr_t f = 0; f < frames; ++f) interleaved[f * channels + c] = plane[f];
	}
}

}

EffectSlot::EffectSlot(ChannelCount channelCount, SampleRate sampleRate, FrameCount bufferSize, std::pmr::memory_resource* memRes)
	: channelCount(channelCount), sampleRate(sampleRate), bufferSize(bufferSize),
	  interleavedBuffer(framesToSamples(bufferSize, channelCount).var, memRes),
	  planarBuffer(framesToSamples(bufferSize, channelCount).var, memRes),
	  scratchBuffer(framesToSamples(bufferSize, channelCount).var, memRes),
	  slots(memRes)
{
}

void EffectSlot::refreshInterleavingRequirement()
{
	planarSlotCount = 0;
	for (const SlotEntry& entry : slots) {
		if (entry.effect && entry.effect->requiresDeinterlacing()) ++planarSlotCount;
	}
}

FrameCount EffectSlot::pullSource(FrameCount maxFrames)
{
	FrameCount processed(0);
	FrameCount todo(std::min(maxFrames.var, bufferSize.var));
	while (todo.var) {
		Output out;
		out.dst = &interleavedBuffer[framesToSamples(processed, channelCount).var];
		out.channels = channelCount;
		out.samplerate = sampleRate;
		out.interleaving = InterleavingType::INTERLEAVED;
		out.frameCount = todo;
		const FrameCount produced = playable->outputTo(out);
		const FrameCount accepted(std::min(produced.var, todo.var));
		if (!accepted.var) break;
		processed += accepted.var;
		todo -= accepted.var;
	}
	return processed;
}

FrameCount EffectSlot::outputTo(Output& output)
{
	if (!output.frameCount.var) return FrameCount(0);
	if (output.channels != channelCount) throw ChannelCountMismatchError(output.channels, channelCount);
	if (output.samplerate == SAMPLE_RATE_DONT_CARE) {
		output.samplerate = sampleRate;
	} else if (output.samplerate != sampleRate) {
		throw SamplerateMismatchError(output.samplerate, sampleRate);
	}
	if (!playable) return FrameCount(0);

	const FrameCount frames = pullSource(output.frameCount);
	if (!frames.var) return FrameCount(0);

	const uintptr_t frameCount = uintptr_t(frames.var);
	const uintptr_t channels = uintptr_t(channelCount.var);
	const SampleCount samples = framesToSamples(frames, channelCount);

	// For a single channel the interleaved and planar layouts are byte-identical,
	// so the conversion passes are skipped even when an effect asked for planes.
	const bool planar = planarSlotCount > 0 && channels > 1;

	float* current = interleavedBuffer.data();
	if (planar) {
		deinterlaceTo(current, planarBuffer.data(), frameCount, channels);
		current = planarBuffer.data();
	}

	float* scratch = scratchBuffer.data();
	for (const SlotEntry& entry : slots) {
		if (!entry.effect) continue;
		ModificationStream stream;
		stream.src = current;
		stream.dst = scratch;
		stream.frameCount = frames;
		stream.samplerate = sampleRate;
		stream.channels = channelCount;
		stream.interleaving = planar ? InterleavingType::SEPARATE_CHANNELS : InterleavingType::INTERLEAVED;
		entry.effect->process(stream);

		const float wet = clampWet(entry.wet);
		if (wet < 1.0f) {
			const float dry = 1.0f - wet;
			const uintptr_t count = uintptr_t(samples.var);
			for (uintptr_t i = 0; i < count; ++i) {
				scratch[i] = current[i] * dry + scratch[i] * wet;
			}
		}
		std::swap(current, scratch);
	}

	const bool wantsPlanar = output.interleaving == InterleavingType::SEPARATE_CHANNELS && channels > 1;
	if (wantsPlanar) {
		const float* planarData = current;
		if (!planar) {
			// `current` may live in the scratch buffer, so convert into whichever
			// of the two planar-capable buffers is not holding the result.
			float* const target = (current == scratchBuffer.data()) ? planarBuffer.data() : scratchBuffer.data();
			deinterlaceTo(current, target, frameCount, channels);
			planarData = target;
		}
		std::memcpy(output.dst, planarData, samplesToBytes(samples));
	} else {
		const float* interleavedData = current;
		if (planar) {
			interlaceTo(current, interleavedBuffer.data(), frameCount, channels);
			interleavedData = interleavedBuffer.data();
		}
		std::memcpy(output.dst, interleavedData, samplesToBytes(samples));
	}
	return frames;
}

void EffectSlot::addEffect(Effect* effect, float wet)
{
	if (!effect) return;
	slots.push_back(SlotEntry{ effect, clampWet(wet) });
	if (effect->requiresDeinterlacing()) ++planarSlotCount;
}

bool EffectSlot::removeEffect(const Effect* effect)
{
	const std::size_t index = find(effect);
	if (index == npos) return false;
	slots.erase(slots.begin() + std::ptrdiff_t(index));
	refreshInterleavingRequirement();
	return true;
}

std::size_t EffectSlot::removeEffects(const Effect* effect)
{
	const std::size_t before = slots.size();
	slots.erase(std::remove_if(slots.begin(), slots.end(), [effect](const SlotEntry& entry) {
		return entry.effect == effect;
	}), slots.end());
	const std::size_t removed = before - slots.size();
	if (removed) refreshInterleavingRequirement();
	return removed;
}

void EffectSlot::clearEffects()
{
	slots.clear();
	planarSlotCount = 0;
}

bool EffectSlot::setWet(std::size_t index, float wet)
{
	if (index >= slots.size()) return false;
	slots[index].wet = clampWet(wet);
	return true;
}

std::size_t EffectSlot::find(const Effect* effect) const
{
	for (std::size_t i = 0; i < slots.size(); ++i) {
		if (slots[i].effect == effect) return i;
	}
	return npos;
}

}

