#include "GameSystem.hpp"
#include <Euphemy/Io/EuphFile.hpp>

GameSystem::GameSystem(Euph::Conf::Configuration& config)
	: AppSystem(std::string(config.getString("Game", "sTitle", "GameDemo")),
				0,
				0,
				config.getUInteger("Video","uWidth", 640),
				config.getUInteger("Video","uHeight", 480),
				0), config(config)
{
	auto samplerate = config.getUInteger("Audio","uSamplerate",48000);
	auto channels = config.getUInteger("Audio","uChannels",2);
	auto bufferSize = config.getUInteger("Audio","uBufferSize",4096);
	sdlDriver = std::make_unique<Haruka_SDL_Driver>(
		samplerate,
		channels,
		bufferSize
		);
	/*
	Mixer(ChannelCount channelCount, InterleavingType interleavingType, SampleRate sampleRate, FrameCount frameCount);
*/
	mixer = Elv::Util::pmr_make_shared<Hrk::Mixer>(std::pmr::get_default_resource(),
		Hrk::ChannelCount(channels), Hrk::InterleavingType::INTERLEAVED, Hrk::SampleRate(samplerate), Hrk::FrameCount(bufferSize)
		);
	sdlDriver->setPlayable(mixer);
}

void GameSystem::initialize()
{
	/*
	 const char* path, Elv::Io::Mode mode
	*/
	std::string modpath = std::string(config.getString("Game", "sSongPath", "/home/legacy/zene/GameMusic/Unreal Tournament/mod/Magnus.it"));
	Euph::Io::uFile file = Elv::Util::pmr_make_unique<Euph::Io::File>(std::pmr::get_default_resource(), modpath.c_str(), Elv::Io::Mode::READ);
	/*
	ModuleStreamer(Elv::Io::uDevice&& fileDev, FrameCount ringCapacityFrames = FrameCount(8192), std::pmr::memory_resource* memRes = get_default_memory_resource());
*/

	if(file->isValid()) {
		modplayer = Elv::Util::pmr_make_shared<Hrk::ModuleStreamer>(std::pmr::get_default_resource(),
																	std::move(file));
		mixer->getChildren().emplace(modplayer.get(), 0.5f);
		modplayer->getState().setCursor(Hrk::FrameIndex(0));
		modplayer->getState().setPlayStatus(Hrk::PlayStatus::PLAYING);
	} else {
		SDL_Log("Could not open module file: %s", modpath.c_str());
	}
	// SDL audio devices start paused: without this the callback never runs,
	// so nothing ever reaches ModuleStreamer::decodeFrames().
	sdlDriver->pause(false);
}

void GameSystem::updateLogic()
{
}

void GameSystem::render()
{
}

void GameSystem::handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event)
{
}

void GameSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent& event)
{
}

void GameSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent& event)
{
}

void GameSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event)
{
}

void GameSystem::handleDollarGestureEvent(const SDL_DollarGestureEvent& event)
{
}

void GameSystem::handleDropEvent(const SDL_DropEvent& event)
{
}

void GameSystem::handleTouchFingerEvent(const SDL_TouchFingerEvent& event)
{
}

void GameSystem::handleKeyboardEvent(const SDL_KeyboardEvent& event)
{
	if(event.keysym.sym == SDLK_ESCAPE) SDL_Quit();
}

void GameSystem::handleJoyAxisEvent(const SDL_JoyAxisEvent& event)
{
}

void GameSystem::handleJoyBallEvent(const SDL_JoyBallEvent& event)
{
}

void GameSystem::handleJoyHatEvent(const SDL_JoyHatEvent& event)
{
}

void GameSystem::handleJoyButtonEvent(const SDL_JoyButtonEvent& event)
{
}

void GameSystem::handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event)
{
}

void GameSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent& event)
{
}

void GameSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
}

void GameSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent& event)
{
}

void GameSystem::handleMultiGestureEvent(const SDL_MultiGestureEvent& event)
{
}

void GameSystem::handleQuitEvent(const SDL_QuitEvent& event)
{
	SDL_Quit();
}

void GameSystem::handleTextEditingEvent(const SDL_TextEditingEvent& event)
{
}

void GameSystem::handleTextInputEvent(const SDL_TextInputEvent& event)
{
}

void GameSystem::handleUserEvent(const SDL_UserEvent& event)
{
}

void GameSystem::handleWindowEvent(const SDL_WindowEvent& event)
{
}