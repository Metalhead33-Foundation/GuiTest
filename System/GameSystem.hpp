#ifndef GAMESYSTEM_HPP
#define GAMESYSTEM_HPP
#include <System/AppSystem.hpp>
#include <MhLib/Media/GFX/MhGFXResourceFactory.hpp>
#include <System/AudioSDL.hpp>
#include <System/SystemConfiguration.hpp>
#include <Text/FontRepository.hpp>
#include <Text/RichTextProcessor.hpp>
#include <Widget/Cursor.hpp>
#include <Widget/IWidget.hpp>
#include <Renderer/Shared/GuiRenderer.hpp>
#include <MhLib/Util/ThreadsafeContainer.hpp>

typedef MH33::GFX::ResourceFactory* (*RESOURCE_FACTORY_CREATOR)(SDL_Window*,SDL_SysWMinfo*);
typedef void (*RESOURCE_FACTORY_DESTROYER)(MH33::GFX::ResourceFactory*);
// SDL_Window* window, SDL_SysWMinfo* winfo

namespace SYS {

class GameSystem : public AppSystem
{
private:
	int logicTicks;
	MH33::GFX::sResourceFactory resourceFactory;
	Driver::SDL sdlAudio;
	TXT::uTextRenderingContext textContext;
	MH33::GFX::uGuiRenderingContext guiContext;
	TXT::sFontRepository fontRepository;
	TXT::sRichTextProcessor richTextProcessor;
	std::vector<TXT::TextBlockUtf32> textToRender;
	sCursor cursor;
	glm::fvec2 mousePos;
	threadsafe<std::vector<sWidget>> widgets;
	IWidget* currentWidget;
public:
	GameSystem(RESOURCE_FACTORY_CREATOR rendererCreator, RESOURCE_FACTORY_DESTROYER rendererDestroyer, const ConfigurationSection& rendererSettings, const ConfigurationSection& audioSettings);
	const Driver::SDL& getSdlAudio() const;
	Driver::SDL& getSdlAudio();
	TXT::FontRepository getFontRepository();

	// AppSystem interface
	const TXT::sRichTextProcessor& getRichTextProcessor() const;

	const sCursor& getCursor() const;
	void setCursor(const sCursor& newCursor);

	const MH33::GFX::sResourceFactory& getResourceFactory() const;
	const threadsafe<std::vector<sWidget> >& getWidgets() const;
	threadsafe<std::vector<sWidget> >& getWidgets();
protected:
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
}
#endif // GAMESYSTEM_HPP
