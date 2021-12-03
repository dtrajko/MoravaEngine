#pragma once

#include "Hazel/Renderer/RendererContext.h"

#include "Core/Window.h"


struct GLFWwindow;

namespace Hazel {

	class OpenGLContext : public RendererContext
	{
	public:
		OpenGLContext(Window* window);
		virtual ~OpenGLContext();

		virtual void Create() override;
		virtual void BeginFrame() override {}
		virtual void SwapBuffers() override;
		virtual void OnResize(uint32_t width, uint32_t height) override {}

	private:
		Window* m_Window;

	};

}
