#include "AppSystem.hpp"

AppSystem::AppSystem(const std::string &title, int x, int y, int w, int h, uint32_t flags)
	: title(title), window(SDL_CreateWindow(this->title.c_str(),x,y,w,h,flags), SDL_DestroyWindow), latestTimestamp(0)
{
	SDL_VERSION(&syswmi.version);
	SDL_GetWindowWMInfo(window.get(),&syswmi);
}

AppSystem::AppSystem(std::string &&title, int x, int y, int w, int h, uint32_t flags)
	: title(std::move(title)), window(SDL_CreateWindow(this->title.c_str(),x,y,w,h,flags), SDL_DestroyWindow), latestTimestamp(0)
{
	SDL_VERSION(&syswmi.version);
	SDL_GetWindowWMInfo(window.get(),&syswmi);
}

void AppSystem::run()
{
	quit = false;
	while(!quit) {
		SDL_Event e;
		while( SDL_PollEvent(&e) ) {
			switch (e.type) {
			case SDL_APP_DIDENTERBACKGROUND: onDidEnterBackground(); break;
			case SDL_APP_DIDENTERFOREGROUND: onDidEnterForeground(); break;
			case SDL_APP_LOWMEMORY: onLowMemory(); break;
			case SDL_APP_TERMINATING: onTerminate(); break;
			case SDL_APP_WILLENTERBACKGROUND: onWillEnterBackground(); break;
			case SDL_APP_WILLENTERFOREGROUND: onWillEnterForeground(); break;
			case SDL_AUDIODEVICEADDED: handleAudioDeviceEvent(e.adevice); break;
			case SDL_AUDIODEVICEREMOVED: handleAudioDeviceEvent(e.adevice); break;
			case SDL_CLIPBOARDUPDATE: onClipboardUpdate(); break;
			case SDL_CONTROLLERAXISMOTION: handleControllerAxisEvent(e.caxis); break;
			case SDL_CONTROLLERBUTTONDOWN: handleControllerButtonEvent(e.cbutton); break;
			case SDL_CONTROLLERBUTTONUP:  handleControllerButtonEvent(e.cbutton); break;
			case SDL_CONTROLLERDEVICEADDED: handleControllerDeviceEvent(e.cdevice); break;
			case SDL_CONTROLLERDEVICEREMAPPED: handleControllerDeviceEvent(e.cdevice); break;
			case SDL_CONTROLLERDEVICEREMOVED: handleControllerDeviceEvent(e.cdevice); break;
			case SDL_CONTROLLERSENSORUPDATE: handleControllerSensorEvent(e.csensor); break;
			case SDL_CONTROLLERSTEAMHANDLEUPDATED: handleControllerDeviceEvent(e.cdevice); break;
			case SDL_CONTROLLERTOUCHPADDOWN: handleControllerTouchpadEvent(e.ctouchpad); break;
			case SDL_CONTROLLERTOUCHPADMOTION: handleControllerTouchpadEvent(e.ctouchpad); break;
			case SDL_CONTROLLERTOUCHPADUP: handleControllerTouchpadEvent(e.ctouchpad); break;
			case SDL_DISPLAYEVENT: handleDisplayEvent(e.display); break;
			case SDL_DOLLARGESTURE: handleDollarGestureEvent(e.dgesture); break;
			case SDL_DOLLARRECORD: handleDollarGestureEvent(e.dgesture); break;
			case SDL_DROPBEGIN: handleDropEvent(e.drop); break;
			case SDL_DROPCOMPLETE: handleDropEvent(e.drop); break;
			case SDL_DROPFILE: handleDropEvent(e.drop); break;
			case SDL_DROPTEXT: handleDropEvent(e.drop); break;
			case SDL_FINGERDOWN: handleTouchFingerEvent(e.tfinger); break;
			case SDL_FINGERMOTION: handleTouchFingerEvent(e.tfinger); break;
			case SDL_FINGERUP: handleTouchFingerEvent(e.tfinger); break;
			case SDL_JOYAXISMOTION: handleJoyAxisEvent(e.jaxis); break;
			case SDL_JOYBALLMOTION: handleJoyBallEvent(e.jball); break;
			case SDL_JOYBATTERYUPDATED: handleJoyBatteryEvent(e.jbattery); break;
			case SDL_JOYBUTTONDOWN: handleJoyButtonEvent(e.jbutton); break;
			case SDL_JOYBUTTONUP: handleJoyButtonEvent(e.jbutton); break;
			case SDL_JOYDEVICEADDED: handleJoyDeviceEvent(e.jdevice); break;
			case SDL_JOYDEVICEREMOVED: handleJoyDeviceEvent(e.jdevice); break;
			case SDL_JOYHATMOTION: handleJoyHatEvent(e.jhat); break;
			case SDL_KEYDOWN: handleKeyboardEvent(e.key); break;
			case SDL_KEYMAPCHANGED: onKeymapChanged(); break;
			case SDL_KEYUP: handleKeyboardEvent(e.key); break;
			case SDL_LOCALECHANGED: onLocaleChanged(); break;
			case SDL_MOUSEBUTTONDOWN: handleMouseButtonEvent(e.button); break;
			case SDL_MOUSEBUTTONUP: handleMouseButtonEvent(e.button); break;
			case SDL_MOUSEMOTION: handleMouseMotionEvent(e.motion); break;
			case SDL_MOUSEWHEEL: handleMouseWheelEvent(e.wheel); break;
			case SDL_MULTIGESTURE: handleMultiGestureEvent(e.mgesture); break;
			case SDL_QUIT: handleQuitEvent(e.quit); break;
			case SDL_SENSORUPDATE: handleSensorEvent(e.sensor); break;
			case SDL_SYSWMEVENT: handleSysWMEvent(e.syswm); break;
			case SDL_TEXTEDITING: handleTextEditingEvent(e.edit); break;
			case SDL_TEXTEDITING_EXT: handleTextEditingExtEvent(e.editExt); break;
			case SDL_TEXTINPUT: handleTextInputEvent(e.text); break;
			case SDL_WINDOWEVENT: handleWindowEvent(e.window); break;
			case SDL_USEREVENT: handleUserEvent(e.user); break;
			default: break;
			}
		}
		const uint32_t currentTimestamp = SDL_GetTicks();
		const float deltaTime = static_cast<float>(currentTimestamp - latestTimestamp) * 0.001f;
		latestTimestamp = currentTimestamp;
		update(deltaTime);
		render(deltaTime);
	}
}
