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

		virtual void Init() = 0;

		static RefH2M<RendererContextH2M> Create(Window* window);

		// virtual void Create() = 0;                               // TODO: remove the method (replace with Init)
		virtual void SwapBuffers() = 0;                             // TODO: remove the method
		virtual void OnResize(uint32_t width, uint32_t height) = 0; // TODO: remove the method
		virtual void BeginFrame() = 0;                              // TODO: remove the method

	};

}
