#include "keyboard_state.h"
#include <string.h>

namespace
{
	inline bool getChar(const unsigned int* table, unsigned char c)
	{
		int table_index = c / sizeof(unsigned int);
		int int_index = c % sizeof(unsigned int);
		return (table[table_index] & (0x01 << int_index)) != 0;
	}
	inline void setChar(unsigned int* table, unsigned char c, bool v)
	{
		int table_index = c / sizeof(unsigned int);
		int int_index = c % sizeof(unsigned int);
		table[table_index] |= int(v) << int_index;
	}
}

eio::KeyboardState::KeyboardState()
	: key_states(), last_key_states()
{

}

bool eio::KeyboardState::IsKeyUp(unsigned char key) const
{
	return !getChar(key_states, key);
}
bool eio::KeyboardState::IsKeyDown(unsigned char key) const
{
	return getChar(key_states, key);
}

bool eio::KeyboardState::IsKeyReleased(unsigned char key) const
{
	return !getChar(key_states, key) && getChar(last_key_states, key);
}
bool eio::KeyboardState::IsKeyPressed(unsigned char key) const
{
	return getChar(key_states, key) && !getChar(last_key_states, key);
}

void eio::KeyboardState::reset()
{
	memcpy((void*)last_key_states, (void*)key_states, num_characters / sizeof(unsigned int));
}
void eio::KeyboardState::setKeyDown(unsigned char key)
{
	setChar(key_states, key, true);
}
void eio::KeyboardState::setKeyUp(unsigned char key)
{
	setChar(key_states, key, false);
}
