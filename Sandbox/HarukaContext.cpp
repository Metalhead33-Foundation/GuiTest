#include "HarukaContext.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

Haruka_SDL_Driver::Haruka_SDL_Driver(int freq, uint8_t channels, uint16_t frameCnt)
	: id(0), have{}, playable()
{
	SDL_AudioSpec want;
	SDL_memset( &want, 0, sizeof( want ) );
	want.freq = freq;
	want.format = AUDIO_F32;
	want.channels = channels;
	// SDL_AudioSpec::samples is the buffer size in sample FRAMES per channel,
	// not the total number of samples across all channels.
	want.samples = frameCnt;
	want.callback = SDL_AudioCallback;
	want.userdata = this;
	// A freshly opened device starts paused: nothing is played until pause(false).
	id = SDL_OpenAudioDevice( nullptr, 0, &want, &have, 0 );
	if ( id == 0 )
		SDL_Log( "SDL_OpenAudioDevice failed: %s", SDL_GetError() );
}

Haruka_SDL_Driver::~Haruka_SDL_Driver()
{
	if ( id )
		SDL_CloseAudioDevice( id );
}

void Haruka_SDL_Driver::SDL_AudioCallback(void* userdata, Uint8* stream, int len)
{
	std::memset( stream, 0, len );
	if ( !userdata || len <= 0 ) return;
	auto chandle = reinterpret_cast< Haruka_SDL_Driver * >( userdata );
	if ( !chandle->playable ) return;
	const uintptr_t channels = chandle->have.channels ? uintptr_t( chandle->have.channels ) : 1u;
	// A Mixer only ever produces its own block size per outputTo() call, so keep
	// pulling in a loop until the device buffer is filled (or the graph runs dry).
	float* cursor = reinterpret_cast< float * >( stream );
	uintptr_t remainingFrames = uintptr_t( len ) / sizeof( float ) / channels;
	while ( remainingFrames ) {
		Hrk::Output out;
		out.dst = cursor;
		out.samplerate = Hrk::SampleRate( chandle->have.freq );
		out.channels = Hrk::ChannelCount( channels );
		out.frameCount = Hrk::FrameCount( remainingFrames );
		out.interleaving = Hrk::InterleavingType::INTERLEAVED;
		const auto produced = chandle->playable->outputTo( out );
		if ( !produced.var ) break;
		const uintptr_t doneFrames = std::min< uintptr_t >( produced.var, remainingFrames );
		cursor += doneFrames * channels;
		remainingFrames -= doneFrames;
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
