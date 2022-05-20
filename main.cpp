#include <iostream>
#include "System/GuiRenderSystem.hpp"
#include "Util/PixelFormat.hpp"
#include "Util/TextureFromSurface.hpp"

static const int WIDTH = 640;
static const int HEIGHT = 480;

int main()
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window * window = SDL_CreateWindow("",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		640, 480,
		0);

	//
	// Set up the method for displaying the pixel buffer.
	//
	SDL_Surface * window_surface = SDL_GetWindowSurface(window);
	puts(SDL_GetPixelFormatName(window_surface->format->format));
	auto framebuffer = textureFromSurface(*window_surface);
	framebuffer->clearToColour(glm::vec4(0.2f,0.7f,0.55f,1.0f));
	SDL_UpdateWindowSurface(window);
	framebuffer->clearToColour([](const glm::ivec2& winpos) {
		const float r = static_cast<float>(winpos.x) / static_cast<float>(WIDTH);
		const float g = static_cast<float>(winpos.y) / static_cast<float>(HEIGHT);
		return glm::vec4(r,g,0.0f,1.0f);
	});
	SDL_UpdateWindowSurface(window);

	while (1)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) exit(0);
		}
		//framebuffer->clearToColour(glm::vec4(0.0f,0.2f,1.0f,1.0f));
		framebuffer->clearToColour([](const glm::ivec2& winpos) {
			const float r = static_cast<float>(winpos.x) / static_cast<float>(WIDTH);
			const float g = static_cast<float>(winpos.y) / static_cast<float>(HEIGHT);
			return glm::vec4(r,g,0.0f,1.0f);
		});
		SDL_UpdateWindowSurface(window);
	}
	//SDL_Init(SDL_INIT_VIDEO);
	//GuiRenderSystem* app = new GuiRenderSystem("GUI Render Demo",0,0,WIDTH,HEIGHT,0);
	//app->run();
	//delete app;
	return 0;
}
