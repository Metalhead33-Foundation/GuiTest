#ifndef APPSYSTEM_HPP
#define APPSYSTEM_HPP
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <memory>
#include <string>

typedef std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> uWindow;

class AppSystem
{
public:
	const std::string title;
protected:
	uWindow window;
	SDL_SysWMinfo syswmi;
	uint32_t latestTimestamp;
	volatile bool quit;
public:
	virtual ~AppSystem() = default;
	AppSystem(const std::string& title, int x, int y, int w, int h, uint32_t flags);
	AppSystem(std::string&& title, int x, int y, int w, int h, uint32_t flags);
	void run();
	virtual void render(float deltaTime) = 0;
	virtual void update(float deltaTime) = 0;
	virtual void handleDisplayEvent(const SDL_DisplayEvent& ev) = 0;
	virtual void handleWindowEvent(const SDL_WindowEvent& ev) = 0;
	virtual void handleKeyboardEvent(const SDL_KeyboardEvent& ev) = 0;
	virtual void handleTextEditingEvent(const SDL_TextEditingEvent& ev) = 0;
	virtual void handleTextEditingExtEvent(const SDL_TextEditingExtEvent& ev) = 0;
	virtual void handleTextInputEvent(const SDL_TextInputEvent& ev) = 0;
	virtual void handleMouseMotionEvent(const SDL_MouseMotionEvent& ev) = 0;
	virtual void handleMouseButtonEvent(const SDL_MouseButtonEvent& ev) = 0;
	virtual void handleMouseWheelEvent(const SDL_MouseWheelEvent& ev) = 0;
	virtual void handleJoyAxisEvent(const SDL_JoyAxisEvent& ev) = 0;
	virtual void handleJoyBallEvent(const SDL_JoyBallEvent& ev) = 0;
	virtual void handleJoyHatEvent(const SDL_JoyHatEvent& ev) = 0;
	virtual void handleJoyButtonEvent(const SDL_JoyButtonEvent& ev) = 0;
	virtual void handleJoyDeviceEvent(const SDL_JoyDeviceEvent& ev) = 0;
	virtual void handleJoyBatteryEvent(const SDL_JoyBatteryEvent& ev) = 0;
	virtual void handleControllerAxisEvent(const SDL_ControllerAxisEvent& ev) = 0;
	virtual void handleControllerButtonEvent(const SDL_ControllerButtonEvent& ev) = 0;
	virtual void handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& ev) = 0;
	virtual void handleControllerTouchpadEvent(const SDL_ControllerTouchpadEvent& ev) = 0;
	virtual void handleControllerSensorEvent(const SDL_ControllerSensorEvent& ev) = 0;
	virtual void handleAudioDeviceEvent(const SDL_AudioDeviceEvent& ev) = 0;
	virtual void handleSensorEvent(const SDL_SensorEvent& ev) = 0;
	virtual void handleQuitEvent(const SDL_QuitEvent& ev) = 0;
	virtual void handleUserEvent(const SDL_UserEvent& ev) = 0;
	virtual void handleSysWMEvent(const SDL_SysWMEvent& ev) = 0;
	virtual void handleTouchFingerEvent(const SDL_TouchFingerEvent& ev) = 0;
	virtual void handleMultiGestureEvent(const SDL_MultiGestureEvent& ev) = 0;
	virtual void handleDollarGestureEvent(const SDL_DollarGestureEvent& ev) = 0;
	virtual void handleDropEvent(const SDL_DropEvent& ev) = 0;
	virtual void onLowMemory() = 0;
	virtual void onDidEnterBackground() = 0;
	virtual void onDidEnterForeground() = 0;
	virtual void onWillEnterBackground() = 0;
	virtual void onWillEnterForeground() = 0;
	virtual void onTerminate() = 0;
	virtual void onLocaleChanged() = 0;
	virtual void onClipboardUpdate() = 0;
	virtual void onKeymapChanged() = 0;
};

#endif // APPSYSTEM_HPP
