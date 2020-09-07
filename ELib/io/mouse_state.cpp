#include "mouse_state.h"
#include <assert.h>

eio::MouseState::MouseState()
	: pos(), button_state(), button_last_state(), mouse_scroll_change()
{

}

bool eio::MouseState::IsButtonDown(MouseButton b) const
{
	return button_state[(int)b];
}
bool eio::MouseState::IsButtonUp(MouseButton b) const
{
	return !button_state[(int)b];
}
bool eio::MouseState::ButtonClick(MouseButton b) const
{
	return button_state[(int)b] && !button_last_state[(int)b];
}
bool eio::MouseState::ButtonRelease(MouseButton b) const
{
	return !button_state[(int)b] && button_last_state[(int)b];
}

const ema::point2D& eio::MouseState::Position() const
{
	return pos;
}
ema::point2D eio::MouseState::Movement() const
{
	return pos - last_pos;
}


void eio::MouseState::updateMousePosition(const ema::point2D& new_pos)
{
	pos = new_pos;
}
void eio::MouseState::updateMouseScroll(int scroll)
{
	mouse_scroll_change = scroll;
}
void eio::MouseState::updateMouseButtonState(int button, bool down)
{
	assert(button >= 0 && button <= 2);
	button_state[button] = down;
}
void eio::MouseState::reset()
{
	last_pos = pos;
	mouse_scroll_change = 0;
	button_last_state[0] = button_state[0];
	button_last_state[1] = button_state[1];
	button_last_state[2] = button_state[2];
}