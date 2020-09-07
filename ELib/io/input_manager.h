#pragma once
#include "keyboard_state.h"
#include "mouse_state.h"
#include "window_state.h"
#include "game_clock.h"

namespace eio
{
	class InputManager
	{
	public:
		InputManager(const ema::point2D& window_size, bool fullscreen_activated)
			: keyboard(), mouse(), game_clock(), window(window_size, fullscreen_activated)
		{

		}

		KeyboardState& Keyboard() { return keyboard; };
		const KeyboardState& Keyboard() const { return keyboard; };
		MouseState& Mouse() { return mouse; };
		const MouseState& Mouse() const { return mouse; };
		GameClock& Clock() { return game_clock; };
		const GameClock& Clock() const { return game_clock; };
		WindowState& Window() { return window; };
		const WindowState& Window() const { return window; };

	private:
		KeyboardState keyboard;
		MouseState mouse;
		GameClock game_clock;
		WindowState window;
	};
}