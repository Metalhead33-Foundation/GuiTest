#include "GuiRenderSystem.hpp"
#include "../Widget/BoxWidget.hpp"
#include "../Widget/TickboxWidget.hpp"
#include "../Widget/TexturedWidget.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string_view>
#include "../Util/TextureHelpers.hpp"
#include "../Util/TextureFromSurface.hpp"
#include <sstream>

const sCursor& GuiRenderSystem::getCursor() const
{
	return cursor;
}

void GuiRenderSystem::setCursor(const sCursor& newCursor)
{
	cursor = newCursor;
}

void GuiRenderSystem::setCursor(sCursor&& newCursor)
{
	cursor = std::move(newCursor);
}

const sFontRepository& GuiRenderSystem::getFont() const
{
	return font;
}

void GuiRenderSystem::setFont(const sFontRepository& newFont)
{
	font = newFont;
	richie = std::make_shared<RichTextProcessor>(std::ref(font));
	//richie = sRichTextProcessor(new RichTextProcessor(font));
}

void GuiRenderSystem::setFont(sFontRepository&& newFont)
{
	font = std::move(newFont);
	richie = std::make_shared<RichTextProcessor>(std::ref(font));
	//richie = sRichTextProcessor(new RichTextProcessor(font));
}

static std::string txt = "Multi-line\ntext";

const GuiRenderSystem::FunctionMap& GuiRenderSystem::getFunctionMap() const
{
	return functionMap;
}

GuiRenderSystem::FunctionMap& GuiRenderSystem::getFunctionMap()
{
	return functionMap;
}

#define INSERT_HUNGARIAN
#define INSERT_RUSSIAN
//#define INSERT_JAPANESE

void GuiRenderSystem::updateLogic()
{
	std::stringstream sstrm;
	if(richie) {
		RichTextProcessor& RT = *richie;
		float fpsMin, fpsAvg, fpsMax;
		fpsCounter.queryData(fpsMin,fpsAvg,fpsMax);

	#ifdef INSERT_HUNGARIAN
		RT << RT.ChangeColour(255,0,0) << "Magyarul " << RT.ChangeColour(255,255,255) <<  "írt "<< RT.ChangeColour(0,255,0) << "szöveg." << std::endl;
	#endif
	#ifdef INSERT_RUSSIAN
		RT << RT.ChangeColour(255,255,255) << "Я " << RT.ChangeColour(0,0,255) << "люблю " << RT.ChangeColour(200,0,0) << "Нику." << std::endl;
	#endif
	#ifdef INSERT_JAPANESE
		RT << "ニカが大好きです。" << std::endl;
	#endif
		RT << RT.ChangeColour(255,0,0) << RT.EnableBold() << "FPS min: " << RT.DisableBold() << fpsMin << std::endl;
		RT << RT.ChangeColour(128,128,128) << RT.EnableBold() << "FPS avg: " << RT.DisableBold() << fpsAvg << std::endl;
		RT << RT.ChangeColour(0,255,0) << RT.EnableBold() << "FPS max: " << RT.DisableBold() << fpsMax << std::endl;
		richie->flush();
		textToRender = richie->getBlocks();
		richie->getBlocks().clear();
	}
}


void GuiRenderSystem::render()
{
	fpsCounter.singleTick([this]() {
		framebuffer->clearToColour(glm::fvec4(0.0f,0.0f,0.0f,0.0f));
		zbuffer->clear();
		if(font) {
			//font->renderText(*this,txt,glm::fvec2(-0.75f,-0.75f),sizeReciprocal,0.5f,glm::fvec4(0.99f,0.35f,0.35f,1.0f),8);
			Font::renderTextBlocks(*this,textToRender,glm::fvec2(-0.75f,-0.75f),sizeReciprocal,0.5f,8);
		}
		widgets.access( [this](const std::vector<sWidget>& cntr) {
			for(auto& it : cntr) {
				it->render(*this);
			}
		});
		if(cursor) cursor->render(*this,mousePos,sizeReciprocal);
		else {
		const glm::fvec2 mouseBottom = glm::fvec2(mousePos.x + (sizeReciprocal.x * 10.f), mousePos.y);
		const glm::fvec2 mouseRight = glm::fvec2(mousePos.x, mousePos.y + (sizeReciprocal.y * 10.f));
		renderCTriang(mousePos,mouseBottom,mouseRight,glm::fvec4(1.0f, 1.0f, 1.0f, 1.0f));
		}
		SDL_UpdateWindowSurface(window.get());
	});
}

