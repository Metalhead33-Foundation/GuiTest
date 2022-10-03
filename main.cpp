#include <omp.h>
#include <iostream>
#include "System/GuiRenderSystem.hpp"
#include "System/AcceleratedGuiRenderSystem.hpp"
#include <AGL/GlTexture2D.hpp>
#include <MhLib/Util/PixelFormat.hpp>
#include <MhLib/Util/TextureFromSurface.hpp>
#include <thread>
#include <SDL2/SDL_image.h>
#include <sstream>
#include "MhLib/Util/TextureHelpers.hpp"
#include "Widget/BoxWidget.hpp"
#include "Widget/TickboxWidget.hpp"
#include "Widget/TexturedWidget.hpp"
#include "System/AudioSDL.hpp"
#include "MhLib/Io/MhFile.hpp"
#include "MhLib/Media/AdvancedAudio/MhModulePlayer.hpp"
//#include "Texture/TextureAtlas.hpp"

/**static const int WIDTH = 640;
static const int HEIGHT = 480;*/

static const int WIDTH = 1024;
static const int HEIGHT = 768;
static const int CIRCLE_W = 128;
static const int CIRCLE_H = 128;
static const glm::ivec2 virtualRes(320,240);

static const char* DOSFONT = "Perfect DOS VGA 437.ttf";
static const char* ARIALFONT = "arial.ttf";
static const char* CJK = "NotoTraditionalNushu-Regular.ttf";

static glm::fvec2 absToRel(const glm::ivec2& abs, const glm::ivec2& customRes)
{
	return glm::fvec2(
				((static_cast<float>(abs.x) / static_cast<float>(customRes.x)) * 2.0f) - 1.0f,
				((static_cast<float>(abs.y) / static_cast<float>(customRes.y)) * 2.0f) - 1.0f
				);
}

typedef std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> uSUrface;

#define ACCELERATED
#ifdef ACCELERATED
typedef SYS::AcceleratedGuiRenderSystem GUISYS;
static const bool isAccelerated = true;
#else
typedef SYS::GuiRenderSystem GUISYS;
static const bool isAccelerated = false;
#endif
int main()
{
	gladLoaderLoadEGL(nullptr);
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	Driver::SDL SDL_AudioSystem(48000,2,4096);
	MH33::Io::sDevice musicfile = std::make_shared<MH33::Io::File>("/home/legacy/zene/GameMusic/Unreal Tournament/mod/09 - Lock.it",MH33::Io::Mode::READ);
	auto musicplayer = std::make_shared<MH33::Audio::ModulePlayer>(musicfile);
	SDL_AudioSystem.setPlayable(musicplayer);
	musicplayer->setState(MH33::Audio::PlayStatus::PLAYING);
	SDL_AudioSystem.pause(false);
	GUISYS* app = new GUISYS("GUI Render Demo",0,0,WIDTH,HEIGHT,0);
	std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> surfacePtr(IMG_Load("wodmouse.png"),SDL_FreeSurface);
	auto tex = SoftwareRenderer::textureFromSurfaceCopySA(*surfacePtr,isAccelerated);
	uSUrface tex1(SDL_CreateRGBSurfaceWithFormat(0,CIRCLE_W,CIRCLE_H,0,SDL_PIXELFORMAT_ARGB8888),SDL_FreeSurface);
	uSUrface tex2(SDL_CreateRGBSurfaceWithFormat(0,CIRCLE_W,CIRCLE_H,0,SDL_PIXELFORMAT_ARGB8888),SDL_FreeSurface);
	uSUrface tex3(SDL_CreateRGBSurfaceWithFormat(0,CIRCLE_W,CIRCLE_H,0,SDL_PIXELFORMAT_ARGB8888),SDL_FreeSurface);
	auto span1 = std::span<uint32_t>(static_cast<uint32_t*>(tex1->pixels),CIRCLE_H*CIRCLE_W);
	auto span2 = std::span<uint32_t>(static_cast<uint32_t*>(tex2->pixels),CIRCLE_H*CIRCLE_W);
	auto span3 = std::span<uint32_t>(static_cast<uint32_t*>(tex3->pixels),CIRCLE_H*CIRCLE_W);
	createCircleTextures(span1, span2, span3, CIRCLE_W,CIRCLE_H,true);
	auto atatex1 = SoftwareRenderer::textureFromSurfaceCopySA(*tex1,isAccelerated);
	auto atatex2 = SoftwareRenderer::textureFromSurfaceCopySA(*tex2,isAccelerated);
	auto atatex3 = SoftwareRenderer::textureFromSurfaceCopySA(*tex3,isAccelerated);
		if(isAccelerated)
		{
			SYS::sCursor cursor = std::make_shared<SYS::TCursor>(std::move(tex));
			app->setCursor(std::move(cursor));

			app->getWidgets().access( [&](std::vector<SYS::sWidget>& cntr) {
				cntr.reserve(256);
				cntr.push_back(std::make_shared<SYS::BoxWidget>(absToRel(glm::ivec2(25,25),virtualRes),absToRel(glm::ivec2(125,125),virtualRes)));
				cntr.push_back(std::make_shared<SYS::TickboxWidget>(absToRel(glm::ivec2(100,100),virtualRes),absToRel(glm::ivec2(200,200),virtualRes),1));
				cntr.push_back(std::make_shared<SYS::TexturedWidget>(absToRel(glm::ivec2(200,200),virtualRes),absToRel(glm::ivec2(200+CIRCLE_W*2,200+CIRCLE_H*2),virtualRes),
																atatex1,
																atatex2,
																atatex3));
			});
		} else {
			auto tex = SoftwareRenderer::textureFromSurfaceCopy(*surfacePtr);
			SYS::sCursor cursor = std::make_shared<SYS::TCursor>(std::move(tex));
			app->setCursor(std::move(cursor));
			app->getWidgets().access( [&](std::vector<SYS::sWidget>& cntr) {
			cntr.reserve(256);
			cntr.push_back(std::make_shared<SYS::BoxWidget>(absToRel(glm::ivec2(25,25),virtualRes),absToRel(glm::ivec2(125,125),virtualRes)));
			cntr.push_back(std::make_shared<SYS::TickboxWidget>(absToRel(glm::ivec2(100,100),virtualRes),absToRel(glm::ivec2(200,200),virtualRes),1));
			cntr.push_back(std::make_shared<SYS::TexturedWidget>(absToRel(glm::ivec2(200,200),virtualRes),absToRel(glm::ivec2(200+CIRCLE_W*2,200+CIRCLE_H*2),virtualRes),
															atatex1,
															atatex2,
															atatex3));
			});
		}
		FT_Library ft;
		if (FT_Init_FreeType(&ft)) {
			return -1;
		}
		TXT::sFreeTypeSystem sys(ft,FT_Done_FreeType);
				auto font = std::make_shared<TXT::FontRepository>( std::move(sys), isAccelerated );
		font->initializeFont("Noto",CJK);
		app->setFont(std::move(font));
	omp_set_dynamic(1);     // Explicitly enable dynamic teams
	app->run();
	//fontAdderThread.join();
	delete app;
	return 0;
}
