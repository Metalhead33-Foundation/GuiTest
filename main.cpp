#include <span>
#include <iostream>
#include "GFX/Abstract/VertexBufferAdapter.hpp"
#include <vector>
#include <cstring>
#include "System/IniConfiguration.hpp"
#include <fstream>
#include "System/TestSystem.hpp"
#include <GFX/GL/GlxContext.hpp>
#include <GFX/GL/EglContext.hpp>
#include <MhLib/IoSys/PhysFSIoSystem.hpp>
#include <MhLib/Util/MhNoise.hpp>
#include <MhLib/Io/MhFile.hpp>
#include <MhLib/Media/Image/MhPNG.hpp>
#define USE_GLX

void tryPerlinNoise() {
	MH33::Image::DecodeTarget decodeTarget;
	decodeTarget.format = MH33::Image::Format::R8U;
	decodeTarget.frames.resize(1);
	auto& frameInsert = decodeTarget.frames[0];
	frameInsert.height = 1024;
	frameInsert.width = 1024;
	frameInsert.stride = frameInsert.width;
	frameInsert.imageData.resize(frameInsert.height * frameInsert.width);
	std::vector<float> floatTest(frameInsert.height * frameInsert.width);
	MH33::Util::Noise::rand_perlin2D(frameInsert.width,frameInsert.height,floatTest.data(),1000,16,2.0f);
	std::span<uint8_t> out(reinterpret_cast<uint8_t*>(frameInsert.imageData.data()),frameInsert.height * frameInsert.width);
	for(size_t i = 0; i < out.size(); ++i) {
		out[i] = MH33::Util::fdenormalize<uint8_t>(floatTest[i]);
	}
	MH33::Io::uFile ufile = std::make_unique<MH33::Io::File>("/tmp/perlin.png", MH33::Io::Mode::WRITE);
	MH33::Image::PNG::encode(*ufile,decodeTarget, 1.0f);
}

int main(int argc, char *argv[]) {
	/*SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if(JS::Core::inintializeJs()) {
	PhysFS::IoSystem::initialize(argv[0]);
	PhysFS::sIoSystem iosys(new PhysFS::IoSystem());
	iosys->mount(iosys->getBaseDir(),"/");
	std::ifstream ifs;
	ifs.open("hello.ini");
	IniConfiguration ini(ifs);
	ifs.close();
	ini["Video"].getValueOrDefault("sRenderer","OpenGL");
	for(auto i = std::begin(ini); i != std::end(ini) ; ++i ) {
		std::cout << i->first << std::endl;
		for(auto j = std::begin(i->second) ; j != std::end( i->second ) ; ++j ) {
			std::cout << std::boolalpha << j->first << '-' << j->second << std::endl;
		}
	}
#ifdef USE_GLX
	auto initializer = [](const SDL_SysWMinfo& syswmi) {
		return new GLX::RenderingContext(syswmi);
	};
#else
	auto initializer = [](const SDL_SysWMinfo& syswmi) {
		return new EGL::RenderingContext(syswmi);
	};
#endif
	TestSystem testSys(iosys,initializer, ini);
	testSys.run();
	JS::Core::shutdownJs();
	}*/
	tryPerlinNoise();
	return 0;
}
