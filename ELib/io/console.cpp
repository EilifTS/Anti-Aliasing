#include "console.h"
#include "../misc/string_helpers.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

#ifdef USE_CONSOLE
static const WORD MAX_CONSOLE_LINES = 500;

namespace
{
	static const eio::GameClock* pgame_clock;
}

void eio::Console::InitConsole(const GameClock* game_clock)
{
	pgame_clock = game_clock;
	// allocate a console for this app
	AllocConsole();
	FILE* fi = 0;
	freopen_s(&fi, "CONOUT$", "w", stdout);
	freopen_s(&fi, "CONOUT$", "w", stderr);
	freopen_s(&fi, "CONIN$", "r", stdin);

	// set the screen buffer to be big enough to let us scroll text
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);
}

void eio::Console::SetColor(int c	)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}


void eio::Console::Log(const std::string& s)
{
	std::cout << emisc::TimeToString(pgame_clock->GetTime()) << ": " << s << std::endl;
}

#endif