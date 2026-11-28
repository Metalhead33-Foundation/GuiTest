#include "HrkSoundPlayer.hpp"
#include <algorithm>

namespace Hrk {

SoundPlayer::SoundPlayer(Mixer& targetMixer, std::pmr::memory_resource* memRes)
	: mixer(&targetMixer), voices(memRes)
{
}

SoundPlayer::~SoundPlayer()
{
	// The player may well outlive nothing, but the mixer stores raw pointers: leaving
	// a voice registered would make the audio callback pull a destroyed source.
	stopAll();
}

float SoundPlayer::effectiveGain(const Voice& voice) const noexcept
{
	if (isMuted) {
		return 0.0f;
	}
	return std::max(voice.gain, 0.0f) * std::max(master, 0.0f);
}

void SoundPlayer::applyGain(Voice& voice)
{
	if (mixer == nullptr || !voice.source) {
		return;
	}
	auto& children = mixer->getChildren();
	const auto child = children.find(voice.source.get());
	if (child != children.end()) {
		child->second = effectiveGain(voice);
	}
}

bool SoundPlayer::play(const SoundBufferHandle& buffer, float gain)
{
	if (!buffer.valid()) {
		return false;
	}
	return play(SoundBufferHandle(buffer), gain);
}

bool SoundPlayer::play(SoundBufferHandle&& buffer, float gain)
{
	if (mixer == nullptr || !buffer.valid()) {
		return false;
	}
	Voice voice;
	voice.source = Elv::Util::pmr_make_unique<SoundSource>(voices.get_allocator().resource());
	if (!voice.source) {
		return false;
	}
	voice.gain = std::max(gain, 0.0f);
	voice.source->setBuffer(std::move(buffer));
	// `PlaybackState` starts out stopped; without this the first `update()` would
	// retire a voice that never played, and the source would produce no frames.
	voice.source->getState().setCursor(FrameIndex(0));
	voice.source->getState().setRepeat(false);
	voice.source->getState().setPlayStatus(PlayStatus::PLAYING);
	voices.push_back(std::move(voice));
	Voice& stored = voices.back();
	mixer->getChildren().emplace(static_cast<Playable*>(stored.source.get()),
								 effectiveGain(stored));
	return true;
}

void SoundPlayer::setMasterVolume(float newVolume)
{
	master = std::max(newVolume, 0.0f);
	for (Voice& voice : voices) {
		applyGain(voice);
	}
}

void SoundPlayer::setMuted(bool newMuted)
{
	if (isMuted == newMuted) {
		return;
	}
	isMuted = newMuted;
	for (Voice& voice : voices) {
		applyGain(voice);
	}
}

void SoundPlayer::update()
{
	if (mixer == nullptr) {
		voices.clear();
		return;
	}
	auto& children = mixer->getChildren();
	const auto finished = [](const Voice& voice) {
		return !voice.source ||
			   voice.source->getState().getPlayStatus() != PlayStatus::PLAYING;
	};
	for (auto it = voices.begin(); it != voices.end();) {
		if (!finished(*it)) {
			++it;
			continue;
		}
		children.erase(static_cast<Playable*>(it->source.get()));
		it = voices.erase(it);
	}
}

void SoundPlayer::stopAll()
{
	if (mixer != nullptr) {
		auto& children = mixer->getChildren();
		for (const Voice& voice : voices) {
			children.erase(static_cast<Playable*>(voice.source.get()));
		}
	}
	voices.clear();
}

} // namespace Hrk
