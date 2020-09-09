#pragma once

class Window;
namespace eio
{
	class InputManager;
	class KeyboardState
	{
	public:
		KeyboardState();

		

		bool IsKeyUp(unsigned char key) const;
		bool IsKeyDown(unsigned char key) const;
		bool IsKeyReleased(unsigned char key) const;
		bool IsKeyPressed(unsigned char key) const;

	private:
		static const unsigned int num_characters = 256;

		// 1 indicates keydown, 0 keyup
		unsigned int key_states[num_characters / sizeof(unsigned int)];
		unsigned int last_key_states[num_characters / sizeof(unsigned int)];

	private:
		// Reset keyboard state before updating current keystates
		void reset();
		void setKeyDown(unsigned char key);
		void setKeyUp(unsigned char key);

		friend Window;
		friend InputManager;
	};

}