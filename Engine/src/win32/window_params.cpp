#ifdef USE_PLATFORM_WIN32_KHR
#include "window.h"
#include "window_params.h"
#include <windowsx.h>

namespace HelloEngine 
{
#define WINDOW_CLASS_NAME "Hello Engine Test"

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		Window* window;
		if (message == WM_NCCREATE) {
			auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
			window = reinterpret_cast<Window*>(createStruct->lpCreateParams);
			window->GetParameters()->handle = hWnd;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else {
			auto data = GetWindowLongPtr(hWnd, GWLP_USERDATA);			
			window = reinterpret_cast<Window*>(data);
		}

		if (window != nullptr) {
			window->HandleEvent({ message, wParam, lParam });
		}

		return DefWindowProc(hWnd, message, wParam, lParam);		
	}

	void Window::HandleEvent(WindowEventParameters message)
	{	
		switch (message.message) {
		case WM_SIZE:
		case WM_EXITSIZEMOVE:
			m_ResizeRequested = true;
			break;		
		case WM_CLOSE:
			m_CloseRequested = true;		
			break;				
		case WM_LBUTTONDOWN:
			for (auto eventHandler : m_EventHandlers) {
				eventHandler->OnLButtonDown(GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
			}		
			break;
		case WM_RBUTTONDOWN:
			for (auto eventHandler : m_EventHandlers) {
				eventHandler->OnRButtonDown(GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
			}
			break;
		case WM_LBUTTONUP:
			for (auto eventHandler : m_EventHandlers) {
				eventHandler->OnLButtonUp(GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
			}
			break;
		case WM_RBUTTONUP:
			for (auto eventHandler : m_EventHandlers) {
				eventHandler->OnRButtonUp(GET_X_LPARAM(message.lParam), GET_Y_LPARAM(message.lParam));
			}			
			break;		
		}
	}

	Window::~Window() {
		if (m_Parameters->handle) {
			DestroyWindow(m_Parameters->handle);
		}

		if (m_Parameters->instance) {
			UnregisterClass(WINDOW_CLASS_NAME, m_Parameters->instance);
		}
		delete m_Parameters;
	}

	bool Window::Create(const char *title) 
	{
		m_Parameters->instance = GetModuleHandle(nullptr);
		WNDCLASSEX wcex{};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = &WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = m_Parameters->instance;
		wcex.hIcon = nullptr;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = WINDOW_CLASS_NAME;
		wcex.hIconSm = nullptr;
		if (!RegisterClassEx(&wcex)) {
			return false;
		}
		m_Parameters->handle = CreateWindow(WINDOW_CLASS_NAME, title, WS_OVERLAPPEDWINDOW, 20, 20, 500, 500, nullptr, nullptr, m_Parameters->instance, this);
		if (!m_Parameters->handle) {
			return false;
		}
		ShowWindow(m_Parameters->handle, SW_SHOWNORMAL);
		UpdateWindow(m_Parameters->handle);
		return true;
	}

	bool Window::ProcessInput()
	{
		MSG message;
		if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {			
			TranslateMessage(&message);
			DispatchMessage(&message);
			return true;
		}
		return false;
	}
}
#endif
