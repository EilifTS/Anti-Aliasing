#include <stdexcept>
#include <string>

#include "window/window.h"
#include "io/console.h"
#include "misc/string_helpers.h"
#include "graphics/device.h"
#include "graphics/command_context.h"

#include "app.h"

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
		egx::CommandContext context(device);

		App app(device, context, input_manager);

		// Main game loop
		int frames = 0;
		auto sec_start = input_manager.Clock().GetTime();
		while (true)
		{
			window.HandleWindowMessages();
			if (window.CloseWindow())
				break;

			// Update
			app.Update(input_manager);

			// Render
			app.Render(device, context, input_manager);
			device.Present(context);

			// Calc FPS
			auto current_time = input_manager.Clock().GetTime();
			if (current_time - sec_start > 1000000)
			{
				eio::Console::Log("FPS: " + emisc::ToString(frames));
				frames = 0;
				sec_start = current_time;
			}
			frames++;
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