#ifndef TESTSYSTEM_HPP
#define TESTSYSTEM_HPP
#include "AppSystem.hpp"
#include "IniConfiguration.hpp"
#include <GFX/Abstract/GfxResourceFactory.hpp>
#include <MhLib/IoSys/MhIoSystem.hpp>

class TestSystem : public AppSystem
{
public:
	typedef std::function<MH33::GFX::pResourceFactory(const SDL_SysWMinfo&)> ResourceFactoryCreator;
private:
	MH33::Io::sSystem iosys;
	MH33::GFX::uUnindexedVertexBuffer triangleVbo;
	MH33::GFX::uIndexedVertexBuffer screenQuad;
	MH33::GFX::uPipeline trianglePipeline;
	MH33::GFX::uPipeline screenPipeline;
	MH33::GFX::uResourceFactory gfx;
	MH33::GFX::uTexture2D tex1;
	MH33::GFX::uTexture2D tex2;
public:
	TestSystem(const MH33::Io::sSystem& iosys, const ResourceFactoryCreator& gfxCreator, IniConfiguration& conf);
	~TestSystem();
	void render(float deltaTime) override;
	void update(float deltaTime) override;
	void handleDisplayEvent(const SDL_DisplayEvent &ev) override;
	void handleWindowEvent(const SDL_WindowEvent &ev) override;
	void handleKeyboardEvent(const SDL_KeyboardEvent &ev) override;
	void handleTextEditingEvent(const SDL_TextEditingEvent &ev) override;
	void handleTextEditingExtEvent(const SDL_TextEditingExtEvent &ev) override;
	void handleTextInputEvent(const SDL_TextInputEvent &ev) override;
	void handleMouseMotionEvent(const SDL_MouseMotionEvent &ev) override;
	void handleMouseButtonEvent(const SDL_MouseButtonEvent &ev) override;
	void handleMouseWheelEvent(const SDL_MouseWheelEvent &ev) override;
	void handleJoyAxisEvent(const SDL_JoyAxisEvent &ev) override;
	void handleJoyBallEvent(const SDL_JoyBallEvent &ev) override;
	void handleJoyHatEvent(const SDL_JoyHatEvent &ev) override;
	void handleJoyButtonEvent(const SDL_JoyButtonEvent &ev) override;
	void handleJoyDeviceEvent(const SDL_JoyDeviceEvent &ev) override;
	void handleJoyBatteryEvent(const SDL_JoyBatteryEvent &ev) override;
	void handleControllerAxisEvent(const SDL_ControllerAxisEvent &ev) override;
	void handleControllerButtonEvent(const SDL_ControllerButtonEvent &ev) override;
	void handleControllerDeviceEvent(const SDL_ControllerDeviceEvent &ev) override;
	void handleControllerTouchpadEvent(const SDL_ControllerTouchpadEvent &ev) override;
	void handleControllerSensorEvent(const SDL_ControllerSensorEvent &ev) override;
	void handleAudioDeviceEvent(const SDL_AudioDeviceEvent &ev) override;
	void handleSensorEvent(const SDL_SensorEvent &ev) override;
	void handleQuitEvent(const SDL_QuitEvent &ev) override;
	void handleUserEvent(const SDL_UserEvent &ev) override;
	void handleSysWMEvent(const SDL_SysWMEvent &ev) override;
	void handleTouchFingerEvent(const SDL_TouchFingerEvent &ev) override;
	void handleMultiGestureEvent(const SDL_MultiGestureEvent &ev) override;
	void handleDollarGestureEvent(const SDL_DollarGestureEvent &ev) override;
	void handleDropEvent(const SDL_DropEvent &ev) override;
	void onLowMemory() override;
	void onDidEnterBackground() override;
	void onDidEnterForeground() override;
	void onWillEnterBackground() override;
	void onWillEnterForeground() override;
	void onTerminate() override;
	void onLocaleChanged() override;
	void onClipboardUpdate() override;
	void onKeymapChanged() override;
};

#endif // TESTSYSTEM_HPP
