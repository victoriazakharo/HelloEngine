#pragma once
#include "window.h"

namespace HelloEngine 
{
	class RenderParameters;

	class HELLO_ENGINE_API Renderer {
	public:
		Renderer();
		~Renderer();

		bool Initialize(WindowParameters *parameters);
		bool OnWindowSizeChanged();
		bool ReadyToDraw() const;
		bool Draw();
	private:
		RenderParameters *m_Params;
	};
}

