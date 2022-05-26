#ifndef GUIRENDERSYSTEM_H
#define GUIRENDERSYSTEM_H
#include "AppSystem.hpp"
#include "../Texture/Texture.hpp"
#include "../Pipeline/ZBuffer.hpp"
#include "../Widget/IWidget.hpp"
#include "../Pipeline/GuiRenderer.hpp"
#include "../Pipeline/BasicPipeline.hpp"
#include "../Pipeline/TexturedPipeline.hpp"
#include "../Util/ThreadsafeContainer.hpp"
#include <vector>

class GuiRenderSystem : public AppSystem, public GuiRenderer
{
protected:
	std::shared_ptr<Texture> framebuffer;
	std::shared_ptr<ZBuffer> zbuffer;
	BasicPipeline bpipeline;
	TexturedPipeline tpipeline;

	IWidget* currentWidget;
	glm::mat4 projection;
	glm::vec2 sizeReciprocal;
	glm::ivec4 viewport;
	threadsafe<std::vector<sWidget>> widgets;
	std::string strbuffer;
	glm::vec2 mousePos;
	bool fullscreen;
	float fpsMin;
	float fpsMax;
	virtual void updateLogic() override;
	virtual void render() override;
	void onResolutionChange(int newWidth, int newHeight);
public:
	GuiRenderSystem(const std::string& title, int offsetX, int offsetY, int width, int height, Uint32 flags);
	glm::vec2 absToRel(const glm::ivec2& abs) const;
	glm::vec2 absToRel(const glm::ivec2& abs, const glm::ivec2& customRes) const;
	glm::ivec2 relToAbs(const glm::vec2& rel) const;

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

	// GuiRenderer interface
public:
	void renderCLine(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& colour, int thickness = 1) override;
	void renderCRect(const glm::vec2& p0, const glm::vec2& p1, const glm::vec4& colour) override;
	void renderCTriang(const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& colour) override;
	void renderTex(const glm::vec2& p0, const glm::vec2& p1, const std::shared_ptr<Texture> tex) override;
	void renderTex(const std::shared_ptr<Texture> tex) override;
};

#endif // GUIRENDERSYSTEM_H
