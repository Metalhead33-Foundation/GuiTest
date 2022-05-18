#ifndef APPSYSTEM_H
#define APPSYSTEM_H
#include <memory>
#include <string>
#include <SDL2/SDL.h>

class AppSystem
{
public:
	typedef std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> uWindow;
	typedef std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> uSdlTexture;
	typedef std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)> uSdlRenderer;
private:
	AppSystem(const AppSystem& cpy) = delete; // Disable copy constructor
	AppSystem& operator=(const AppSystem& cpy) = delete; // Disable copy assignment operator
protected:
	uWindow window;
	virtual void updateLogic() = 0;
	virtual void render() = 0;
	// SDL event handlers
	virtual void handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event) = 0;
	virtual void handleControllerAxisEvent(const SDL_ControllerAxisEvent& event) = 0;
	virtual void handleControllerButtonEvent(const SDL_ControllerButtonEvent& event) = 0;
	virtual void handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event) = 0;
	virtual void handleDollarGestureEvent(const SDL_DollarGestureEvent& event) = 0;
	virtual void handleDropEvent(const SDL_DropEvent& event) = 0;
	virtual void handleTouchFingerEvent(const SDL_TouchFingerEvent& event) = 0;
	virtual void handleKeyboardEvent(const SDL_KeyboardEvent& event) = 0;
	virtual void handleJoyAxisEvent(const SDL_JoyAxisEvent& event) = 0;
	virtual void handleJoyBallEvent(const SDL_JoyBallEvent& event) = 0;
	virtual void handleJoyHatEvent(const SDL_JoyHatEvent& event) = 0;
	virtual void handleJoyButtonEvent(const SDL_JoyButtonEvent& event) = 0;
	virtual void handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event) = 0;
	virtual void handleMouseMotionEvent(const SDL_MouseMotionEvent& event) = 0;
	virtual void handleMouseButtonEvent(const SDL_MouseButtonEvent& event) = 0;
	virtual void handleMouseWheelEvent(const SDL_MouseWheelEvent& event) = 0;
	virtual void handleMultiGestureEvent(const SDL_MultiGestureEvent& event) = 0;
	virtual void handleQuitEvent(const SDL_QuitEvent& event) = 0;
	virtual void handleTextEditingEvent(const SDL_TextEditingEvent& event) = 0;
	virtual void handleTextInputEvent(const SDL_TextInputEvent& event) = 0;
	virtual void handleUserEvent(const SDL_UserEvent& event) = 0;
	virtual void handleWindowEvent(const SDL_WindowEvent& event) = 0;
public:
	// Regular constructors
	virtual ~AppSystem() = default;
	AppSystem(const char *title, int offsetX, int offsetY, int width, int height, Uint32 flags);
	AppSystem(const std::string& title, int offsetX, int offsetY, int width, int height, Uint32 flags);
	void run();
};

#endif // APPSYSTEM_H
