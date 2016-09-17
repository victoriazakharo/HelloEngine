#ifdef USE_PLATFORM_WIN32_KHR
#ifndef WINDOW_PARAMS_H
#define WINDOW_PARAMS_H
#pragma once
#include <Windows.h>

namespace HelloEngine 
{	
	struct WindowParameters {	
		HINSTANCE	instance;	
		HWND		handle;
		WindowParameters() :
			instance(),
			handle() {}	
	};	

	struct WindowEventParameters {
		UINT		message;
		WPARAM		wParam;
		LPARAM		lParam;
	};
}

#endif
#endif