#ifndef HRKPLAYBACKSTATE_HPP
#define HRKPLAYBACKSTATE_HPP
/**
 * @file HrkPlaybackState.hpp
 * @brief Declares the HrkPlaybackState API in the Haruka/IO module.
 *
 * This header is part of the public declaration surface for Haruka/IO.
 * It exposes types, functions, constants, and helpers used by clients
 * and backend implementations that include this module.
 */
#include <Haruka/Core/HrkPlayable.hpp>

namespace Hrk {
/** @brief Cursor, play/pause state, and repeat flag shared by stream playables. */
class MH_HARUKA_API PlaybackState
{
private:
	FrameIndex cursor;
	PlayStatus playStatus;
	bool repeat;
public:
	/** @brief Creates a stopped playback state at cursor zero. */
	PlaybackState();
	/** @brief Copies a playback state. */
	PlaybackState(const PlaybackState& cpy) = default;
	/** @brief Moves a playback state. */
	PlaybackState(PlaybackState&& mov) = default;
	/** @brief Copies playback-state fields. */
	PlaybackState& operator=(const PlaybackState& cpy) = default;
	/** @brief Moves playback-state fields. */
	PlaybackState& operator=(PlaybackState&& mov) = default;
	/** @brief Returns the current frame cursor. */
	FrameIndex getCursor() const;
	/** @brief Sets the current frame cursor. */
	void setCursor(Euph::Media::Audio::FrameIndex newCursor);
	/** @brief Returns the current play status. */
	PlayStatus getPlayStatus() const;
	/** @brief Sets the current play status. */
	void setPlayStatus(PlayStatus newPlayStatus);
	/** @brief Returns whether playback repeats at end of stream. */
	bool getRepeat() const;
	/** @brief Sets whether playback repeats at end of stream. */
	void setRepeat(bool newRepeat);
};

/** @brief Interface for objects that expose mutable playback state. */
class IHasPlaybackState {
public:
	/** @brief Releases the interface through a base pointer. */
	virtual ~IHasPlaybackState() = default;
	/** @brief Returns immutable playback state. */
	virtual const PlaybackState& getState() const = 0;
	/** @brief Returns mutable playback state. */
	virtual PlaybackState& getState() = 0;
};

}

#endif // HRKPLAYBACKSTATE_HPP
