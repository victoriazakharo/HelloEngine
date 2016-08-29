#pragma once
#include "window.h"
#include "renderer.h"

class Game : public HelloEngine::WindowEventHandler {
	const char           *NAME = "Test";
	HelloEngine::Window   m_Window;
	HelloEngine::Renderer m_Renderer;
public:
	bool Initialize();
	void Run();
	void OnLButtonDown(int x, int y) override;
};
