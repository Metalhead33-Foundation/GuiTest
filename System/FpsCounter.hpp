#ifndef FPSCOUNTER_HPP
#define FPSCOUNTER_HPP
#include <array>
#include <cstdint>
#include <functional>

class FpsCounter
{
private:
	std::array<uint_fast32_t,256> recordedTicks;
	uint_fast32_t minTime;
	uint_fast32_t maxTime;
	size_t tickIndex;
	size_t ticksRecorded;
public:
	FpsCounter();
	void singleTick(const std::function<void()>& toDo);
	void queryData(float& minFps, float& avgFps, float& maxFps);
};

#endif // FPSCOUNTER_HPP
