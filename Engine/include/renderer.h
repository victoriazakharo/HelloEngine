#ifndef RENDERER_H
#define RENDERER_H
#pragma once
#include "window.h"
#include "color.h"

namespace HelloEngine 
{
	class RenderParameters;

	class HELLO_ENGINE_API Renderer {
	public:
		Renderer();
		~Renderer();

		bool Initialize(WindowParameters *parameters, const std::vector<float>& vertex_data, Color color = { 0.0f, 0.3f, 0.4f });
		bool OnWindowSizeChanged();
		bool ReadyToDraw() const;
		bool Draw();
	private:
		RenderParameters *m_Params;
	};
}
#endif

