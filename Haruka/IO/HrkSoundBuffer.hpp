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

namespace Hrk {

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

}
#endif // HRKSOUNDBUFFER_HPP
