#include "GameSystem.hpp"
#include <Euphemy/Io/EuphFile.hpp>
#include <cstdint>
#include <cstdlib>

namespace {

/** @brief Expands a packed `0xAARRGGBB` colour into four linear RGBA components. */
void expandColor(std::uint32_t packed, float (&out)[4])
{
	out[0] = float((packed >> 16) & 0xFFu) / 255.0f;
	out[1] = float((packed >> 8) & 0xFFu) / 255.0f;
	out[2] = float(packed & 0xFFu) / 255.0f;
	out[3] = float((packed >> 24) & 0xFFu) / 255.0f;
}

} // namespace

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
	mixer = Elv::Util::pmr_make_shared<Hrk::Mixer>(std::pmr::get_default_resource(),
		Hrk::ChannelCount(channels), Hrk::InterleavingType::INTERLEAVED, Hrk::SampleRate(samplerate), Hrk::FrameCount(bufferSize)
		);
	sdlDriver->setPlayable(mixer);
	// Graphics: the Kaldi backend is a shared library loaded at run time, so the
	// configuration picks which one and which colour the window is cleared to.
	KaldiContext::Settings graphicsSettings;
	graphicsSettings.backendLibrary =
		std::string(config.getString("Graphics", "sBackendLibrary", "libKaldiVK.so"));
	// The configuration parses numbers in base 10, so the packed colour travels as a
	// string: "0xAARRGGBB" or "AARRGGBB", whichever reads better in the ini file.
	const std::string clearColorText { config.getString("Graphics", "sClearColor", "0xFF102030") };
	const std::uint32_t packedColor =
		static_cast<std::uint32_t>(std::strtoul(clearColorText.c_str(), nullptr, 16));
	expandColor(packedColor, clearColor);
	graphicsSettings.clearColor[0] = clearColor[0];
	graphicsSettings.clearColor[1] = clearColor[1];
	graphicsSettings.clearColor[2] = clearColor[2];
	graphicsSettings.clearColor[3] = clearColor[3];
	try {
		kaldi = std::make_unique<KaldiContext>(windowInfo, width, height, graphicsSettings);
		SDL_Log("Kaldi HAL %s via %s (%s), clearing to 0x%08X every frame",
			kaldi->halVersionName().c_str(), kaldi->rendererName().data(),
			kaldi->backendVersion().data(), static_cast<unsigned>(packedColor));
	} catch (const std::exception& error) {
		// Rendering is optional here: without a usable GPU the audio half still runs
		// and `render()` becomes a no-op, which keeps the playground usable.
		kaldi.reset();
		SDL_Log("Graphics disabled: %s", error.what());
	}
}

void GameSystem::initialize()
{
	std::string modpath = std::string(config.getString("Game", "sSongPath", "/home/legacy/zene/GameMusic/Unreal Tournament/mod/Magnus.it"));
	Euph::Io::uFile file = Elv::Util::pmr_make_unique<Euph::Io::File>(std::pmr::get_default_resource(), modpath.c_str(), Elv::Io::Mode::READ);
	if(file->isValid()) {
		modplayer = Elv::Util::pmr_make_shared<Hrk::ModuleStreamer>(std::pmr::get_default_resource(),
																	std::move(file));
		mixer->getChildren().emplace(modplayer.get(), config.getFloat("Game","fModVolume",0.10F));
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
	if (kaldi == nullptr) {
		return;
	}
	const Kld::HalStatus status = kaldi->clear(clearColor);
	if (!status.ok() && !clearFailureLogged) {
		// `status.message` is backend storage: log it now instead of keeping it.
		clearFailureLogged = true;
		SDL_Log("Kaldi clear failed (%d): %s", static_cast<int>(status.code),
			status.message != nullptr ? status.message : "(no message)");
	}
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