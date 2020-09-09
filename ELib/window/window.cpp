#include "window.h"
#include <stdexcept>

namespace
{
	void centerMouse(HWND hwnd, const ema::point2D& center_pos)
	{
		POINT screen_center;
		screen_center.x = center_pos.x;
		screen_center.y = center_pos.y;
		ClientToScreen(hwnd, &screen_center);
		SetCursorPos(screen_center.x, screen_center.y);
	}
}

Window::Window(HINSTANCE hinstance, HINSTANCE hprevinstance, LPWSTR lpcmdline, int icmdshow, eio::InputManager& input_manager)
	: close_window(false), rinput_manager(input_manager)
{
	UNREFERENCED_PARAMETER(hprevinstance);
	UNREFERENCED_PARAMETER(lpcmdline);

	currentWindow = this;

	// Register the window class.
	const wchar_t kClassName[] = L"Anti-Aliasing Class";

	WNDCLASS wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hinstance;
	wc.lpszClassName = kClassName;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

	RegisterClass(&wc);

	DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	RECT screen_rect;
	const HWND h_desktop = GetDesktopWindow();
	GetWindowRect(h_desktop, &screen_rect);

	// Creating a window with client size equal to windowsize and placed in the middle of the screen
	RECT window_rect;

	if (input_manager.Window().IsFullscreen())
	{
		window_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;

		DEVMODE dm_screen_settings;
		memset(&dm_screen_settings, 0, sizeof(dm_screen_settings));
		dm_screen_settings.dmSize = sizeof(dm_screen_settings);
		dm_screen_settings.dmPelsWidth = (unsigned long)screen_rect.right;
		dm_screen_settings.dmPelsHeight = (unsigned long)screen_rect.bottom;
		dm_screen_settings.dmBitsPerPel = 32;
		dm_screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		//Change the display settings to fullscreen
		ChangeDisplaySettings(&dm_screen_settings, CDS_FULLSCREEN);

		window_rect = screen_rect;
		input_manager.Window().WindowSize().x = screen_rect.right;
		input_manager.Window().WindowSize().y = screen_rect.bottom;
	}
	else
	{
		window_rect.left = 0;
		window_rect.top = 0;
		window_rect.right = input_manager.Window().WindowSize().x;
		window_rect.bottom = input_manager.Window().WindowSize().y;
		AdjustWindowRect(&window_rect, window_style, false);

		window_rect.right -= window_rect.left;
		window_rect.left -= window_rect.left;
		window_rect.bottom -= window_rect.top;
		window_rect.top -= window_rect.top;

		window_rect.right += (screen_rect.right - input_manager.Window().WindowSize().x) / 2;
		window_rect.left += (screen_rect.right - input_manager.Window().WindowSize().x) / 2;
		window_rect.bottom += (screen_rect.bottom - input_manager.Window().WindowSize().y) / 2;
		window_rect.top += (screen_rect.bottom - input_manager.Window().WindowSize().y) / 2;
	}


	this->hwnd = CreateWindowEx(
		0,                              // Optional window styles. WS_EX_APPWINDOW
		kClassName,                     // Window class
		L"Summer",						// Window text
		window_style,					// Window style
										// Size and position
		window_rect.left,
		window_rect.top,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		NULL,							// Parent window    
		NULL,							// Menu
		hinstance,						// Instance handle
		NULL							// Additional application data
	);

	if (hwnd == NULL)
		throw std::runtime_error("Failed to create window");

	RECT w, c;
	GetWindowRect(hwnd, &w);
	GetClientRect(hwnd, &c);

	centerMouse(hwnd, rinput_manager.Window().WindowSize() / 2);

	ShowWindow(hwnd, icmdshow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);
}

Window::~Window()
{
	//Fix settings if in fullscreen
	if (rinput_manager.Window().IsFullscreen())
		ChangeDisplaySettings(NULL, 0);
}

void Window::HandleWindowMessages()
{
	MSG msg;
	while (!close_window && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
			close_window = true;
	}

	if (rinput_manager.Keyboard().IsKeyDown(27))
		DestroyWindow(hwnd);
}

LRESULT CALLBACK Window::MessageHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		rinput_manager.Keyboard().setKeyDown((unsigned char)wParam);
		return 0;
	}
	case WM_KEYUP:
	{
		rinput_manager.Keyboard().setKeyUp((unsigned char)wParam);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		POINT mp = { LOWORD(lParam), HIWORD(lParam) };
		
		rinput_manager.Mouse().updateMousePosition({ (int)mp.x, (int)mp.y });

		centerMouse(hwnd, rinput_manager.Window().WindowSize() / 2);
		
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		rinput_manager.Mouse().updateMouseScroll(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		rinput_manager.Mouse().updateMouseButtonState((int)eio::MouseButton::Left, true);
		return 0;
	}
	case WM_LBUTTONUP:
	{
		rinput_manager.Mouse().updateMouseButtonState((int)eio::MouseButton::Left, false);
		return 0;
	}
	case WM_MBUTTONDOWN:
	{
		rinput_manager.Mouse().updateMouseButtonState((int)eio::MouseButton::Middle, true);
		return 0;
	}
	case WM_MBUTTONUP:
	{
		rinput_manager.Mouse().updateMouseButtonState((int)eio::MouseButton::Middle, false);
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		rinput_manager.Mouse().updateMouseButtonState((int)eio::MouseButton::Right, true);
		return 0;
	}
	case WM_RBUTTONUP:
	{
		rinput_manager.Mouse().updateMouseButtonState((int)eio::MouseButton::Right, false);
		return 0;
	}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

HWND Window::Handle() const
{
	return hwnd;
}
bool Window::CloseWindow() const
{
	return close_window;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;
	default:
		return currentWindow->MessageHandler(hwnd, uMsg, wParam, lParam);
	}
}