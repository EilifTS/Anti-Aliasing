#include <stdexcept>
#include <string>

#include "window/window.h"
#include "io/console.h"
#include "misc/string_helpers.h"
#include "graphics/device.h"
#include "graphics/command_context.h"

#include "app.h"

#pragma comment(lib, "windowscodecs.lib")
//#include <wrl/wrappers/corewrappers.h>

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow
)
{
	try
	{
		// Initialize windows runtime
		//Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
		//if (FAILED(initialize))
		//{
		//	throw std::runtime_error("Failed to initialize windows runtime");
		//	return 1;
		//}

		// 1920x1080 window and no fullscreen
		eio::InputManager input_manager({1920, 1080}, false);

		Window window(hInstance, hPrevInstance, lpCmdLine, nCmdShow, input_manager);

		eio::Console::InitConsole(&input_manager.Clock());
		eio::Console::Log("Console open!");

		egx::Device device(window, input_manager, false, 3);
		egx::CommandContext context(device, input_manager.Window().WindowSize());

		App app(device, context, input_manager);

		// Main game loop
		int frames = 0;
		auto sec_start = input_manager.Clock().GetTime();
		while (true)
		{
			input_manager.ResetOnFrameStart();

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
		device.WaitForGPU();
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