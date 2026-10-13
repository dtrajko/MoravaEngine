#include "Core/Window.h"

#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Platform/GLFW/GLFWWindow.h"

#if defined(HZ_PLATFORM_WINDOWS)
	#include "Platform/DX11/DX11Window.h"
#endif


Window* Window::Create(const WindowSpecification& windowSpecification)
{
#if defined(HZ_PLATFORM_WINDOWS)
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::DX11)
	{
		return new DX11Window(windowSpecification);
	}
#endif
	return new GLFWWindow(windowSpecification);
}
