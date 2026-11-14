#include "sandbox.hpp"

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
	testFontRoundTrip();
	testAudioEffects();
	testTextPipeline();
	testSignals();
	testGeometryBatches();
	testFontParser();
	return 0;
}
