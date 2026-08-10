#ifndef HRKPLAYBACKSTATEPLAYABLE_HPP
#define HRKPLAYBACKSTATEPLAYABLE_HPP
/**
 * @file HrkPlaybackStatePlayable.hpp
 * @brief Declares the HrkPlaybackStatePlayable API in the Haruka/IO module.
 *
 * This header is part of the public declaration surface for Haruka/IO.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/Core/HrkPlayable.hpp>
#include <Haruka/IO/HrkPlaybackState.hpp>

namespace Hrk {

/** @brief Playable base that stores and exposes a PlaybackState. */
class MH_HARUKA_API PlaybackStatePlayable : public Playable, public IHasPlaybackState
{
protected:
	/** @brief Documents the state declaration. */
	PlaybackState state;
public:
	/** @brief Returns immutable playback state. */
	const PlaybackState& getState() const override;

	/** @brief Returns mutable playback state. */
	PlaybackState& getState() override;
};

}

#endif // HRKPLAYBACKSTATEPLAYABLE_HPP
