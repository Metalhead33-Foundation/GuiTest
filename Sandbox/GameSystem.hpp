#ifndef GAMESYSTEM_HPP
#define GAMESYSTEM_HPP
#include "AppSystem.hpp"
#include <Euphemy/Config/EuphConfiguration.hpp>
#include "HarukaContext.hpp"
#include <Haruka/Core/HrkMixer.hpp>
#include <Haruka/IO/HrkModuleStreamer.hpp>

class GameSystem : public AppSystem
{
private:
	Euph::Conf::Configuration& config;
	Hrk::sMixer mixer;
	Hrk::sModuleStreamer modplayer;
	std::unique_ptr<Haruka_SDL_Driver> sdlDriver;
public:
	GameSystem(Euph::Conf::Configuration& config);

	// AppSystem interface
protected:
	void initialize() override;
	void updateLogic() override;
	void render() override;
	void handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event) override;
	void handleControllerAxisEvent(const SDL_ControllerAxisEvent& event) override;
	void handleControllerButtonEvent(const SDL_ControllerButtonEvent& event) override;
	void handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event) override;
	void handleDollarGestureEvent(const SDL_DollarGestureEvent& event) override;
	void handleDropEvent(const SDL_DropEvent& event) override;
	void handleTouchFingerEvent(const SDL_TouchFingerEvent& event) override;
	void handleKeyboardEvent(const SDL_KeyboardEvent& event) override;
	void handleJoyAxisEvent(const SDL_JoyAxisEvent& event) override;
	void handleJoyBallEvent(const SDL_JoyBallEvent& event) override;
	void handleJoyHatEvent(const SDL_JoyHatEvent& event) override;
	void handleJoyButtonEvent(const SDL_JoyButtonEvent& event) override;
	void handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event) override;
	void handleMouseMotionEvent(const SDL_MouseMotionEvent& event) override;
	void handleMouseButtonEvent(const SDL_MouseButtonEvent& event) override;
	void handleMouseWheelEvent(const SDL_MouseWheelEvent& event) override;
	void handleMultiGestureEvent(const SDL_MultiGestureEvent& event) override;
	void handleQuitEvent(const SDL_QuitEvent& event) override;
	void handleTextEditingEvent(const SDL_TextEditingEvent& event) override;
	void handleTextInputEvent(const SDL_TextInputEvent& event) override;
	void handleUserEvent(const SDL_UserEvent& event) override;
	void handleWindowEvent(const SDL_WindowEvent& event) override;
};

#endif // GAMESYSTEM_HPP
