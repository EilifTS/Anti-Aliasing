#pragma once
#include "../math/point2d.h"

class Window;
namespace eio
{
	class WindowState
	{
	public:
		WindowState(const ema::point2D& window_size, bool fullscreen_activated) 
			: window_size(window_size), fullscreen_activated(fullscreen_activated) 
		{};

		const ema::point2D& WindowSize() const { return window_size; };
		ema::point2D& WindowSize() { return window_size; };
		bool IsFullscreen() const { return fullscreen_activated; };

	private:
		ema::point2D window_size;
		bool fullscreen_activated;

		friend Window;
	};
}
