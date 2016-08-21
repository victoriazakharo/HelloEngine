#include "window.h"
#include "window_params.h"

namespace HelloEngine 
{

	Window::Window() :	
		m_Parameters(new WindowParameters()),		
		m_CloseRequested(false), 
		m_ResizeRequested(false)
	{
	}

	bool Window::IsCloseRequested() const
	{
		return m_CloseRequested;
	}

	bool Window::IsResizeRequested()
	{
		if(m_ResizeRequested)
		{
			m_ResizeRequested = false;
			return true;
		}
		return false;
	}

	WindowParameters * Window::GetParameters() const
	{
		return m_Parameters;
	}

	void Window::AddEventHandler(WindowEventHandler* eventHandler)
	{
		m_EventHandlers.push_back(eventHandler);
	}
}
