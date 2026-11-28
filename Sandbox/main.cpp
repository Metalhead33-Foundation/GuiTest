#include "sandbox.hpp"
#include "GameSystem.hpp"
#include "ThemeDemo.hpp"
#include <cstdlib>
#include <iostream>
#include <Euphemy/Io/PhysFS.hpp>
#include <sstream>
#include <Elvavena/Io/ElvStreamBuffer.hpp>
#include "JS/JsCore.hpp"

int main(int argc, char *argv[])
{
	PhysFS::System::init(argv[0]);
	JS::Core::inintializeJs();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
	PhysFS::System physfsSystem;
	std::string writepath = std::string(physfsSystem.getBaseDir()) + physfsSystem.separator() + "overwrite";
	physfsSystem.setWriteDir(writepath.c_str());
	physfsSystem.mount(physfsSystem.getBaseDir(),"/");
	physfsSystem.mount(writepath.c_str(),"/");
	Euph::Conf::Configuration config;
	{
		auto inifile = physfsSystem.open("config.ini", Elv::Io::Mode::READ);

		if(inifile->isValid()) {
			Elv::Io::DeviceStreamBuffer<char> buf(*inifile);
			std::istream stream(&buf);
			stream.seekg(0);
			config.fromString(stream);
		}
	}
	{
		std::string paths = std::string(config.getString("Game","sModPaths"));
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
	{
		auto inifile = physfsSystem.open("config.ini", Elv::Io::Mode::WRITE);

		if(inifile->isValid()) {
			Elv::Io::DeviceStreamBuffer<char> buf(*inifile);
			std::ostream stream(&buf);
			stream << "; header test\n";
			//stream.seekg(0);
			stream << config << std::endl;
		}
		else {
			std::cerr << "Failed to open INI config for writing!" << std::endl;
		}
	}
	JS::Core::shutdownJs();
	PhysFS::System::deinit();
	std::cout << "Goodbye!" << std::endl;
	std::cout.flush();
	std::cerr.flush();
	// Everything this program owns has been released by now: `GameSystem` left its scope
	// above (the sound player, the script context, the renderer and the resource factory
	// are its members in the order that matters), and the two explicit shutdowns are done.
	// What is left is third-party process-global state. glslang and SPIRV-Tools keep
	// tables whose own static destructors walk the heap at exit, and once a Kaldi device
	// has been created and destroyed that teardown is not reliable: glibc reports a
	// corrupted arena from `~unordered_set<spv::Op>` in libSPIRV-Tools on roughly two out
	// of three runs (a run without a device - `--theme`, or `SDL_VIDEODRIVER=dummy` -
	// exits normally, and so does a standalone shader-compile probe). Leaving through
	// `_Exit` skips static destruction on purpose instead of depending on that; the
	// streams above are flushed, so nothing is lost. Revisit this if the shader toolchain
	// is ever updated or replaced.
	std::_Exit(0);
}
