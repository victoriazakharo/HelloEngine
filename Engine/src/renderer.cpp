#include "renderer.h"
#include "render_params.h"

namespace HelloEngine 
{
	Renderer::Renderer() : m_Params(new RenderParameters())
	{
	}

	Renderer::~Renderer()
	{
		delete m_Params;
	}	
}
