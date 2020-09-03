#include "game_clock.h"
#include "console.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define SEC_TO_MICROSEC 1000000

eio::GameClock::GameClock()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&count_frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&start_time);
}

long long eio::GameClock::GetTime() const
{
	long long current_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
	return (current_time - start_time) * SEC_TO_MICROSEC / count_frequency;
}