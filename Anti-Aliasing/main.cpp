#include <stdexcept>
#include <string>

#include "window/window.h"
#include "io/console.h"
#include "graphics/device.h"


int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	try
	{
		// 1920x1080 window and no fullscreen
		eio::InputManager input_manager({1920, 1080}, false);

		eio::Console::InitConsole(&input_manager.Clock());
		eio::Console::Log("Console open!");

		Window window(hInstance, hPrevInstance, lpCmdLine, nCmdShow, input_manager);
		eio::Console::Log("Window open!");

		egx::Device device(window, input_manager, false);

		// Main game loop
		while (true)
		{
			window.HandleWindowMessages();
			if (window.CloseWindow())
				break;
		}
	}
	catch (std::runtime_error& e)
	{
		std::string error_message(e.what());
		std::wstring error_message_wide(error_message.begin(), error_message.end());
		MessageBox(
			NULL,
			error_message_wide.c_str(),
			L"Runtime exception thrown",
			MB_OK);
	}

	return 0;
}