void GuiRenderSystem::onResolutionChange(int newWidth, int newHeight)
{
	SDL_Surface * window_surface = SDL_GetWindowSurface(window.get());
	viewport = glm::ivec4(0,0,newWidth,newHeight);
	projection = glm::ortho(0,newWidth,0,newHeight);
	sizeReciprocal = glm::fvec2(2.0f/static_cast<float>(newWidth),2.0f/static_cast<float>(newHeight));
	framebuffer = textureFromSurface(*window_surface);
	zbuffer = std::make_shared<ZBuffer>(newWidth,newHeight);
	bpipeline.framebuffer = framebuffer.get();
	tpipeline.framebuffer = framebuffer.get();
	ctpipeline.framebuffer = framebuffer.get();
	bpipeline.viewport = viewport;
	tpipeline.viewport = viewport;
	ctpipeline.viewport = viewport;
}

static const int CIRCLE_W = 128;
static const int CIRCLE_H = 128;

static const glm::ivec2 virtualRes = glm::ivec2(320,240);

GuiRenderSystem::GuiRenderSystem(const std::string& title, int offsetX, int offsetY, int width, int height, Uint32 flags)
	: AppSystem(title,offsetX,offsetY,width,height,flags), currentWidget(nullptr),
	  strbuffer(""), fullscreen(false), mousePos(glm::fvec2(0.0,0.0f)), cursor(nullptr), font(nullptr), richie(nullptr)
{
	bpipeline.uniform.blending = ALPHA_DITHERING;
	bpipeline.vert = basicVertexShader;
	bpipeline.frag = basicFragmentShader;
	tpipeline.uniform.blending = ALPHA_DITHERING;
	tpipeline.uniform.samplerState.wrap = Wrap::MIRRORED_REPEAT;
	tpipeline.uniform.samplerState.filtering = TextureFiltering::DITHERED;
	tpipeline.vert = TexturedVertexShader;
	tpipeline.frag = TexturedFragmentShader;
	ctpipeline.uniform.blending = ALPHA_TESTING;
	ctpipeline.uniform.samplerState.wrap = Wrap::MIRRORED_REPEAT;
	ctpipeline.uniform.samplerState.filtering = TextureFiltering::NEAREST_NEIGHBOUR;
	ctpipeline.vert = ColouredTexturedVertexShader;
	ctpipeline.frag = ColouredTexturedFragmentShader;
	onResolutionChange(width,height);
	SDL_ShowCursor(SDL_DISABLE);
	//SDL_SetRelativeMouseMode(SDL_TRUE);
	//SDL_StartTextInput();
	std::vector<uint32_t> tex1,tex2,tex3;
	createCircleTextures(tex1,tex2,tex3,CIRCLE_W,CIRCLE_H);
	widgets.access( [&,this](std::vector<sWidget>& cntr) {
		cntr.reserve(256);
		cntr.push_back(std::make_shared<BoxWidget>(absToRel(glm::ivec2(25,25),virtualRes),absToRel(glm::ivec2(125,125),virtualRes)));
		cntr.push_back(std::make_shared<TickboxWidget>(absToRel(glm::ivec2(100,100),virtualRes),absToRel(glm::ivec2(200,200),virtualRes),1));
		cntr.push_back(std::make_shared<TexturedWidget>(absToRel(glm::ivec2(200,200),virtualRes),absToRel(glm::ivec2(200+CIRCLE_W,200+CIRCLE_H),virtualRes),CIRCLE_W,CIRCLE_H,tex1.data(),tex2.data(),tex3.data()));
	});
}

glm::fvec2 GuiRenderSystem::absToRel(const glm::ivec2& abs) const
{
	return glm::fvec2(
				((static_cast<float>(abs.x) / static_cast<float>(viewport.z)) * 2.0f) - 1.0f,
				((static_cast<float>(abs.y) / static_cast<float>(viewport.w)) * 2.0f) - 1.0f
				);
}

