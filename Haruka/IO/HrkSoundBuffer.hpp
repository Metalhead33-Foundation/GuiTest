#ifndef HRKSOUNDBUFFER_HPP
#define HRKSOUNDBUFFER_HPP
/**
 * @file HrkSoundBuffer.hpp
 * @brief Declares the HrkSoundBuffer API in the Haruka/IO module.
 *
 * This header is part of the public declaration surface for Haruka/IO.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>
#include <Elvavena/Util/ElvIntrusiveArray.hpp>
#include <cstddef>
#include <cstring>
#include <memory_resource>

namespace Hrk {

DEFINE_STRUCT_WITH_POLYMORPHIC_ALLOCATOR(SoundBufferMetadata)
/** @brief Metadata stored alongside an intrusive sound sample buffer. */
struct SoundBufferMetadata {
	/** @brief Number of frames held by the sample buffer. */
	FrameCount                  frameCount;
	/** @brief Sample rate of the stored frames. */
	SampleRate                  sampleRate;
	/** @brief Channel count of the stored frames. */
	ChannelCount                channels;
	/** @brief Interleaving layout of the stored samples. */
	InterleavingType            interleaving;
};

/** @brief Reference-counted intrusive float sample buffer with audio metadata. */
typedef Elv::Util::IntrusiveArray<float, SoundBufferMetadata,32> SoundBufferHandle;

/**
 * @brief Allocates a sound buffer and copies interleaved frames into it.
 *
 * This is the one place that knows how a @ref SoundBufferHandle is filled, so a
 * caller never has to reason about the intrusive header, the metadata fields or the
 * allocation lifetime: it hands over samples and gets a handle that every
 * `Hrk::SoundSource` can play.
 *
 * @param frames Interleaved float samples; `frameCount * channels` values.
 * @param frameCount Number of frames to copy.
 * @param channels Channel count of the interleaved data.
 * @param sampleRate Rate the frames were captured at.
 * @param interleaving Layout of the interleaved data.
 * @param memRes Memory resource that owns the slab; it must outlive the last handle.
 * @return The new buffer, or an invalid handle when the frame or channel count is 0.
 * @note The metadata is what `SoundSource::outputTo()` checks against the mixer, and
 *       a mismatch *throws from the audio callback*: the caller is responsible for
 *       having conformed the data first (see `Euph::Media::Audio::SoundClip`).
 */
MH_HARUKA_API SoundBufferHandle makeSoundBuffer(
	const float* frames, FrameCount frameCount, ChannelCount channels, SampleRate sampleRate,
	InterleavingType interleaving = InterleavingType::INTERLEAVED,
	std::pmr::memory_resource* memRes = std::pmr::get_default_resource());

}
#endif // HRKSOUNDBUFFER_HPP
