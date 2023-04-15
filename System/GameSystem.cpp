#include <System/GameSystem.hpp>
#include <Text/MmlParser.hpp>

namespace SYS {


const Driver::SDL& GameSystem::getSdlAudio() const
{
	return sdlAudio;
}

Driver::SDL& GameSystem::getSdlAudio()
{
	return sdlAudio;
}

TXT::FontRepository GameSystem::getFontRepository()
{
	return *fontRepository;
}

const TXT::sRichTextProcessor& GameSystem::getRichTextProcessor() const
{
	return richTextProcessor;
}

const sCursor& GameSystem::getCursor() const
{
	return cursor;
}

void GameSystem::setCursor(const sCursor& newCursor)
{
	cursor = newCursor;
}

const MH33::GFX::sResourceFactory& GameSystem::getResourceFactory() const
{
	return resourceFactory;
}
const threadsafe<std::vector<sWidget> >& GameSystem::getWidgets() const
{
	return widgets;
}

threadsafe<std::vector<sWidget> >& GameSystem::getWidgets()
{
	return widgets;
}

GameSystem::GameSystem(RESOURCE_FACTORY_CREATOR rendererCreator, RESOURCE_FACTORY_DESTROYER rendererDestroyer, const ConfigurationSection& rendererSettings, const ConfigurationSection& audioSettings)
	: logicTicks(0), resourceFactory(nullptr,rendererDestroyer), AppSystem(rendererSettings.getValueOrDefaultStr("sTitle"),0,0,
												  rendererSettings.getValueOrDefaultInt("iWIdth",1024),rendererSettings.getValueOrDefaultInt("iHeight",768),0),
	  sdlAudio(audioSettings.getValueOrDefaultInt("iSamplerate",48000),2,4096), currentWidget(nullptr)
{
	SDL_ShowCursor(SDL_DISABLE);
	resourceFactory = MH33::GFX::sResourceFactory(rendererCreator(window.get(),&windowInfo),rendererDestroyer);
	textContext = TXT::uTextRenderingContext(new TXT::TextRenderingContext(*resourceFactory));
	guiContext = MH33::GFX::uGuiRenderingContext(new MH33::GFX::GuiRenderingContext(*resourceFactory));
	{
		FT_Library ft;
		FT_Init_FreeType(&ft);
		TXT::sFreeTypeSystem sys(ft,FT_Done_FreeType);
		fontRepository = std::make_shared<TXT::FontRepository>(sys,resourceFactory);
		fontRepository->initializeFont(rendererSettings.getValueOrDefaultStr("DefaultFont","noto"),rendererSettings.getValueOrDefaultStr("DefaultFontPath","NotoTraditionalNushu-Regular.ttf"));
		richTextProcessor = std::make_shared<TXT::RichTextProcessor>(fontRepository);
		richTextProcessor->setDefaultFontName(rendererSettings.getValueOrDefaultStr("DefaultFont","noto"));
		richTextProcessor->setCurrentFontName(rendererSettings.getValueOrDefaultStr("DefaultFont","noto"));
	}
}

#define INSERT_HUNGARIAN
#define INSERT_RUSSIAN

void GameSystem::updateLogic()
{
	if(richTextProcessor && logicTicks >= 10) {
		TXT::RichTextProcessor& RT = *richTextProcessor;
		//float fpsMin, fpsAvg, fpsMax;
		std::stringstream strm;
		TXT::MmlParser parser(richTextProcessor.get());
#ifdef INSERT_HUNGARIAN
		strm << "<colour=#FF0000><b>Magyar </b></colour><colour=#FFFFFF><u>nyelven &amp; </u></colour><colour=#00FF00><i>írtam.</i></colour><br>";
#endif
#ifdef INSERT_RUSSIAN
		strm << "<b><s><colour=#FFFFFF><b>Я </b></colour><colour=#0000AA><u>люблю </u></colour><colour=#AA0000><i>Нику.</i></colour><br></s></b>";
#endif
#ifdef INSERT_JAPANESE
		strm << "<colour=#AA00AA>ニカが大好きです。</colour><br>";
#endif
		/*strm << "<colour=#FF0000>FPS min: "<< fpsMin << "</colour><br>";
		strm << "<colour=#A0A0A0>FPS avg: "<< fpsAvg << "</colour><br>";
		strm << "<colour=#00FF00>FPS max: "<< fpsMax << "</colour><br>";*/
		parser.parse(strm.str());
		richTextProcessor->flush();
		textToRender = richTextProcessor->getBlocks();
		richTextProcessor->getBlocks().clear();
		logicTicks = 0;
	} else ++logicTicks;
}

void GameSystem::render()
{
	resourceFactory->beginFrame();
	TXT::Font::renderTextBlocks(*textContext,textToRender,glm::fvec2(-0.9f,-0.9f),sizeReciprocal,0.5f,8);
	widgets.access( [this](const std::vector<sWidget>& cntr) {
		for(auto& it : cntr) {
			it->render(*guiContext);
		}
	});
	if(cursor) cursor->render(*guiContext,mousePos,sizeReciprocal);
	resourceFactory->endFrame();
}

void GameSystem::handleAudioDeviceEvent(const SDL_AudioDeviceEvent& event)
{
}

void GameSystem::handleControllerAxisEvent(const SDL_ControllerAxisEvent& event)
{
}

void GameSystem::handleControllerButtonEvent(const SDL_ControllerButtonEvent& event)
{
}

void GameSystem::handleControllerDeviceEvent(const SDL_ControllerDeviceEvent& event)
{
}

void GameSystem::handleDollarGestureEvent(const SDL_DollarGestureEvent& event)
{
}

void GameSystem::handleDropEvent(const SDL_DropEvent& event)
{
}

void GameSystem::handleTouchFingerEvent(const SDL_TouchFingerEvent& event)
{
}

void GameSystem::handleKeyboardEvent(const SDL_KeyboardEvent& event)
{
	if(event.keysym.sym == SDLK_ESCAPE) SDL_Quit();
}

void GameSystem::handleJoyAxisEvent(const SDL_JoyAxisEvent& event)
{
}

void GameSystem::handleJoyBallEvent(const SDL_JoyBallEvent& event)
{
}

void GameSystem::handleJoyHatEvent(const SDL_JoyHatEvent& event)
{
}

void GameSystem::handleJoyButtonEvent(const SDL_JoyButtonEvent& event)
{
}

void GameSystem::handleJoyDeviceEvent(const SDL_JoyDeviceEvent& event)
{
}

void GameSystem::handleMouseMotionEvent(const SDL_MouseMotionEvent& event)
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

void GameSystem::handleMouseButtonEvent(const SDL_MouseButtonEvent& event)
{
	if(currentWidget) {
		const auto& tl = currentWidget->getTopLeft();
		const auto& br = currentWidget->getBottomRight();
		currentWidget->onClick(IWidget::getRelativePosFromAbs(tl,br,mousePos),event.button,event.state,event.clicks);
	}
}

void GameSystem::handleMouseWheelEvent(const SDL_MouseWheelEvent& event)
{
}

void GameSystem::handleMultiGestureEvent(const SDL_MultiGestureEvent& event)
{
}

void GameSystem::handleQuitEvent(const SDL_QuitEvent& event)
{
	SDL_Quit();
}

void GameSystem::handleTextEditingEvent(const SDL_TextEditingEvent& event)
{
}

void GameSystem::handleTextInputEvent(const SDL_TextInputEvent& event)
{
}

void GameSystem::handleUserEvent(const SDL_UserEvent& event)
{
}

void GameSystem::handleWindowEvent(const SDL_WindowEvent& event)
{
}

}
