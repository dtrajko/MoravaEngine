/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"


class Window;
struct GLFWwindow;

namespace H2M
{

	class RendererContextH2M : public RefCountedH2M
	{
	public:
		RendererContextH2M() = default;
		virtual ~RendererContextH2M() = default;

		virtual void Create() = 0;
		virtual void BeginFrame() = 0;
		virtual void SwapBuffers() = 0;

		virtual void OnResize(uint32_t width, uint32_t height) = 0;

		static RefH2M<RendererContextH2M> Create(Window* window);
	};

}
