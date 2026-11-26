#include "sandbox.hpp"
#include "GameSystem.hpp"
#include "ThemeDemo.hpp"
#include <fstream>
#include <Euphemy/Io/PhysFS.hpp>
#include <sstream>
#include "JS/JsCore.hpp"

int main(int argc, char *argv[])
{
	PhysFS::System::init(argv[0]);
	JS::Core::inintializeJs();
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
	physfsSystem.mount(physfsSystem.getBaseDir(),"/");
	physfsSystem.mount(writepath.c_str(),"/");
	{
		std::string paths = std::string(config.getString("Game","ModPaths"));
		std::string token;
		std::stringstream ss(paths);
		// Extract tokens separated by semicolons
		while (getline(ss, token, ';')) {
			physfsSystem.mount(token.c_str(),"");
		}

	}
	physfsSystem.mount("/home/legacy/blenda","/");
	physfsSystem.mount("/home/legacy/zene/GameMusic/Unreal Tournament/mod","/music");
	// A terminal-only path: parse the theme document, generate its procedural atlas,
	// dump the PNG next to the working directory and exit. It needs no window, no GPU
	// and no audio device, which is what makes it usable from CI.
	if (argc > 1 && std::string(argv[1]) == "--theme") {
		const int exitCode = Sandbox::runThemeDemo(physfsSystem, "/gfx/theme_000.json", "theme_out");
		JS::Core::shutdownJs();
		PhysFS::System::deinit();
		return exitCode;
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
	{
		GameSystem gamesys(config, physfsSystem);
		gamesys.run();
	}
	// The game's module streamer owns a PhysFS device. It must be destroyed while
	// PhysicsFS is still initialized, otherwise PHYSFS_close() runs on freed state.
	JS::Core::shutdownJs();
	PhysFS::System::deinit();
	return 0;
}
