#ifndef HRKPLAYABLE_HPP
#define HRKPLAYABLE_HPP
/**
 * @file HrkPlayable.hpp
 * @brief Declares the HrkPlayable API in the Haruka/Core module.
 *
 * This header is part of the public declaration surface for Haruka/Core.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/HarukaLib.hpp>
#include <Haruka/Core/HrkAudioPacket.hpp>
namespace Hrk {

/** @brief Abstract source or processor that writes audio frames into an output packet. */
class Playable {
public:
	/** @brief Releases a playable through the base interface. */
	virtual ~Playable() = default;
	/** @brief Fills `output` and returns the number of frames actually produced. */
	virtual FrameCount outputTo(Output& output) = 0;
};

};
#endif // HRKPLAYABLE_HPP
