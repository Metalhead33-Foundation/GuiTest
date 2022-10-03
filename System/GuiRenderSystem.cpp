#include "GuiRenderSystem.hpp"
#include "../Widget/BoxWidget.hpp"
#include "../Widget/TickboxWidget.hpp"
#include "../Widget/TexturedWidget.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <string_view>
#include "../MhLib/Util/TextureFromSurface.hpp"
#include "../Text/MmlParser.hpp"
#include <sstream>
#include <omp.h>

namespace SYS {
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

const TXT::sFontRepository& GuiRenderSystem::getFont() const
{
	return font;
}

void GuiRenderSystem::setFont(const TXT::sFontRepository& newFont)
{
	font = newFont;
	richie = std::make_shared<TXT::RichTextProcessor>(std::ref(font));
	//richie = sRichTextProcessor(new RichTextProcessor(font));
}

void GuiRenderSystem::setFont(TXT::sFontRepository&& newFont)
{
	font = std::move(newFont);
	richie = std::make_shared<TXT::RichTextProcessor>(std::ref(font));
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

const threadsafe<std::vector<sWidget> >& GuiRenderSystem::getWidgets() const
{
	return widgets;
}

threadsafe<std::vector<sWidget> >& GuiRenderSystem::getWidgets()
{
	return widgets;
}

void GuiRenderSystem::updateLogic()
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


void GuiRenderSystem::render()
{
	fpsCounter.singleTick([this]() {
		ctpipeline.framebuffer->clearToColour(glm::fvec4(0.0f,0.0f,0.0f,0.0f));
		zbuffer->clear();
		if(font) {
			//font->renderText(*this,txt,glm::fvec2(-0.75f,-0.75f),sizeReciprocal,0.5f,glm::fvec4(0.99f,0.35f,0.35f,1.0f),8);
			TXT::Font::renderTextBlocks(*this,textToRender,glm::fvec2(-0.9f,-0.9f),sizeReciprocal,0.5f,8);
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
	framebuffer = SoftwareRenderer::textureFromSurface(*window_surface);
	zbuffer = std::make_shared<SoftwareRenderer::ZBuffer>(newWidth,newHeight);
	bpipeline.framebuffer = dynamic_cast<SoftwareRenderer::Texture*>(framebuffer.get());
	tpipeline.framebuffer = dynamic_cast<SoftwareRenderer::Texture*>(framebuffer.get());
	ctpipeline.framebuffer = dynamic_cast<SoftwareRenderer::Texture*>(framebuffer.get());
	bpipeline.viewport = viewport;
	tpipeline.viewport = viewport;
	ctpipeline.viewport = viewport;
}

static const glm::ivec2 virtualRes = glm::ivec2(320,240);

GuiRenderSystem::GuiRenderSystem(const std::string& title, int offsetX, int offsetY, int width, int height, Uint32 flags)
	: AppSystem(title,offsetX,offsetY,width,height,flags), currentWidget(nullptr),
	  strbuffer(""), fullscreen(false), mousePos(glm::fvec2(0.0,0.0f)), cursor(nullptr), font(nullptr), richie(nullptr), logicTicks(0)
{
	bpipeline.uniform.blending = SoftwareRenderer::ALPHA_DITHERING;
	bpipeline.vert = SoftwareRenderer::basicVertexShader;
	bpipeline.frag = SoftwareRenderer::basicFragmentShader;
	tpipeline.uniform.blending = SoftwareRenderer::ALPHA_DITHERING;
	tpipeline.uniform.samplerState.wrap = SoftwareRenderer::Wrap::MIRRORED_REPEAT;
	tpipeline.uniform.samplerState.filtering = SoftwareRenderer::TextureFiltering::DITHERED;
	tpipeline.vert = SoftwareRenderer::TexturedVertexShader;
	tpipeline.frag = SoftwareRenderer::TexturedFragmentShader;
	ctpipeline.uniform.blending = SoftwareRenderer::ALPHA_DITHERING;
	ctpipeline.uniform.samplerState.wrap = SoftwareRenderer::Wrap::MIRRORED_REPEAT;
	ctpipeline.uniform.samplerState.filtering = SoftwareRenderer::TextureFiltering::BILINEAR;
	ctpipeline.vert = SoftwareRenderer::ColouredTexturedVertexShader;
	ctpipeline.frag = SoftwareRenderer::ColouredTexturedFragmentShader;
	onResolutionChange(width,height);
	SDL_ShowCursor(SDL_DISABLE);
	//SDL_SetRelativeMouseMode(SDL_TRUE);
	//SDL_StartTextInput();
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
				SoftwareRenderer::BasicVertexIn{ .POS = p0, .COLOUR = colour },
				SoftwareRenderer::BasicVertexIn{ .POS = p1, .COLOUR = colour },
				thickness);
}

void GuiRenderSystem::renderCRect(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec4& colour)
{
	bpipeline.renderRectangle(
				SoftwareRenderer::BasicVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .COLOUR = colour },
				SoftwareRenderer::BasicVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .COLOUR = colour }
				);
}

void GuiRenderSystem::renderCTriang(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2& p2, const glm::fvec4& colour)
{
	SoftwareRenderer::BasicVertexIn vertices[] = { SoftwareRenderer::BasicVertexIn{ .POS = p0, .COLOUR = colour },
	SoftwareRenderer::BasicVertexIn{ .POS = p1, .COLOUR = colour },
	SoftwareRenderer::BasicVertexIn{ .POS = p2, .COLOUR = colour }
	};
	bpipeline.renderTriangles(vertices);
}

void GuiRenderSystem::renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const ITexture& tex)
{
	tpipeline.uniform.tex = dynamic_cast<const SoftwareRenderer::Texture*>(&tex);
	tpipeline.renderRectangle(
				SoftwareRenderer::TexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = t0 },
				SoftwareRenderer::TexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = t1 }
				);
}

