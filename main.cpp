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
	app->run();
	delete app;
	return 0;
}
