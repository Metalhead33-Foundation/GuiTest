#include <iostream>
#include "System/GuiRenderSystem.hpp"
#include "Util/PixelFormat.hpp"
#include "Util/TextureFromSurface.hpp"
#include <thread>
#include <SDL2/SDL_image.h>

/**static const int WIDTH = 640;
static const int HEIGHT = 480;*/

static const int WIDTH = 1024;
static const int HEIGHT = 768;

static const char* DOSFONT = "Perfect DOS VGA 437.ttf";
static const char* ARIALFONT = "arial.ttf";
static const char* CJK = "NotoTraditionalNushu-Regular.ttf";

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
	auto fontAdderThread = std::thread([app]() {
		FT_Library ft;
		if (FT_Init_FreeType(&ft)) {
			return;
		}
		FT_Face face;
		if (FT_New_Face(ft, CJK, 0, &face)) {
			return;
		}
		FT_Set_Pixel_Sizes(face, 0, 48);
		auto font = std::make_shared<Font>(face);
		app->setFont(std::move(font));
	});
	app->run();
	delete app;
	return 0;
}
