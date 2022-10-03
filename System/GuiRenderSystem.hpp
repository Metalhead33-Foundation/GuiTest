#ifndef GUIRENDERSYSTEM_H
#define GUIRENDERSYSTEM_H
#include "AppSystem.hpp"
#include <Texture/Texture.hpp>
#include <SoftPipeline/ZBuffer.hpp>
#include <Widget/IWidget.hpp>
#include <Pipeline/GuiRenderer.hpp>
#include <SoftPipeline/BasicPipeline.hpp>
#include <SoftPipeline/TexturedPipeline.hpp>
#include <SoftPipeline/ColouredTexturedPipeline.hpp>
#include <MhLib/Util/ThreadsafeContainer.hpp>
#include <Widget/Cursor.hpp>
#include <Text/FontRepository.hpp>
#include "FpsCounter.hpp"
#include <vector>
#include "../Text/RichTextProcessor.hpp"

namespace SYS {
class GuiRenderSystem : public AppSystem, public GuiRenderer
{
public:
	typedef std::function<void(GuiRenderSystem*)> KeyFunction;
	typedef std::map<SDL_Keycode,KeyFunction> FunctionMap;
protected:
	sTexture framebuffer;
	std::shared_ptr<SoftwareRenderer::ZBuffer> zbuffer;
	SoftwareRenderer::BasicPipeline bpipeline;
	SoftwareRenderer::TexturedPipeline tpipeline;
	SoftwareRenderer::ColouredTexturedPipeline ctpipeline;
	sCursor cursor;
	TXT::sFontRepository font;
	IWidget* currentWidget;
	glm::mat4 projection;
	glm::fvec2 sizeReciprocal;
	glm::ivec4 viewport;
	threadsafe<std::vector<sWidget>> widgets;
	std::string strbuffer;
	glm::fvec2 mousePos;
	bool fullscreen;
	FpsCounter fpsCounter;
	FunctionMap functionMap;
	TXT::sRichTextProcessor richie;
	std::vector<TXT::TextBlockUtf32> textToRender;
	int logicTicks;
	virtual void updateLogic() override;
	virtual void render() override;
	void onResolutionChange(int newWidth, int newHeight);
public:
	GuiRenderSystem(const std::string& title, int offsetX, int offsetY, int width, int height, Uint32 flags);
	glm::fvec2 absToRel(const glm::ivec2& abs) const;
	glm::fvec2 absToRel(const glm::ivec2& abs, const glm::ivec2& customRes) const;
	glm::ivec2 relToAbs(const glm::fvec2& rel) const;

	// AppSystem interface
protected:
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
public:
	void renderCLine(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour, int thickness = 1) override;
	void renderCRect(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour) override;
	void renderCTriang(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2& p2, const glm::fvec4& colour) override;
	void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const ITexture& tex) override;
	void renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const ITexture& tex) override;
	void renderTex(const ITexture& tex) override;
	void renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const ITexture& tex) override;
	void renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::vec4& colour, const ITexture& tex) override;
	void renderCTex(const glm::vec4& colour, const ITexture& tex) override;
	const sCursor& getCursor() const;
	void setCursor(const sCursor& newCursor);
	void setCursor(sCursor&& newCursor);
	const TXT::sFontRepository& getFont() const;
	void setFont(const TXT::sFontRepository& newFont);
	void setFont(TXT::sFontRepository&& newFont);
	const FunctionMap& getFunctionMap() const;
	FunctionMap& getFunctionMap();
	void renderTiltedCTex(float tilt, const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const ITexture& tex) override;
	const threadsafe<std::vector<sWidget> >& getWidgets() const;
	threadsafe<std::vector<sWidget> >& getWidgets();
};
}

#endif // GUIRENDERSYSTEM_H
