#ifndef WINDOW_H
#define WINDOW_H
#pragma once
#include "hello_export.h"
#include <vector>

namespace HelloEngine 
{
	struct WindowParameters;
	struct WindowEventParameters;

	enum class Key {
		ESC, UP, DOWN, LEFT, RIGHT, SPACE
	};		

	class HELLO_ENGINE_API WindowEventHandler {
	public:
		virtual ~WindowEventHandler() {}

		virtual void OnKeyDown(Key key) {};
		virtual void OnKeyUp(Key key) {};
		virtual void OnLButtonDown(int x, int y) {};
		virtual void OnLButtonUp(int x, int y) {};
		virtual void OnRButtonDown(int x, int y) {};
		virtual void OnRButtonUp(int x, int y) {};	
	};

	class HELLO_ENGINE_API Window {
	public:
		Window();
		~Window();

		bool					Create(const char *title, const int x = 20, const int y = 20, const int w = 500, const int h = 500);
		bool					ProcessInput();			
		bool					IsCloseRequested() const;
		bool					IsResizeRequested();
		WindowParameters		*GetParameters() const;
		void					AddEventHandler(WindowEventHandler* eventHandler);
		void                    HandleEvent(WindowEventParameters event);
	private:
		WindowParameters				 *m_Parameters;
		std::vector<WindowEventHandler*> m_EventHandlers;
		bool							 m_CloseRequested;
		bool							 m_ResizeRequested;
	};
}
#endif