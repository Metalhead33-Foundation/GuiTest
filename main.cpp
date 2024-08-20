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
	/*const float topLeft = static_cast<float>(33) / static_cast<float>(255);
	const float topRight = static_cast<float>(66) / static_cast<float>(255);
	const float bottomLeft = static_cast<float>(99) / static_cast<float>(255);
	const float bottomRight = static_cast<float>(254) / static_cast<float>(255);*/
	std::mt19937 mt19937;
	//MH33::Util::Noise::rng_bilinearPerlin2D(frameInsert.width,frameInsert.height,floatTest.data(),mt19937,16,topLeft,topRight,bottomLeft,bottomRight,1.2f,0.95f);
	/*std::vector<float> voronoi(frameInsert.height * frameInsert.width);
	std::vector<float> perlin(frameInsert.height * frameInsert.width);
	MH33::Util::Noise::rng_voronoi2D(frameInsert.width,frameInsert.height,voronoi.data(),64,64,mt19937,false,false);
	MH33::Util::Noise::rng_perlin2D(frameInsert.width,frameInsert.height,perlin.data(),mt19937,8,1.5f,true);*/
	std::vector<float> valueNoise(frameInsert.height * frameInsert.width);
	MH33::Util::Noise::rng_valueNoise2D(valueNoise.data(),frameInsert.width,frameInsert.height,mt19937,128,128,MH33::Util::Noise::Interpolation::SMOOTHSTEP);
	std::span<uint8_t> out(reinterpret_cast<uint8_t*>(frameInsert.imageData.data()),frameInsert.height * frameInsert.width);
	for(size_t i = 0; i < out.size(); ++i) {
		//out[i] = MH33::Util::fdenormalize<uint8_t>(voronoi[i] * perlin[i]);
		out[i] = MH33::Util::fdenormalize<uint8_t>(valueNoise[i]);
	}
	MH33::Io::uFile ufile = std::make_unique<MH33::Io::File>("/tmp/perlin.png", MH33::Io::Mode::WRITE);
	MH33::Image::PNG::encode(*ufile,decodeTarget, 1.0f);
}
void tryPinkNoise() {
	//MH33::Io::uFile ufile = std::make_unique<MH33::Io::File>("/tmp/perlin.wav", MH33::Io::Mode::WRITE);
	MH33::Audio::SoundFileCreateInfo sndCreateInfo;
	sndCreateInfo.channelCount.var = 2;
	sndCreateInfo.frameRate.var = 44100;
	sndCreateInfo.channelCount.var = 1;
	sndCreateInfo.format = MH33::Audio::SoundFormat::WAV_PCM_16;
	MH33::Audio::SoundFile sfile([](MH33::Io::Mode mode){ return new MH33::Io::File("/tmp/perlin.wav", mode); }, MH33::Io::Mode::WRITE, &sndCreateInfo);
	//std::vector<float> perlin(sndCreateInfo.frameRate.var * 10);
	//std::vector<float> voronoi(sndCreateInfo.frameRate.var * 10);
	std::vector<float> output(sndCreateInfo.frameRate.var * 10);
	std::mt19937 mt19937;
	MH33::Util::Noise::rng_valueNoise1D(output,mt19937,44100/15,MH33::Util::Noise::Interpolation::SMOOTHSTEP);
	MH33::Util::Noise::remapToSigned(output);
	/*MH33::Util::Noise::rng_perlin1D(perlin,mt19937,128,1.05f,true);
	MH33::Util::Noise::rng_voronoi1D(voronoi,1024,mt19937,false);
	for(size_t i = 0; i < output.size(); ++i) {
		output[i] = perlin[i] * voronoi[i];
	}
	for(auto& it : output) {
		it = (it - 0.5f) * 2.0f;
	}*/
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
