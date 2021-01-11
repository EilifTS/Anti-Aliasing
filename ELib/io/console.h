#pragma once
#include <string>
#include "game_clock.h"

#ifdef _DEBUG
#define USE_CONSOLE
#endif

namespace eio
{
	class Console
	{
	public:
#ifndef USE_CONSOLE
		static inline void InitConsole(const GameClock* game_clock) {};
		static void InitConsole2(const GameClock* game_clock) {};
		static inline void SetColor(int c) {};
		static inline void Log(const std::string& s) {};
#else
		static void InitConsole(const GameClock* game_clock);
		static void InitConsole2(const GameClock* game_clock); // Used for pure console apps
		static void SetColor(int c);
		static void Log(const std::string& s);
		static void LogProgress(const std::string& s);

#endif

	};
}