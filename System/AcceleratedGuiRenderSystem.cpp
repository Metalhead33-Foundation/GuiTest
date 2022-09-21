#include "AcceleratedGuiRenderSystem.hpp"
#include "../Widget/BoxWidget.hpp"
#include "../Widget/TickboxWidget.hpp"
#include "../Widget/TexturedWidget.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string_view>
#include "../Util/TextureFromSurface.hpp"
#include "../Text/MmlParser.hpp"
#include <sstream>

namespace SYS {
const sCursor& AcceleratedGuiRenderSystem::getCursor() const
{
	return cursor;
}

void AcceleratedGuiRenderSystem::setCursor(const sCursor& newCursor)
{
	cursor = newCursor;
}

void AcceleratedGuiRenderSystem::setCursor(sCursor&& newCursor)
{
	cursor = std::move(newCursor);
}

const TXT::sFontRepository& AcceleratedGuiRenderSystem::getFont() const
{
	return font;
}

void AcceleratedGuiRenderSystem::setFont(const TXT::sFontRepository& newFont)
{
	font = newFont;
	richie = std::make_shared<TXT::RichTextProcessor>(std::ref(font));
	//richie = sRichTextProcessor(new RichTextProcessor(font));
}

void AcceleratedGuiRenderSystem::setFont(TXT::sFontRepository&& newFont)
{
	font = std::move(newFont);
	richie = std::make_shared<TXT::RichTextProcessor>(std::ref(font));
	//richie = sRichTextProcessor(new RichTextProcessor(font));
}

static std::string txt = "Multi-line\ntext";

const AcceleratedGuiRenderSystem::FunctionMap& AcceleratedGuiRenderSystem::getFunctionMap() const
{
	return functionMap;
}

AcceleratedGuiRenderSystem::FunctionMap& AcceleratedGuiRenderSystem::getFunctionMap()
{
	return functionMap;
}

#define INSERT_HUNGARIAN
#define INSERT_RUSSIAN
//#define INSERT_JAPANESE

const threadsafe<std::vector<sWidget> >& AcceleratedGuiRenderSystem::getWidgets() const
{
	return widgets;
}

threadsafe<std::vector<sWidget> >& AcceleratedGuiRenderSystem::getWidgets()
{
	return widgets;
}

void AcceleratedGuiRenderSystem::updateLogic()
{
	if(richie && logicTicks > 10) {
		TXT::RichTextProcessor& RT = *richie;
		float fpsMin, fpsAvg, fpsMax;
		fpsCounter.queryData(fpsMin,fpsAvg,fpsMax);
		std::stringstream strm;
		TXT::MmlParser parser(richie.get());
#ifdef INSERT_HUNGARIAN
		strm << "<colour=#FF0000><b>Magyar </b></colour><colour=#FFFFFF><u>nyelven &amp; </u></colour><colour=#00FF00><i>írtam.</i></colour><br>";
#endif
#ifdef INSERT_RUSSIAN
		strm << "<b><s><colour=#FFFFFF><b>Я </b></colour><colour=#0000AA><u>люблю </u></colour><colour=#AA0000><i>Нику.</i></colour><br></s></b>";
#endif
#ifdef INSERT_JAPANESE
		strm << "<colour=#AA00AA>ニカが大好きです。</colour><br>";
#endif
		strm << "<colour=#FF0000>FPS min: "<< fpsMin << "</colour><br>";
		strm << "<colour=#A0A0A0>FPS avg: "<< fpsAvg << "</colour><br>";
		strm << "<colour=#00FF00>FPS max: "<< fpsMax << "</colour><br>";
		parser.parse(strm.str());
		richie->flush();
		textToRender = richie->getBlocks();
		richie->getBlocks().clear();
		logicTicks = 0;
	} else ++logicTicks;
}


void AcceleratedGuiRenderSystem::render()
{
	fpsCounter.singleTick([this]() {
		glClearColor(0.2f,0.2f,0.2f,1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(font) {
			//font->renderText(*this,txt,glm::fvec2(-0.75f,-0.75f),sizeReciprocal,0.5f,glm::fvec4(0.99f,0.35f,0.35f,1.0f),8);
			TXT::Font::renderTextBlocks(*guiRenderer,textToRender,glm::fvec2(-0.9f,-0.9f),sizeReciprocal,0.5f,8);
		}
		widgets.access( [this](const std::vector<sWidget>& cntr) {
			for(auto& it : cntr) {
				it->render(*guiRenderer);
			}
		});
		if(cursor) cursor->render(*guiRenderer,mousePos,sizeReciprocal);
		else {
		const glm::fvec2 mouseBottom = glm::fvec2(mousePos.x + (sizeReciprocal.x * 10.f), mousePos.y);
		const glm::fvec2 mouseRight = glm::fvec2(mousePos.x, mousePos.y + (sizeReciprocal.y * 10.f));
		guiRenderer->renderCTriang(mousePos,mouseBottom,mouseRight,glm::fvec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
		context->swapBuffers();
	});
}

void AcceleratedGuiRenderSystem::onResolutionChange(int newWidth, int newHeight)
{
	viewport = glm::ivec4(0,0,newWidth,newHeight);
	projection = glm::ortho(0,newWidth,0,newHeight);
	sizeReciprocal = glm::fvec2(2.0f/static_cast<float>(newWidth),2.0f/static_cast<float>(newHeight));
}

static const glm::ivec2 virtualRes = glm::ivec2(320,240);

AcceleratedGuiRenderSystem::AcceleratedGuiRenderSystem(const std::string& title, int offsetX, int offsetY, int width, int height, Uint32 flags)
	: AppSystem(title,offsetX,offsetY,width,height,flags), currentWidget(nullptr),
	  strbuffer(""), fullscreen(false), mousePos(glm::fvec2(0.0,0.0f)), cursor(nullptr), font(nullptr), richie(nullptr), logicTicks(0)
{
	onResolutionChange(width,height);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_VERSION(&sysinfo.version)
	SDL_GetWindowWMInfo(window.get(), &sysinfo);
	context = std::make_unique<EGL::Context>(sysinfo.info.x11.display,sysinfo.info.x11.window);
	context->makeCurrent();
	gladLoaderLoadGLES2();
	guiRenderer = std::make_unique<GL::Gui>();
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//SDL_SetRelativeMouseMode(SDL_TRUE);
	//SDL_StartTextInput();
}

glm::fvec2 AcceleratedGuiRenderSystem::absToRel(const glm::ivec2& abs) const
{
	return glm::fvec2(
				((static_cast<float>(abs.x) / static_cast<float>(viewport.z)) * 2.0f) - 1.0f,
				((static_cast<float>(abs.y) / static_cast<float>(viewport.w)) * 2.0f) - 1.0f
				);
}

glm::fvec2 AcceleratedGuiRenderSystem::absToRel(const glm::ivec2& abs, const glm::ivec2& customRes) const
{
	return glm::fvec2(
				((static_cast<float>(abs.x) / static_cast<float>(customRes.x)) * 2.0f) - 1.0f,
				((static_cast<float>(abs.y) / static_cast<float>(customRes.y)) * 2.0f) - 1.0f
				);
}

glm::ivec2 AcceleratedGuiRenderSystem::relToAbs(const glm::fvec2& rel) const
{
	return glm::ivec2(
				static_cast<int>(((rel.x + 1.0f) * 0.5f) * static_cast<float>(viewport.z)),
				static_cast<int>(((rel.y + 1.0f) * 0.5f) * static_cast<float>(viewport.y))
				);
}

void AcceleratedGuiRenderSystem::handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleDollarGestureEvent(const SDL_DollarGestureEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleDropEvent(const SDL_DropEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleTouchFingerEvent(const SDL_TouchFingerEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleJoyAxisEvent(const SDL_JoyAxisEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleJoyBallEvent(const SDL_JoyBallEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleJoyHatEvent(const SDL_JoyHatEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleJoyButtonEvent(const SDL_JoyButtonEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent& event)
{
	IWidget* pcwidg = currentWidget;
	IWidget* cwidg = nullptr;
	mousePos = glm::fvec2( static_cast<float>(event.x), static_cast<float>(event.y) ) * sizeReciprocal - glm::fvec2(1.0f,1.0f);
	const auto relpos = glm::fvec2( static_cast<float>(event.xrel), static_cast<float>(event.yrel) ) * sizeReciprocal;
	widgets.access( [&,this](std::vector<sWidget>& cntr) {
	for(auto& it : cntr) {
		const auto& tl = it->getTopLeft();
		const auto& br = it->getBottomRight();
		if( (mousePos.x >= tl.x && mousePos.x <= br.x) && (mousePos.y >= tl.y && mousePos.y <= br.y) ) {
			if(it->onHover(IWidget::getRelativePosFromAbs(tl,br,mousePos),relpos)) cwidg = it.get();
			break;
		}
	}
	});
	if(pcwidg && pcwidg != cwidg) {
		pcwidg->onOutOfFocus();
	}
	currentWidget = cwidg;
}

void AcceleratedGuiRenderSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
	if(currentWidget) {
		const auto& tl = currentWidget->getTopLeft();
		const auto& br = currentWidget->getBottomRight();
		currentWidget->onClick(IWidget::getRelativePosFromAbs(tl,br,mousePos),event.button,event.state,event.clicks);
	}
}

void AcceleratedGuiRenderSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleMultiGestureEvent(const SDL_MultiGestureEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleQuitEvent(const SDL_QuitEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleTextEditingEvent(const SDL_TextEditingEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleKeyboardEvent(const SDL_KeyboardEvent& event)
{
	//(void)event;
	if(event.type == SDL_KEYDOWN) {
		switch (event.keysym.sym) {
		case SDLK_ESCAPE: SDL_Quit(); exit(0); break;
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
				SDL_SetWindowFullscreen(window.get(),SDL_WINDOW_FULLSCREEN_DESKTOP);
				//SDL_RenderSetLogicalSize(renderer.get(),viewport.z,viewport.z);
				fullscreen = true;
			}
			break;
		}
		default: {
			auto k = functionMap.find(event.keysym.sym);
			if(k != std::end(functionMap)) {
				k->second(this);
			}
			break;
		}
		}
	}
}

void AcceleratedGuiRenderSystem::handleTextInputEvent(const SDL_TextInputEvent& event)
{
	//(void)event;
	strbuffer += event.text;
	//std::cout << std::string(event.text,32) << std::endl;
}

void AcceleratedGuiRenderSystem::handleUserEvent(const SDL_UserEvent& event)
{
	(void)event;
}

void AcceleratedGuiRenderSystem::handleWindowEvent(const SDL_WindowEvent& event)
{
	switch (event.event) {
		case SDL_WINDOWEVENT_CLOSE: SDL_Quit(); exit(0); break;
		case SDL_WINDOWEVENT_ENTER: // break through
		case SDL_WINDOWEVENT_FOCUS_GAINED: /*SDL_SetRelativeMouseMode(SDL_TRUE);*/ break;
		case SDL_WINDOWEVENT_LEAVE: // break through
		case SDL_WINDOWEVENT_FOCUS_LOST: /*SDL_SetRelativeMouseMode(SDL_FALSE);*/ break;
		case SDL_WINDOWEVENT_SIZE_CHANGED: onResolutionChange(event.data1,event.data2); break;
		default: break;
	}
}
}
