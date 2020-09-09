#pragma once

namespace eio
{
	class InputManager;
	class GameClock
	{
	public:
		GameClock();


		// Returns time in microseconds
		long long GetTime() const;

		// Returns the frame time in seconds
		inline double FrameTime() const { return last_frame_frame_time; };

	private:
		long long count_frequency;
		long long start_time;

		long long last_frame_start_time;
		long long last_frame_end_time;
		double last_frame_frame_time;

	private:
		void startFrame();
		friend InputManager;
	};
}