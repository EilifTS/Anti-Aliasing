#include "game_clock.h"
#include "console.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define SEC_TO_MICROSEC 1000000

eio::GameClock::GameClock()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&count_frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);

	last_frame_start_time = start_time;
	last_frame_end_time = start_time;
	last_frame_frame_time = 0.0;
}

void eio::GameClock::startFrame()
{
	last_frame_start_time = last_frame_end_time;
	last_frame_end_time = GetTime();
	last_frame_frame_time = (double)(last_frame_end_time - last_frame_start_time) / (double)SEC_TO_MICROSEC;
}

long long eio::GameClock::GetTime() const
{
	long long current_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
	return (current_time - start_time) * SEC_TO_MICROSEC / count_frequency;
}