#pragma once

namespace eio
{

	class GameClock
	{
	public:
		GameClock();

		// Returns time in microseconds
		long long GetTime() const;

	private:
		long long count_frequency;
		long long start_time;
	};
}