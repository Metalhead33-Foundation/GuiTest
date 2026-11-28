#ifndef HARUKACONTEXT_HPP
#define HARUKACONTEXT_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <SDL2/SDL.h>

/*
 * This is the intended way to wire Haruka to an actual audio output.
 * Couldbe also done with portaudio, or any other callback-based audio library.
 * In theory, supports any `Hrk::Playable` - in practice, you're pretty much always gonna use it with `Hrk::Mixer`.
 *
 * NOTE: SDL audio devices are created paused. Call pause(false) once the playable
 * graph is wired up, otherwise the callback is never invoked and nothing is heard.
*/
class Haruka_SDL_Driver
{
private:
	SDL_AudioDeviceID id;
	SDL_AudioSpec have;
	Hrk::sPlayable playable;

public:
	Haruka_SDL_Driver( int freq, uint8_t channels, uint16_t frameCnt );
	~Haruka_SDL_Driver( );
	static void SDL_AudioCallback( void *userdata, Uint8 *stream, int len );
	void lock( );
	void unlock( );
	void pause( bool val );
	Hrk::sPlayable getPlayable( ) const;
	void setPlayable( const Hrk::sPlayable &value );
	void setPlayable( Hrk::sPlayable &&value );
	int getFreq( ) const;
	Uint8 getChannels( ) const;
	Uint16 getSamples( ) const;
};

/**
 * @brief Scoped lock over the SDL audio callback.
 *
 * `Hrk::Mixer`, `Hrk::PlaybackState` and `Hrk::SoundPlayer` are plain containers that
 * the audio callback reads. `SDL_LockAudioDevice` guarantees the callback is not
 * running, so a script that inserts a module, starts a one-shot or toggles playback
 * cannot race the mixer. A null driver (no audio device) makes the lock a no-op,
 * which lets every caller stay branch-free.
 */
class AudioLock
{
public:
	/// @brief Locks @p driver when it is not null.
	explicit AudioLock(Haruka_SDL_Driver* driver) : driver(driver)
	{
		if (driver != nullptr) driver->lock();
	}
	/// @brief Releases the lock.
	~AudioLock()
	{
		if (driver != nullptr) driver->unlock();
	}
	AudioLock(const AudioLock&) = delete;
	AudioLock& operator=(const AudioLock&) = delete;

private:
	/// @brief Device whose callback is held off.
	Haruka_SDL_Driver* driver;
};

#endif // HARUKACONTEXT_HPP