void GuiRenderSystem::renderTex(const glm::fvec2& p0, const glm::fvec2& p1, const ITexture& tex)
{
	tpipeline.uniform.tex = dynamic_cast<const SoftwareRenderer::Texture*>(&tex);
	tpipeline.renderRectangle(
				SoftwareRenderer::TexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = glm::fvec2(0.0f, 0.0f) },
				SoftwareRenderer::TexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = glm::fvec2(1.0f, 1.0f) }
				);
}

void GuiRenderSystem::renderTex(const ITexture& tex)
{
	tpipeline.uniform.tex = dynamic_cast<const SoftwareRenderer::Texture*>(&tex);
	tpipeline.renderRectangle(
				SoftwareRenderer::TexturedVertexIn{ .POS = glm::fvec2(-1.0f,-1.0f), .TEXCOORD = glm::fvec2(0.0f, 0.0f) },
				SoftwareRenderer::TexturedVertexIn{ .POS = glm::fvec2(1.0f,1.0f), .TEXCOORD = glm::fvec2(1.0f, 1.0f) }
				);
}


void GuiRenderSystem::renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const ITexture& tex)
{
	ctpipeline.uniform.tex = dynamic_cast<const SoftwareRenderer::Texture*>(&tex);
	ctpipeline.renderRectangle(
				SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = t0, .COLOUR = colour },
				SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = t1, .COLOUR = colour }
				);
}

void GuiRenderSystem::renderCTex(const glm::fvec2& p0, const glm::fvec2& p1, const glm::vec4& colour, const ITexture& tex)
{
	ctpipeline.uniform.tex = dynamic_cast<const SoftwareRenderer::Texture*>(&tex);;
	ctpipeline.renderRectangle(
				SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::min(p0.y,p1.y)), .TEXCOORD = glm::fvec2(0.0f, 0.0f), .COLOUR = colour },
				SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = glm::fvec2(1.0f, 1.0f), .COLOUR = colour }
				);
}

void GuiRenderSystem::renderCTex(const glm::vec4& colour, const ITexture& tex)
{
	ctpipeline.uniform.tex = dynamic_cast<const SoftwareRenderer::Texture*>(&tex);
	ctpipeline.renderRectangle(
				SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(-1.0f,-1.0f), .TEXCOORD = glm::fvec2(0.0f, 0.0f), .COLOUR = colour },
				SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(1.0f,1.0f), .TEXCOORD = glm::fvec2(1.0f, 1.0f), .COLOUR = colour }
				);
}


void GuiRenderSystem::renderTiltedCTex(float tilt, const glm::fvec2& p0, const glm::fvec2& p1, const glm::fvec2 t0, const glm::fvec2& t1, const glm::vec4& colour, const ITexture& tex)
{
	const float xdiff = std::abs(std::max(p0.y,p1.y) - std::min(p0.y,p1.y)) * tilt;
	SoftwareRenderer::ColouredTexturedVertexIn vertices[] = {
		SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x)+xdiff,std::min(p0.y,p1.y)), .TEXCOORD = glm::fvec2(std::min(t0.x,t1.x), std::min(t0.y,t1.y)) , .COLOUR = colour },
		SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x)+xdiff,std::min(p0.y,p1.y)), .TEXCOORD = glm::fvec2(std::max(t0.x,t1.x), std::min(t0.y,t1.y)) , .COLOUR = colour },
		SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::min(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = glm::fvec2(std::min(t0.x,t1.x), std::max(t0.y,t1.y)) , .COLOUR = colour },
		SoftwareRenderer::ColouredTexturedVertexIn{ .POS = glm::fvec2(std::max(p0.x,p1.x),std::max(p0.y,p1.y)), .TEXCOORD = glm::fvec2(std::max(t0.x,t1.x), std::max(t0.y,t1.y)) , .COLOUR = colour }
	};
	unsigned int indices[] = { 0, 1, 2, 1, 2, 3 };
	ctpipeline.uniform.tex = dynamic_cast<const SoftwareRenderer::Texture*>(&tex);
	ctpipeline.renderTriangles(vertices,indices);
}
}