glm::fvec2 GuiRenderSystem::absToRel(const glm::ivec2& abs, const glm::ivec2& customRes) const
{
	return glm::fvec2(
				((static_cast<float>(abs.x) / static_cast<float>(customRes.x)) * 2.0f) - 1.0f,
				((static_cast<float>(abs.y) / static_cast<float>(customRes.y)) * 2.0f) - 1.0f
				);
}

glm::ivec2 GuiRenderSystem::relToAbs(const glm::fvec2& rel) const
{
	return glm::ivec2(
				static_cast<int>(((rel.x + 1.0f) * 0.5f) * static_cast<float>(viewport.z)),
				static_cast<int>(((rel.y + 1.0f) * 0.5f) * static_cast<float>(viewport.y))
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

void GuiRenderSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
	if(currentWidget) {
		const auto& tl = currentWidget->getTopLeft();
		const auto& br = currentWidget->getBottomRight();
		currentWidget->onClick(IWidget::getRelativePosFromAbs(tl,br,mousePos),event.button,event.state,event.clicks);
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

void GuiRenderSystem::renderCLine(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour, int thickness)
{
	bpipeline.renderLine(
				BasicVertexIn{ .POS = p0, .COLOUR = colour },
				BasicVertexIn{ .POS = p1, .COLOUR = colour },
				thickness);
}

void GuiRenderSystem::renderCRect(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour)
{
	bpipeline.renderRectangle(
				BasicVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .COLOUR = colour },
				BasicVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .COLOUR = colour }
				);
}

void GuiRenderSystem::renderCTriang(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2& p2, const glm::fvec4& colour)
{
	BasicVertexIn vertices[] = { BasicVertexIn{ .POS = p0, .COLOUR = colour },
	BasicVertexIn{ .POS = p1, .COLOUR = colour },
	BasicVertexIn{ .POS = p2, .COLOUR = colour }
	};
	bpipeline.renderTriangles(vertices);
}

void GuiRenderSystem::renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const Texture& tex)
{
	tpipeline.uniform.tex = &tex;
	tpipeline.renderRectangle(
				TexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = t0 },
				TexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = t1 }
				);
}

void GuiRenderSystem::renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const Texture& tex)
{
	tpipeline.uniform.tex = &tex;
	tpipeline.renderRectangle(
				TexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = glm::fvec2(0.0f, 0.0f) },
				TexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = glm::fvec2(1.0f, 1.0f) }
				);
}

void GuiRenderSystem::renderTex(const Texture& tex)
{
	tpipeline.uniform.tex = &tex;
	tpipeline.renderRectangle(
				TexturedVertexIn{ .POS = glm::fvec2(-1.0f,-1.0f), .TEXCOORD = glm::fvec2(0.0f, 0.0f) },
				TexturedVertexIn{ .POS = glm::fvec2(1.0f,1.0f), .TEXCOORD = glm::fvec2(1.0f, 1.0f) }
				);
}


void GuiRenderSystem::renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const Texture& tex)
{
	ctpipeline.uniform.tex = &tex;
	ctpipeline.renderRectangle(
				ColouredTexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = t0, .COLOUR = colour },
				ColouredTexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = t1, .COLOUR = colour }
				);
}

void GuiRenderSystem::renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::vec4& colour, const Texture& tex)
{
	ctpipeline.uniform.tex = &tex;
	ctpipeline.renderRectangle(
				ColouredTexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = glm::fvec2(0.0f, 0.0f), .COLOUR = colour },
				ColouredTexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = glm::fvec2(1.0f, 1.0f), .COLOUR = colour }
				);
}

void GuiRenderSystem::renderCTex(const glm::vec4& colour, const Texture& tex)
{
	ctpipeline.uniform.tex = &tex;
	ctpipeline.renderRectangle(
				ColouredTexturedVertexIn{ .POS = glm::fvec2(-1.0f,-1.0f), .TEXCOORD = glm::fvec2(0.0f, 0.0f), .COLOUR = colour },
				ColouredTexturedVertexIn{ .POS = glm::fvec2(1.0f,1.0f), .TEXCOORD = glm::fvec2(1.0f, 1.0f), .COLOUR = colour }
				);
}
