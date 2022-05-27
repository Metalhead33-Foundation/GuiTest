#include "FpsCounter.hpp"
#include <SDL2/SDL.h>
#include <limits>
#include <cstring>

FpsCounter::FpsCounter()
	: minTime(std::numeric_limits<uint_fast32_t>::max()), maxTime(std::numeric_limits<uint_fast32_t>::min()), tickIndex(0), ticksRecorded(0)
{
	std::memset(recordedTicks.data(),0,sizeof(uint_fast32_t)*recordedTicks.size());
}

void FpsCounter::singleTick(const std::function<void ()>& toDo)
{
	const uint_fast32_t start_time = SDL_GetTicks();
	toDo();
	const uint_fast32_t frame_time = SDL_GetTicks()-start_time;
	recordedTicks[tickIndex] = frame_time;
	++ticksRecorded;
	tickIndex = ticksRecorded % recordedTicks.size();
	minTime = std::min(minTime,frame_time);
	maxTime = std::max(maxTime,frame_time);
}

void FpsCounter::queryData(float& minFps, float& avgFps, float& maxFps)
{
	minFps = (maxTime > 0) ? (1000.0f / static_cast<float>(maxTime)) : 0.0f;
	maxFps = (maxTime > 0) ? (1000.0f / static_cast<float>(minTime)) : 0.0f;
	avgFps = 0.0f;
	if(ticksRecorded <= recordedTicks.size()) {
		for(size_t i = 0; i < ticksRecorded; ++i) {
			const auto& tick = recordedTicks[i];
			avgFps += (tick > 0) ? (1000.0f / static_cast<float>(tick)) : 0.0f;
		}
		avgFps /= static_cast<float>(ticksRecorded);
	} else {
		for(const auto& tick : recordedTicks) {
			avgFps += (tick > 0) ? (1000.0f / static_cast<float>(tick)) : 0.0f;
		}
		avgFps /= static_cast<float>(recordedTicks.size());
	}
}
