#include "HarukaContext.hpp"

Haruka_SDL_Driver::Haruka_SDL_Driver(int freq, uint8_t channels, uint16_t frameCnt)
{
	SDL_AudioSpec want;
	SDL_memset( &want, 0, sizeof( want ) );
	want.freq = freq;
	want.format = AUDIO_F32;
	want.channels = channels;
	want.samples = frameCnt * channels;
	want.callback = SDL_AudioCallback;
	want.userdata = this;
	id = SDL_OpenAudioDevice( nullptr, 0, &want, &have, 0 );
}

Haruka_SDL_Driver::~Haruka_SDL_Driver()
{
	if ( id )
		SDL_CloseAudioDevice( id );
}

void Haruka_SDL_Driver::SDL_AudioCallback(void* userdata, Uint8* stream, int len)
{
	memset( stream, 0, len );
	if ( userdata ) {
		auto chandle = reinterpret_cast< Haruka_SDL_Driver * >( userdata );
		if ( chandle->playable ) {
			Hrk::Output out;
			out.dst = reinterpret_cast< float * >( stream );
			out.samplerate = Hrk::SampleRate(chandle->have.freq);
			out.channels = Hrk::ChannelCount{chandle->have.channels};
			out.frameCount = Hrk::FrameCount(len / sizeof( float ) / out.channels.var);
			out.interleaving = Hrk::InterleavingType::INTERLEAVED;
			chandle->playable->outputTo( out );
		}
	}
}

void Haruka_SDL_Driver::lock()
{
	if ( id )
		SDL_LockAudioDevice( id );
}

void Haruka_SDL_Driver::unlock()
{
	if ( id )
		SDL_UnlockAudioDevice( id );
}

void Haruka_SDL_Driver::pause(bool val)
{
	if ( id )
		SDL_PauseAudioDevice( id, val );
}

Hrk::sPlayable Haruka_SDL_Driver::getPlayable() const
{
	return playable;
}

void Haruka_SDL_Driver::setPlayable(const Hrk::sPlayable& value)
{
	playable = value;
}

void Haruka_SDL_Driver::setPlayable(Hrk::sPlayable&& value)
{
	playable = std::move(value);
}

int Haruka_SDL_Driver::getFreq() const
{
	return have.freq;
}

Uint8 Haruka_SDL_Driver::getChannels() const
{
	return have.channels;
}

Uint16 Haruka_SDL_Driver::getSamples() const
{
	return have.samples;
}
