#include "game.h"
#include <stdio.h>
#include <thread>

void Game::OnLButtonDown(int x, int y)
{
	printf("Left button down at %d %d\n", x, y);
}

bool Game::Initialize()
{
	m_Window.AddEventHandler(this);	
	if (!m_Window.Create("Test")) {
		return false;
	}	
	if (!m_Renderer.Initialize(m_Window.GetParameters())) {
		return false;
	}
	return true;
}

void Game::Run()
{
	while (!m_Window.IsCloseRequested()) {
		if (!m_Window.ProcessInput()) {
			if (m_Window.IsResizeRequested()) {				
				if (!m_Renderer.OnWindowSizeChanged()) {
					break;
				}
			}
			if (m_Renderer.ReadyToDraw()) {
				if (!m_Renderer.Draw()) {
					break;
				}
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
	}
}