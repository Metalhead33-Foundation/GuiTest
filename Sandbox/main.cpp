#include "sandbox.hpp"
#include "GameSystem.hpp"
#include <fstream>

int main(void)
{
	Euph::Conf::Configuration config;
	{
		std::ifstream istrm("config.ini", std::ios_base::in);
		if(istrm.is_open()) {
			config.fromString(istrm);
		}
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	//testConfiguration();
	//testCustomAllocator();
	/*testAssetRuntime();
	testGpuAssetStreamer();
	testAsyncIo();
	testTGA();
	testPNG();
	testMemoryArenas();
	testFontRoundTrip();
	testAudioEffects();
	testTextPipeline();
	testSignals();
	testGeometryBatches();
	testNetLoopback();
	testFontParser();*/
	GameSystem gamesys(config);
	gamesys.run();
	return 0;
}
