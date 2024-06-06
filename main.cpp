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
#define USE_GLX

int main(int argc, char *argv[]) {
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
	return 0;
}
