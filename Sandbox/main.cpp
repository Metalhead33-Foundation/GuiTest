#include "sandbox.hpp"
#include "GameSystem.hpp"
#include <fstream>
#include <Euphemy/Io/PhysFS.hpp>
#include <sstream>

int main(int argc, char *argv[])
{
	PhysFS::System::init(argv[0]);
	Euph::Conf::Configuration config;
	{
		std::ifstream istrm("config.ini", std::ios_base::in);
		if(istrm.is_open()) {
			config.fromString(istrm);
		}
	}
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	PhysFS::System physfsSystem;
	std::string writepath = std::string(physfsSystem.getBaseDir()) + physfsSystem.separator() + "overwrite";
	physfsSystem.mount(physfsSystem.getBaseDir(),"");
	physfsSystem.mount(writepath.c_str(),"");
	{
		std::string paths = std::string(config.getString("Game","ModPaths"));
		std::string token;
		std::stringstream ss(paths);
		// Extract tokens separated by semicolons
		while (getline(ss, token, ';')) {
			physfsSystem.mount(token.c_str(),"");
		}

	}
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
	GameSystem gamesys(config, physfsSystem);
	gamesys.run();
	PhysFS::System::deinit();
	return 0;
}
