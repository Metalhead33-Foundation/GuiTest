#ifndef HARUKACONTEXT_HPP
#define HARUKACONTEXT_HPP
#include <Haruka/Core/HrkPlayable.hpp>
#include <SDL2/SDL.h>

/*
 * This is the intended way to wire Haruka to an actual audio output.
 * Couldbe also done with portaudio, or any other callback-based audio library.
 * In theory, supports any `Hrk::Playable` - in practice, you're pretty much always gonna use it with `Hrk::Mixer`.
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

#endif // HARUKACONTEXT_HPP
