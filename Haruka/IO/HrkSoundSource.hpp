#ifndef HRKSOUNDSOURCE_HPP
#define HRKSOUNDSOURCE_HPP
/**
 * @file HrkSoundSource.hpp
 * @brief Declares the HrkSoundSource API in the Haruka/IO module.
 *
 * This header is part of the public declaration surface for Haruka/IO.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/IO/HrkPlaybackStatePlayable.hpp>
#include <Haruka/IO/HrkSoundBuffer.hpp>
namespace Hrk {
/** @brief Playback-state-aware source backed by an in-memory sound buffer. */
class MH_HARUKA_API SoundSource : public PlaybackStatePlayable
{
private:
	SoundBufferHandle buffer;
public:
	/** @brief Creates an empty sound source. */
	SoundSource();
	/** @brief Returns the current sound buffer handle. */
	const SoundBufferHandle& getBuffer() const;
	/** @brief Sets the current buffer by sharing an existing handle. */
	void setBuffer(const SoundBufferHandle& newBuffer);
	/** @brief Sets the current buffer by moving an existing handle. */
	void setBuffer(SoundBufferHandle&& newBuffer);
	/** @brief Writes buffered audio to `output` according to playback state. */
	Euph::Media::Audio::FrameCount outputTo(Output& output) override;
};

}
#endif // HRKSOUNDSOURCE_HPP
