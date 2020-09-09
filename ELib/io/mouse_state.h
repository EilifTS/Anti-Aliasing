#pragma once
#include "../math/point2d.h"

class Window;
namespace eio
{
	class InputManager;
	enum class MouseButton { Left, Middle, Right };
	class MouseState
	{
	public:
		MouseState(const ema::point2D& lock_pos);

		bool IsButtonDown(MouseButton b) const;
		bool IsButtonUp(MouseButton b) const;
		bool ButtonClick(MouseButton b) const;
		bool ButtonRelease(MouseButton b) const;

		const ema::point2D& Position() const;
		ema::point2D Movement() const;

	private:
		ema::point2D pos;
		ema::point2D last_pos;
		bool button_state[3];
		bool button_last_state[3];
		int mouse_scroll_change;

	private:
		void updateMousePosition(const ema::point2D& new_pos);
		void updateMouseScroll(int scroll);
		void updateMouseButtonState(int button, bool down);

		// Reset mouse state before updating current mousestate
		void reset();
		friend InputManager;
		friend Window;
	};
}