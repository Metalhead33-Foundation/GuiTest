#include "TestSystem.hpp"

TestSystem::TestSystem(IniConfiguration &conf)
	: AppSystem(
		conf["Main"].getValueOrDefault("sTitle","Default Window")->second.toString(),
		0, 0,
		conf["Video"].getValueOrDefault("iWidth","640")->second.value.i_int,
		conf["Video"].getValueOrDefault("iHeight","640")->second.value.i_int,
		(conf["Video"].getValueOrDefault("bFullscreen","0")->second.value.i_bool) ? SDL_WINDOW_FULLSCREEN : 0
	)
{

}


void TestSystem::render(float deltaTime)
{
	//Get window surface
	auto screenSurface = SDL_GetWindowSurface( window.get() );

	//Fill the surface white
	SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0xFF, 0xFF, 0xFF ) );

	//Update the surface
	SDL_UpdateWindowSurface( window.get() );
}

void TestSystem::update(float deltaTime)
{
}

void TestSystem::handleDisplayEvent(const SDL_DisplayEvent &ev)
{
}

void TestSystem::handleWindowEvent(const SDL_WindowEvent &ev)
{
	switch (ev.type) {
		case SDL_WINDOWEVENT_CLOSE:
		this->quit = true; break;
	default: break;
	}
}

void TestSystem::handleKeyboardEvent(const SDL_KeyboardEvent &ev)
{
}

void TestSystem::handleTextEditingEvent(const SDL_TextEditingEvent &ev)
{
}

void TestSystem::handleTextEditingExtEvent(const SDL_TextEditingExtEvent &ev)
{
}

void TestSystem::handleTextInputEvent(const SDL_TextInputEvent &ev)
{
}

void TestSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent &ev)
{
}

void TestSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent &ev)
{
}

void TestSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent &ev)
{
}

void TestSystem::handleJoyAxisEvent(const SDL_JoyAxisEvent &ev)
{
}

void TestSystem::handleJoyBallEvent(const SDL_JoyBallEvent &ev)
{
}

void TestSystem::handleJoyHatEvent(const SDL_JoyHatEvent &ev)
{
}

void TestSystem::handleJoyButtonEvent(const SDL_JoyButtonEvent &ev)
{
}

void TestSystem::handleJoyDeviceEvent(const SDL_JoyDeviceEvent &ev)
{
}

void TestSystem::handleJoyBatteryEvent(const SDL_JoyBatteryEvent &ev)
{
}

void TestSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent &ev)
{
}

void TestSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent &ev)
{
}

void TestSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent &ev)
{
}

void TestSystem::handleControllerTouchpadEvent(const SDL_ControllerTouchpadEvent &ev)
{
}

void TestSystem::handleControllerSensorEvent(const SDL_ControllerSensorEvent &ev)
{
}

void TestSystem::handleAudioDeviceEvent(const SDL_AudioDeviceEvent &ev)
{
}

void TestSystem::handleSensorEvent(const SDL_SensorEvent &ev)
{
}

void TestSystem::handleQuitEvent(const SDL_QuitEvent &ev)
{
	this->quit = true;
}

void TestSystem::handleUserEvent(const SDL_UserEvent &ev)
{
}

void TestSystem::handleSysWMEvent(const SDL_SysWMEvent &ev)
{
}

void TestSystem::handleTouchFingerEvent(const SDL_TouchFingerEvent &ev)
{
}

void TestSystem::handleMultiGestureEvent(const SDL_MultiGestureEvent &ev)
{
}

void TestSystem::handleDollarGestureEvent(const SDL_DollarGestureEvent &ev)
{
}

void TestSystem::handleDropEvent(const SDL_DropEvent &ev)
{
}

void TestSystem::onLowMemory()
{
	quit = true;
}

void TestSystem::onDidEnterBackground()
{
}

void TestSystem::onDidEnterForeground()
{
}

void TestSystem::onWillEnterBackground()
{
}

void TestSystem::onWillEnterForeground()
{
}

void TestSystem::onTerminate()
{
	quit = true;
}

void TestSystem::onLocaleChanged()
{
}

void TestSystem::onClipboardUpdate()
{
}

void TestSystem::onKeymapChanged()
{
}
