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
		static inline void InitConsole2(const GameClock* game_clock) {};
		static inline void SetColor(int c) {};
		static inline void Log(const std::string& s) {};
		static inline void LogProgress(const std::string& s) {};
#else
		static inline void InitConsole(const GameClock* game_clock) { initConsole(game_clock); };
		static inline void InitConsole2(const GameClock* game_clock) { initConsole2(game_clock); };
		static inline void SetColor(int c) { setColor(c); };
		static inline void Log(const std::string& s) { log(s); };
		static inline void LogProgress(const std::string& s) { logProgress(s); };

#endif
	private:
		static void initConsole(const GameClock* game_clock);
		static void initConsole2(const GameClock* game_clock); // Used for pure console apps
		static void setColor(int c);
		static void log(const std::string& s);
		static void logProgress(const std::string& s);

	};
}