#include "render_params.h"
#include "window_params.h"
#include "renderer.h"

namespace HelloEngine
{
	bool Renderer::Initialize(WindowParameters *parameters)
	{
		return true;
	}

	bool Renderer::OnWindowSizeChanged()
	{
		return true;
	}

	bool Renderer::ReadyToDraw() const
	{
		return true;
	}

	bool Renderer::Draw()
	{
		return true;
	}	
}