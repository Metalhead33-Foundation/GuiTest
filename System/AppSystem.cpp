#include "AppSystem.hpp"

namespace SYS {
AppSystem::AppSystem(const char *title, int offsetX, int offsetY, int width, int height, Uint32 flags)
	: window(SDL_CreateWindow(title,offsetX,offsetY,width,height,flags),SDL_DestroyWindow), width(width), height(height), sizeReciprocal(2.0f/static_cast<float>(width),2.0f/static_cast<float>(height))
{
	SDL_GetWindowWMInfo(window.get(),&windowInfo);
}

AppSystem::AppSystem(const std::string &title, int offsetX, int offsetY, int width, int height, Uint32 flags)
	: window(SDL_CreateWindow(title.c_str(),offsetX,offsetY,width,height,flags),SDL_DestroyWindow), width(width), height(height), sizeReciprocal(2.0f/static_cast<float>(width),2.0f/static_cast<float>(height))
{
	SDL_GetWindowWMInfo(window.get(),&windowInfo);
}

void AppSystem::run()
{
	bool isInterrupted=false;
	do {
		SDL_Event ev;
		while(SDL_PollEvent(&ev)) {
			switch(ev.type) {
			case SDL_QUIT: { isInterrupted = true; break; }
			case SDL_AUDIODEVICEADDED: handleAudioDeviceEvent(ev.adevice); break;
			case SDL_AUDIODEVICEREMOVED: handleAudioDeviceEvent(ev.adevice); break;
			case SDL_CONTROLLERAXISMOTION: handleControllerAxisEvent(ev.caxis); break;
			case SDL_CONTROLLERBUTTONDOWN: handleControllerButtonEvent(ev.cbutton); break;
			case SDL_CONTROLLERBUTTONUP: handleControllerButtonEvent(ev.cbutton); break;
			case SDL_CONTROLLERDEVICEADDED: handleControllerDeviceEvent(ev.cdevice); break;
			case SDL_CONTROLLERDEVICEREMOVED: handleControllerDeviceEvent(ev.cdevice); break;
			case SDL_CONTROLLERDEVICEREMAPPED: handleControllerDeviceEvent(ev.cdevice); break;
			case SDL_DOLLARGESTURE: handleDollarGestureEvent(ev.dgesture); break;
			case SDL_DOLLARRECORD: handleDollarGestureEvent(ev.dgesture); break;
			case SDL_DROPFILE: handleDropEvent(ev.drop); break;
			case SDL_DROPTEXT: handleDropEvent(ev.drop); break;
			case SDL_DROPBEGIN: handleDropEvent(ev.drop); break;
			case SDL_DROPCOMPLETE: handleDropEvent(ev.drop); break;
			case SDL_FINGERMOTION: handleTouchFingerEvent(ev.tfinger); break;
			case SDL_FINGERDOWN:handleTouchFingerEvent(ev.tfinger);  break;
			case SDL_FINGERUP: handleTouchFingerEvent(ev.tfinger); break;
			case SDL_KEYDOWN: handleKeyboardEvent(ev.key); break;
			case SDL_KEYUP: handleKeyboardEvent(ev.key); break;
			case SDL_JOYAXISMOTION: handleJoyAxisEvent(ev.jaxis); break;
			case SDL_JOYBALLMOTION: handleJoyBallEvent(ev.jball); break;
			case SDL_JOYHATMOTION: handleJoyHatEvent(ev.jhat); break;
			case SDL_JOYBUTTONDOWN: handleJoyButtonEvent(ev.jbutton); break;
			case SDL_JOYBUTTONUP: handleJoyButtonEvent(ev.jbutton); break;
			case SDL_JOYDEVICEADDED: handleJoyDeviceEvent(ev.jdevice); break;
			case SDL_JOYDEVICEREMOVED: handleJoyDeviceEvent(ev.jdevice); break;
			case SDL_MOUSEMOTION: handleMouseMotionEvent(ev.motion); break;
			case SDL_MOUSEBUTTONDOWN: handleMouseButtonEvent(ev.button); break;
			case SDL_MOUSEBUTTONUP: handleMouseButtonEvent(ev.button); break;
			case SDL_MOUSEWHEEL: handleMouseWheelEvent(ev.wheel); break;
			case SDL_MULTIGESTURE: handleMultiGestureEvent(ev.mgesture); break;
			case SDL_TEXTEDITING: handleTextEditingEvent(ev.edit); break;
			case SDL_TEXTINPUT: handleTextInputEvent(ev.text); break;
			case SDL_USEREVENT: handleUserEvent(ev.user); break;
			case SDL_WINDOWEVENT: handleWindowEvent(ev.window); break;
			default: break;
			}
		}
		updateLogic();
		render();
	} while(!isInterrupted);
}
}
