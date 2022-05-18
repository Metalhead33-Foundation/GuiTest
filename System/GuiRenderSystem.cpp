#include "GuiRenderSystem.hpp"
#include "../Widget/BoxWidget.hpp"
#include "../Widget/TickboxWidget.hpp"
#include "../Widget/TexturedWidget.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string_view>
#include "../Util/TextureHelpers.hpp"

void GuiRenderSystem::updateLogic()
{

}

void GuiRenderSystem::render()
{
	SDL_SetRenderDrawColor(renderer.get(),0,0,0,255);
	SDL_RenderClear(renderer.get());
	for(auto& it : widgets) {
		it->render(*renderer,viewport);
	}
	SDL_Rect mouseRect = { .x = 0, .y = 0,  .w = 6, .h = 6 };
	SDL_GetMouseState(&mouseRect.x,&mouseRect.y);
	SDL_SetRenderDrawColor(renderer.get(),255,255,255,255);
	SDL_RenderFillRect(renderer.get(),&mouseRect);
	SDL_RenderPresent(renderer.get());
	//SDL_Delay(16);
}

static const int CIRCLE_W = 128;
static const int CIRCLE_H = 128;

/*
	glm::mat4 projecetion;
	glm::vec2 sizeReciprocal;
	glm::ivec4 viewport;
*/

GuiRenderSystem::GuiRenderSystem(const std::string& title, int offsetX, int offsetY, int width, int height, Uint32 flags)
	: AppSystem(title,offsetX,offsetY,width,height,flags), renderer(SDL_CreateRenderer(this->window.get(),0,SDL_RENDERER_SOFTWARE),SDL_DestroyRenderer), currentWidget(nullptr),
	  projection(glm::ortho(0,width,0,height)),sizeReciprocal(glm::vec2(2.0/static_cast<float>(width),2.0/static_cast<float>(height))),viewport(glm::ivec4(0,0,width,height)),
	  strbuffer(""), fullscreen(false)
{
	SDL_ShowCursor(SDL_DISABLE);
	SDL_StartTextInput();
	widgets.reserve(256);
	widgets.push_back(std::make_shared<BoxWidget>(absToRel(glm::ivec2(100,100)),absToRel(glm::ivec2(150,150))));
	widgets.push_back(std::make_shared<TickboxWidget>(absToRel(glm::ivec2(200,200)),absToRel(glm::ivec2(300,300)) ));
	// TickboxWidget
	std::vector<uint32_t> tex1,tex2,tex3;
	createCircleTextures(tex1,tex2,tex3,CIRCLE_W,CIRCLE_H);
	widgets.push_back(std::make_shared<TexturedWidget>(absToRel(glm::ivec2(400,400)),absToRel(glm::ivec2(400+CIRCLE_W,400+CIRCLE_H)),*renderer,CIRCLE_W,CIRCLE_H,tex1.data(),tex2.data(),tex3.data()));

}

glm::vec2 GuiRenderSystem::absToRel(const glm::ivec2& abs) const
{
	return glm::vec2(
				((static_cast<float>(abs.x) / static_cast<float>(viewport.z)) * 2.0f) - 1.0f,
				((static_cast<float>(abs.y) / static_cast<float>(viewport.w)) * 2.0f) - 1.0f
				);
}

void GuiRenderSystem::handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleDollarGestureEvent(const SDL_DollarGestureEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleDropEvent(const SDL_DropEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleTouchFingerEvent(const SDL_TouchFingerEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleJoyAxisEvent(const SDL_JoyAxisEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleJoyBallEvent(const SDL_JoyBallEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleJoyHatEvent(const SDL_JoyHatEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleJoyButtonEvent(const SDL_JoyButtonEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent& event)
{
	IWidget* pcwidg = currentWidget;
	IWidget* cwidg = nullptr;
	const auto pos = glm::vec2( static_cast<float>(event.x), static_cast<float>(event.y) ) * sizeReciprocal - glm::vec2(1.0f,1.0f);
	const auto relpos = glm::vec2( static_cast<float>(event.xrel), static_cast<float>(event.yrel) ) * sizeReciprocal;
	for(auto& it : widgets) {
		const auto& tl = it->getTopLeft();
		const auto& br = it->getBottomRight();
		if( (pos.x >= tl.x && pos.x <= br.x) && (pos.y >= tl.y && pos.y <= br.y) ) {
			if(it->onHover(IWidget::getRelativePosFromAbs(tl,br,pos),relpos)) cwidg = it.get();
			break;
		}
	}
	if(pcwidg && pcwidg != cwidg) {
		pcwidg->onOutOfFocus();
	}
	currentWidget = cwidg;
}

void GuiRenderSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
	const auto pos = glm::vec2( static_cast<float>(event.x), static_cast<float>(event.y) ) * sizeReciprocal - glm::vec2(1.0f,1.0f);
	if(currentWidget) {
		const auto& tl = currentWidget->getTopLeft();
		const auto& br = currentWidget->getBottomRight();
		currentWidget->onClick(IWidget::getRelativePosFromAbs(tl,br,pos),event.button,event.state,event.clicks);
	}
}

void GuiRenderSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleMultiGestureEvent(const SDL_MultiGestureEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleQuitEvent(const SDL_QuitEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleTextEditingEvent(const SDL_TextEditingEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleKeyboardEvent(const SDL_KeyboardEvent& event)
{
	//(void)event;
	if(event.type == SDL_KEYDOWN) {
		switch (event.keysym.sym) {
		case SDLK_ESCAPE: SDL_Quit(); break;
		case SDLK_BACKSPACE: {
			if(strbuffer.length()) strbuffer = strbuffer.substr(0, strbuffer.length() - 1);
			break;
		}
		case SDLK_RETURN: {
			std::cout << strbuffer << std::endl;
			break;
		}
		case SDLK_F1: {
			if(fullscreen) SDL_SetWindowFullscreen(window.get(),0);
			else {
				SDL_SetWindowFullscreen(window.get(),SDL_WINDOW_FULLSCREEN);
				//SDL_RenderSetLogicalSize(renderer.get(),viewport.z,viewport.z);
				fullscreen = true;
			}
			break;
		}
		default: break;
		}
	}
}

void GuiRenderSystem::handleTextInputEvent(const SDL_TextInputEvent& event)
{
	//(void)event;
	strbuffer += event.text;
	//std::cout << std::string(event.text,32) << std::endl;
}

void GuiRenderSystem::handleUserEvent(const SDL_UserEvent& event)
{
	(void)event;
}

void GuiRenderSystem::handleWindowEvent(const SDL_WindowEvent& event)
{
	(void)event;
}
