#include <iostream>
#include "System/GuiRenderSystem.hpp"
#include "Util/PixelFormat.hpp"
#include "Util/TextureFromSurface.hpp"
#include <thread>
#include <SDL2/SDL_image.h>
#include <sstream>

/**static const int WIDTH = 640;
static const int HEIGHT = 480;*/

static const int WIDTH = 1024;
static const int HEIGHT = 768;

static const char* DOSFONT = "Perfect DOS VGA 437.ttf";
static const char* ARIALFONT = "arial.ttf";
static const char* CJK = "NotoTraditionalNushu-Regular.ttf";

typedef std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> uSUrface;

int main()
{
	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	GuiRenderSystem* app = new GuiRenderSystem("GUI Render Demo",0,0,WIDTH,HEIGHT,0);
	auto cursorAdderThread = std::thread([app]() {
		// textureFromSurfaceCopy
		std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> surfacePtr(IMG_Load("wodmouse.png"),SDL_FreeSurface);
		auto tex = textureFromSurfaceCopy(*surfacePtr);
		sCursor cursor = std::make_shared<Cursor>(std::move(tex));
		app->setCursor(std::move(cursor));
	});
	app->getFunctionMap().insert_or_assign(SDLK_SPACE,[](GuiRenderSystem* sys) {
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
				}
			}
		}
	});
	/*auto fontAdderThread = std::thread([app]() {
		FT_Library ft;
		if (FT_Init_FreeType(&ft)) {
			return;
		}
		sFreeTypeSystem sys(ft,FT_Done_FreeType);

		auto font = std::make_shared<FontRepository>( std::move(sys) );
		font->initializeFont("Noto",CJK);
		app->setFont(std::move(font));
	});*/
	{
		FT_Library ft;
		if (!FT_Init_FreeType(&ft)) {
		} {
		sFreeTypeSystem sys(ft,FT_Done_FreeType);

		auto font = std::make_shared<FontRepository>( std::move(sys) );
		font->initializeFont("Noto",CJK);
		app->setFont(std::move(font));
		}
	}
	app->run();
	//fontAdderThread.join();
	delete app;
	return 0;
}
