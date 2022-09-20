#include <omp.h>
#include <iostream>
#include "System/GuiRenderSystem.hpp"
#include "System/AcceleratedGuiRenderSystem.hpp"
#include "Util/PixelFormat.hpp"
#include "Util/TextureFromSurface.hpp"
#include <thread>
#include <SDL2/SDL_image.h>
#include <sstream>
#include "Util/TextureHelpers.hpp"
#include "Widget/BoxWidget.hpp"
#include "Widget/TickboxWidget.hpp"
#include "Widget/TexturedWidget.hpp"
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

typedef SYS::AcceleratedGuiRenderSystem GUISYS;
//typedef SYS::GuiRenderSystem GUISYS;
int main()
{
	gladLoaderLoadEGL(nullptr);
	/*TextureAtlas atlas([](const glm::ivec2& size) {
		return new StandardTexture<PixelARGB8888>(size.x,size.y);
	},glm::ivec2(32,32),glm::ivec2(12,12));*/
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	GUISYS* app = new GUISYS("GUI Render Demo",0,0,WIDTH,HEIGHT,0);
	//auto cursorAdderThread = std::thread([app,&atlas]() {
	auto cursorAdderThread = std::thread([app]() {
		// textureFromSurfaceCopy
		std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> surfacePtr(IMG_Load("wodmouse.png"),SDL_FreeSurface);
		//auto tex = textureFromSurfaceCopy(*surfacePtr);
		std::vector<uint32_t> tex1,tex2,tex3;
		createCircleTextures(tex1,tex2,tex3,CIRCLE_W,CIRCLE_H);
		/*TexARGB8888 atex1(reinterpret_cast<PixelARGB8888*>(tex1.data()),CIRCLE_W,CIRCLE_H);
		TexARGB8888 atex2(reinterpret_cast<PixelARGB8888*>(tex2.data()),CIRCLE_W,CIRCLE_H);
		TexARGB8888 atex3(reinterpret_cast<PixelARGB8888*>(tex3.data()),CIRCLE_W,CIRCLE_H);
		sTexture atatex1(atlas.allocateBlocks(atex1));
		sTexture atatex2(atlas.allocateBlocks(atex2));
		sTexture atatex3(atlas.allocateBlocks(atex3));
		sTexture atatex4(atlas.allocateBlocks(*tex));

		sCursor cursor = std::make_shared<TCursor>(std::move(atatex4));
		app->setCursor(std::move(cursor));*/

		app->getWidgets().access( [&](std::vector<SYS::sWidget>& cntr) {
			cntr.reserve(256);
			cntr.push_back(std::make_shared<SYS::BoxWidget>(absToRel(glm::ivec2(25,25),virtualRes),absToRel(glm::ivec2(125,125),virtualRes)));
			cntr.push_back(std::make_shared<SYS::TickboxWidget>(absToRel(glm::ivec2(100,100),virtualRes),absToRel(glm::ivec2(200,200),virtualRes),1));
			/*cntr.push_back(std::make_shared<SYS::TexturedWidget>(absToRel(glm::ivec2(200,200),virtualRes),absToRel(glm::ivec2(200+CIRCLE_W*2,200+CIRCLE_H*2),virtualRes),
															atatex1,
															atatex2,
															atatex3));*/
		});

	});
	/*app->getFunctionMap().insert_or_assign(SDLK_SPACE,[](GUISYS* sys) {
		auto fontSys = sys->getFont();
		if(fontSys) {
			for(auto it = std::begin(fontSys->getFonts()); it != std::end(fontSys->getFonts());++it) {
				for(size_t i = 0; i < it->second.size(); ++i) {
					std::stringstream sstream;
					sstream << it->first << '-' << i << std::endl;
					auto str = sstream.str();
					uSUrface s(SDL_CreateRGBSurfaceWithFormat(0,it->second[i]->getTexture().getWidth(),it->second[i]->getTexture().getHeight(),8,SDL_PIXELFORMAT_RGB332),SDL_FreeSurface);
					memcpy(s->pixels,it->second[i]->getTexture().getRawPixels(),s->w * s->h);
					IMG_SavePNG(s.get(),str.c_str());
					SDL_Delay(30000);
				}
			}
		}
	})*/;
	/*app->getFunctionMap().insert_or_assign(SDLK_TAB,[&atlas](GUISYS* sys) {
		uSUrface s(SDL_CreateRGBSurfaceWithFormat(0,atlas.getTexture()->getWidth(),atlas.getTexture()->getHeight(),8,SDL_PIXELFORMAT_ARGB8888),SDL_FreeSurface);
		memcpy(s->pixels,atlas.getTexture()->getRawPixels(),s->w * s->h * 4);
		IMG_SavePNG(s.get(),"textureAtlas.png");
	});*/
	auto fontAdderThread = std::thread([app]() {
		FT_Library ft;
		if (FT_Init_FreeType(&ft)) {
			return;
		}
		TXT::sFreeTypeSystem sys(ft,FT_Done_FreeType);

		auto font = std::make_shared<TXT::FontRepository>( std::move(sys), true );
		font->initializeFont("Noto",CJK);
		app->setFont(std::move(font));
	});
	/*{
		FT_Library ft;
		if (!FT_Init_FreeType(&ft)) {
		} {
		TXT::sFreeTypeSystem sys(ft,FT_Done_FreeType);

		auto font = std::make_shared<TXT::FontRepository>( std::move(sys), true );
		font->initializeFont("Noto",CJK);
		app->setFont(std::move(font));
		}
	}*/
	omp_set_dynamic(1);     // Explicitly enable dynamic teams
	app->run();
	//fontAdderThread.join();
	delete app;
	return 0;
}
