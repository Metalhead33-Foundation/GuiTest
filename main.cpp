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
#include <MhLib/Media/Audio/MhSoundFile.hpp>
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
	std::mt19937 mt19937;
	std::vector<float> valueNoise(frameInsert.height * frameInsert.width);
	MH33::Util::Noise::sumOfSines2D(valueNoise.data(),frameInsert.width,frameInsert.height,4.0f,96,[](float x, float y) {
		return std::sin(std::fmod(x, M_PI * 2.0f)) * std::sin(std::fmod(y, M_PI * 2.0f));
	} );
	MH33::Util::Noise::remapToUnsigned(valueNoise);
	std::span<uint8_t> out = frameInsert.asDataSpan<uint8_t>();
	MH33::Util::fdenormalize(valueNoise,out);
	MH33::Io::uFile ufile = std::make_unique<MH33::Io::File>("/tmp/perlin.png", MH33::Io::Mode::WRITE);
	MH33::Image::PNG::encode(*ufile,decodeTarget, 1.0f);
}
void tryPinkNoise() {
	MH33::Audio::SoundFileCreateInfo sndCreateInfo;
	sndCreateInfo.channelCount.var = 2;
	sndCreateInfo.frameRate.var = 44100;
	sndCreateInfo.channelCount.var = 1;
	sndCreateInfo.format = MH33::Audio::SoundFormat::WAV_PCM_16;
	MH33::Audio::SoundFile sfile([](MH33::Io::Mode mode){ return new MH33::Io::File("/tmp/perlin.wav", mode); }, MH33::Io::Mode::WRITE, &sndCreateInfo);
	std::vector<float> output(sndCreateInfo.frameRate.var * 10);
	std::mt19937 mt19937;
	MH33::Util::Noise::sumOfSines1D(output,1024.0f,64,[](float x){ return std::sin(std::fmod(x, M_PI * 2.0f)); });
	sfile.write(output.data(),MH33::Audio::SampleCount(output.size()));
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
	tryPinkNoise();
	return 0;
}
