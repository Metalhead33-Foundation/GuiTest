#include <omp.h>
#include <iostream>
#include <MhLib/Util/PixelFormat.hpp>
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
#include "MhLib/Media/AdvancedAudio/MhAudioStreamer.hpp"
#include <System/SystemConfiguration.hpp>
#include <System/GameSystem.hpp>
#include <Renderer/Software/SoftwareRenderer.hpp>
#include <MhLib/Media/Image/MhPNG.hpp>
#include <MhLib/Io/MhFile.hpp>
//#include "Texture/TextureAtlas.hpp"

/**static const int WIDTH = 640;
static const int HEIGHT = 480;*/

static const int CIRCLE_W = 128;
static const int CIRCLE_H = 128;
static const glm::ivec2 virtualRes(640,480);

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

/*#define ACCELERATED
#ifdef ACCELERATED
typedef SYS::AcceleratedGuiRenderSystem GUISYS;
static const bool isAccelerated = true;
#else
typedef SYS::GuiRenderSystem GUISYS;
static const bool isAccelerated = false;
#endif*/

/*
typedef MH33::GFX::ResourceFactory* (*RESOURCE_FACTORY_CREATOR)(SDL_Window*,SDL_SysWMinfo*);
typedef void (*RESOURCE_FACTORY_DESTROYER)(MH33::GFX::ResourceFactory*);
*/
MH33::GFX::ResourceFactory* CreateSoftwareRenderer(SDL_Window* w,SDL_SysWMinfo* wi)
{
	return new Renderer::Software::SoftwareRenderer(w,wi);
}
void DestroySoftwareRenderer(MH33::GFX::ResourceFactory* fac) {
	delete fac;
}

int main()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	std::ifstream file("configuration.ini");
	SYS::Configuration config(file);
	file.close();
	SYS::GameSystem gamesys(CreateSoftwareRenderer,DestroySoftwareRenderer,config.getSections()["Renderer"],config.getSections()["Audio"]);
	{
		auto sfile = std::make_shared<MH33::Io::File>("wodmouse.png", MH33::Io::Mode::READ);
		MH33::Image::DecodeTarget target;
		MH33::Image::PNG::decode(*sfile,target);
		auto cursortext = gamesys.getResourceFactory()->createTexture2D(target);
		auto cursor = std::make_shared<SYS::TCursor>(cursortext);
		gamesys.setCursor(cursor);
	}
	{
		MH33::Image::DecodeTarget span1,span2,span3;
		createCircleTextures(span1, span2, span3, CIRCLE_W,CIRCLE_H,true);
		std::vector<bool> bitmap;
		glm::ivec2 circleDimensions;
		auto tex1 = gamesys.getResourceFactory()->createTexture2D(span1,bitmap,circleDimensions);
		auto tex2 = gamesys.getResourceFactory()->createTexture2D(span2);
		auto tex3 = gamesys.getResourceFactory()->createTexture2D(span3);
		gamesys.getWidgets().access( [tex1,tex2,tex3,&bitmap](std::vector<SYS::sWidget>& widgets) {
			widgets.push_back(std::make_shared<SYS::TexturedWidget>(absToRel(glm::ivec2(200,200),virtualRes),absToRel(glm::ivec2(200+CIRCLE_W*2,200+CIRCLE_H*2),virtualRes),
																	std::move(tex1),
																	std::move(tex2),
																	std::move(tex3),std::move(bitmap)));
		});
	}
	gamesys.run();
/*	gladLoaderLoadEGL(nullptr);
	Driver::SDL SDL_AudioSystem(48000,2,4096);
	MH33::Io::sDevice musicfile = std::make_shared<MH33::Io::File>("/home/legacy/zene/Jmusic/Records of Lodoss War/Record Of Lodoss War - Opening 1 - Kiseki No Umi-lO-kxhToG48.ogg",MH33::Io::Mode::READ);
	auto musicplayer = std::make_shared<MH33::Audio::Streamer>(musicfile);
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
	delete app;*/
	return 0;
}
