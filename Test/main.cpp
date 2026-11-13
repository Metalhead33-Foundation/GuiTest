#include "test.hpp"

int main(void)
{
	//testConfiguration();
	//testCustomAllocator();
	testAssetRuntime();
	testGpuAssetStreamer();
	testAsyncIo();
	testTGA();
	testPNG();
	testMemoryArenas();
	testFontParser();
	return 0;
}
