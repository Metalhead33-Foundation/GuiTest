#ifndef AUDIOSDL_HPP
#define AUDIOSDL_HPP
#include <MhLib/Media/AdvancedAudio/MhAudioPlayable.hpp>
#include <SDL2/SDL.h>

namespace Driver {

	class SDL {
	  private:
		SDL_AudioDeviceID id;
		SDL_AudioSpec have;
		MH33::Audio::sPlayable playable;

	  public:
		SDL( int freq, uint8_t channels, uint16_t frameCnt );
		~SDL( );
		static void SDL_AudioCallback( void *userdata, Uint8 *stream, int len );
		void lock( );
		void unlock( );
		void pause( bool val );
		MH33::Audio::sPlayable getPlayable( ) const;
		void setPlayable( const MH33::Audio::sPlayable &value );
		void setPlayable( MH33::Audio::sPlayable &&value );
		int getFreq( ) const;
		Uint8 getChannels( ) const;
		Uint16 getSamples( ) const;
	};

}

#endif // AUDIOSDL_HPP
