#include <iostream>
#include <span>
#include <limits>
#include "test.hpp"
#include <fstream>
#include <Euphemy/Config/GlobalConfig.hpp>

Euph::Conf::Configuration GLOBAL_CONFIGURATION;

int main(void)
{
	std::ifstream ifs;
	ifs.open("hello.ini");
	GLOBAL_CONFIGURATION.fromString(ifs);
	//testGlobalConfigFile();
	//testCustomAllocator();
	testAsyncIo();
	testTGA();
	testPNG();
	return 0;
}
