#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "../io/input_manager.h"

class Window
{
public:
	Window(HINSTANCE hinstance, HINSTANCE hprevinstance, LPWSTR lpcmdline, int icmdshow, eio::InputManager& input_manager);
	~Window();

	void HandleWindowMessages();
	LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	HWND Handle() const;
	bool CloseWindow() const;

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

private:
	HWND hwnd;
	bool close_window;
	eio::InputManager& rinput_manager;
};

static Window* currentWindow;
